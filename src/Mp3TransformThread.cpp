/***************************************************************************
 *   MP3 Diags - diagnosis, repairs and tag editing for MP3 files          *
 *                                                                         *
 *   Copyright (C) 2009 by Marian Ciobanu                                  *
 *   ciobi@inbox.com                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include  <memory>

#include  <QMessageBox>

#include  "Mp3TransformThread.h"

#include  "ThreadRunnerDlgImpl.h"
#include  "Helpers.h"
#include  "CommonData.h"
#include  "Transformation.h"
#include  "OsFile.h"


using namespace std;
//using namespace pearl;


void logTransformation(const string& strLogFile, const char* szActionName, const string& strMp3File)
{
    time_t t (time(0));
    ofstream_utf8 out (strLogFile.c_str(), ios_base::app);
    out << "<" << strMp3File << "> <" << szActionName << "> - " << ctime(&t); // !!! ctime and a \n
}



//ttt2 perhaps make "fast-save aware" other transf that operate on id3v3 only (case transf, codepage, discards, ...); OTOH how likely is it to run 2 of these one ofer another? (otherwise you'd have to rescan anyway). still, perhaps allow proceeding in most cases without rescanning ID3V2 would be better, perhaps optional; then everything would be faster with ID3V2
namespace {


struct Mp3TransformThread : public PausableThread
{
    CommonData* m_pCommonData;
    const TransfConfig& m_transfConfig;
    //bool m_bAll; // if to use all handlers or only the selected ones
    const deque<const Mp3Handler*>& m_vpHndlr;

    vector<const Mp3Handler*>& m_vpDel;
    vector<const Mp3Handler*>& m_vpAdd; // for proc files that are in the same directory as the source and have a different name

    vector<Transformation*>& m_vpTransf;

    Mp3TransformThread(
        CommonData* pCommonData, const TransfConfig& transfConfig,
        const deque<const Mp3Handler*>& vpHndlr,
        vector<const Mp3Handler*>& vpDel,
        vector<const Mp3Handler*>& vpAdd,
        vector<Transformation*>& vpTransf) :

        m_pCommonData(pCommonData), m_transfConfig(transfConfig),
        m_vpHndlr(vpHndlr),
        m_vpDel(vpDel),
        m_vpAdd(vpAdd),
        m_vpTransf(vpTransf),
        m_bWriteError(true),
        m_bFileChanged(false)
    {
    }

    string m_strErrorFile; // normally this is empty; if it's not, writing to the specified file failed
    bool m_bWriteError;
    bool m_bFileChanged;

    /*override*/ void run()
    {
        try
        {
            CompleteNotif notif(this);

            notif.setSuccess(transform());
        }
        catch (...)
        {
            LAST_STEP("Mp3TransformThread::run()");
            CB_ASSERT (false);
        }
    }

    bool transform();
};



// the idea is to mark a file for deletion but only rename it, so other things can be done as if the file got erased, but if something goes wrong the file can be restored;
// the default behavior on the destructor is to restore the file, if the name isn't already used; to prevent the file from being restored, finalize() should be called after the things that could go wrong complete OK
class FileEraser
{
    string m_strOrigName;
    string m_strChangedName;
public:
    void erase(const string& strOrigName)
    {
        CB_ASSERT(m_strOrigName.empty());
        m_strOrigName = strOrigName;
        char a [20];

        for (int i = 1; i < 1000; ++i)
        {
            sprintf(a, ".QQREN%03dREN", i);
            m_strChangedName = strOrigName + a;
            if (!fileExists(m_strChangedName))
            {
                a[0] = 0;
                break;
            }
        }

        CB_ASSERT (0 == a[0]); // not really correct to assert, but quite likely

        try
        {
            renameFile(strOrigName, m_strChangedName);
        }
        catch (const CannotDeleteFile&)
        {
            revert();
            throw;
        }
        catch (const CannotRenameFile&)
        {
            revert();
            throw CannotDeleteFile();
        }
    }

    ~FileEraser()
    {
        if (m_strOrigName.empty() || m_strChangedName.empty()) { return; }

        try
        {
            renameFile(m_strChangedName, m_strOrigName);
        }
        catch (...)
        { //ttt2 perhaps do something
        }
    }

    void finalize()
    {
        if (m_strChangedName.empty()) { return; }

        deleteFile(m_strChangedName);
        m_strOrigName.clear();
        m_strChangedName.clear();
    }

private:
    void revert() // to be called on exceptions; assumes that the old file got copied but couldn't be deleted, so now the copy must be deleted
    {
        CB_ASSERT (!m_strChangedName.empty());
        try
        {
            deleteFile(m_strChangedName);
        }
        catch (const CannotDeleteFile&)
        { // nothing
        }
        m_strChangedName.clear();
    }
};


// on destructor erases the file given on constructor, unless release() was called; doesn't throw
class TempFileEraser
{
    string m_strName;
public:
    TempFileEraser(const string& strName) : m_strName(strName)
    {
    }

    ~TempFileEraser()
    {
        if (m_strName.empty()) { return; }

        try
        {
            deleteFile(m_strName);
        }
        catch (...)
        { //ttt2 perhaps do something
        }
    }

    void release()
    {
        m_strName.clear();
    }
};


void logTransformation(const string& strLogFile, const char* szActionName, const Mp3Handler* pHandler)
{
    ::logTransformation(strLogFile, szActionName, pHandler->getName());
}



bool Mp3TransformThread::transform()
{
    bool bAborted (false);

    bool bUseFastSave (m_pCommonData->useFastSave());
    for (int j = 0, m = cSize(m_vpTransf); j < m; ++j)
    {
        if (!m_vpTransf[j]->acceptsFastSave())
        {
            bUseFastSave = false;
            break;
        }
    }

    try
    {
        for (int i = 0, n = cSize(m_vpHndlr); i < n; ++i)
        {
            if (isAborted())
            {
                return false;
            }
            checkPause();

            const Mp3Handler* pOrigHndl (m_vpHndlr[i]);
            string strOrigName (pOrigHndl->getName());

            if (pOrigHndl->needsReload(bUseFastSave))
            {
                m_strErrorFile = strOrigName;
                m_bWriteError = false;
                m_bFileChanged = true;
                return false;
            }

            string strTempName;
            string strPrevTempName;
            StrList l;
            l.push_back(toNativeSeparators(convStr(strOrigName)));
            l.push_back("");
            //emit stepChanged(l);
            auto_ptr<const Mp3Handler> pNewHndl (pOrigHndl);


            try
            {
                long long nSize, nOrigTime;
                getFileInfo(strOrigName, nOrigTime, nSize);

                for (int j = 0, m = cSize(m_vpTransf); j < m; ++j)
                {
                    Transformation& t (*m_vpTransf[j]);
                    TRACER("Mp3TransformThread::transform()" + strOrigName + "/" + t.getActionName());
                    l[1] = t.getActionName();
                    emit stepChanged(l, i + 1);
                    Transformation::Result eTransf;
                    try
                    {
                        eTransf = t.apply(*pNewHndl, m_transfConfig, strOrigName, strTempName);
                    }
                    catch (const WriteError&)
                    {
        //qDebug("disk err");
                        m_strErrorFile = strTempName;
                        m_bWriteError = true;
                        if (pNewHndl.get() == pOrigHndl)
                        {
                            pNewHndl.release();
                        }
                        TempFileEraser er (strTempName);
                        return false; //ttt2 review what happens to pNewHndl
                    }
                    catch (const EndOfFile&) //ttt2 catch other exceptions, perhaps in the outer loop
                    {
                        m_strErrorFile = strOrigName;
                        m_bWriteError = false;
                        if (pNewHndl.get() == pOrigHndl)
                        {
                            pNewHndl.release();
                        }
                        TempFileEraser er (strTempName);
                        return false;
                    }

                    if (eTransf != Transformation::NOT_CHANGED)
                    {
                        CB_ASSERT (!m_pCommonData->m_strTransfLog.empty()); //ttt0 triggered according to http://sourceforge.net/apps/mantisbt/mp3diags/view.php?id=45 ; however, the code is quite simple and it doesn't seem to be a valid reason for m_strTransfLog to be empty (aside from corrupted memory)
                        if (m_pCommonData->m_bLogTransf)
                        {
                            logTransformation(m_pCommonData->m_strTransfLog, t.getActionName(), pNewHndl.get());
                        }

                        if (pNewHndl.get() == pOrigHndl)
                        {
                            pNewHndl.release();
                            CB_ASSERT (strPrevTempName.empty());
                        }
                        else
                        {
                            CB_ASSERT (!strPrevTempName.empty());
                            switch (m_transfConfig.getTempAction())
                            {
                            case TransfConfig::TRANSF_DONT_CREATE: deleteFile(strPrevTempName); break; //ttt2 try ... // or perhaps a try-catch for file errors for the whole block
                            case TransfConfig::TRANSF_CREATE: break;
                            default: CB_ASSERT (false);
                            }
                        }

                        strPrevTempName = strTempName;
                        pNewHndl.reset(new Mp3Handler(strTempName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds())); //ttt2 try..catch
                        checkPause();
                        if (isAborted())
                        {
                            bAborted = true;
                            break; // needed because it is possible that a bug in a transformation to make it create "transformations" that are equal to the original, so the loop never exits;
                            // not 100% right, but seems better anyway than just returning; // ttt3 fix: we should delete all the temp and comp files and return false, but for now it can stay as is;
                        }

                        if (Transformation::CHANGED_NO_RECALL != eTransf)
                        {
                            CB_ASSERT (Transformation::CHANGED == eTransf);
                            j = -1; // !!! start again with the first transformation
                            //ttt2 consider: 5 transforms, 1 and 2 are CHANGE_NO_RECALL, 3 is CHANGE, causing 1 and 2 to be called again; probably OK, but review
                        }
                    }
                }

                string strNewOrigName;  // new name for the orig file; if this is empty, the original file wasn't changed; if it's "*", it was erased; if it's something else, it was renamed;
                string strProcName;     // name for the proc file; if this is empty, a proc file doesn't exist; if it's something else, it's the file name;

                bool bErrorInTransform (false);

                FileEraser fileEraser;

                try
                {
                    //bool bChanged (true);
                    if (pNewHndl.get() == pOrigHndl)
                    { // nothing changed
                        pNewHndl.release();
                        switch (m_transfConfig.getUnprocOrigAction())
                        {
                        case TransfConfig::ORIG_DONT_CHANGE: break;
                        case TransfConfig::ORIG_ERASE: { strNewOrigName = "*"; fileEraser.erase(strOrigName); } break; //ttt2 try ...
                        case TransfConfig::ORIG_MOVE: { m_transfConfig.getUnprocOrigName(strOrigName, strNewOrigName); renameFile(strOrigName, strNewOrigName); } break;
                        default: CB_ASSERT (false);
                        }
                    }
                    else
                    { // at least a processed file exists
                        CB_ASSERT (!strTempName.empty());

                        TempFileEraser tmpEraser (strTempName);

                        // first we have to handle the original file;

                        switch (m_transfConfig.getProcOrigAction())
                        {
                        case TransfConfig::ORIG_DONT_CHANGE: break;
                        case TransfConfig::ORIG_ERASE: { strNewOrigName = "*"; fileEraser.erase(strOrigName); } break; //ttt2 try ...
                        case TransfConfig::ORIG_MOVE: { m_transfConfig.getProcOrigName(strOrigName, strNewOrigName); renameFile(strOrigName, strNewOrigName); } break;
                        case TransfConfig::ORIG_MOVE_OR_ERASE:
                            {
                                m_transfConfig.getProcOrigName(strOrigName, strNewOrigName);
                                if (fileExists(strNewOrigName))
                                {
                                    strNewOrigName = "*";
                                    fileEraser.erase(strOrigName);
                                }
                                else
                                {
                                    renameFile(strOrigName, strNewOrigName);
                                }
                            }
                            break;
                        default: CB_ASSERT (false);
                        }

                        // the last processed file exists (usualy in the same folder as the source), its name is in strTempName, and we have to see what to do with it (erase, rename, or copy);
                        switch (m_transfConfig.getProcessedAction())
                        {
                        case TransfConfig::TRANSF_DONT_CREATE: deleteFile(strTempName); break;
                        case TransfConfig::TRANSF_CREATE:
                            {
                                m_transfConfig.getProcessedName(strOrigName, strProcName);
                                switch (m_transfConfig.getTempAction())
                                {
                                case TransfConfig::TRANSF_DONT_CREATE: renameFile(strTempName, strProcName); break;
                                case TransfConfig::TRANSF_CREATE: copyFile(strTempName, strProcName); break;
                                default: CB_ASSERT (false);
                                }
                            }
                            break;

                        default: CB_ASSERT (false);
                        }

                        tmpEraser.release();
                    }

                    fileEraser.finalize();
                }
                catch (const CannotDeleteFile&)
                {
                    bErrorInTransform = true;
                }
                catch (const CannotRenameFile&) //ttt2 perhaps also NameNotFound, AlreadyExists, ...
                {
                    bErrorInTransform = true;
                }
                catch (const CannotCopyFile&)
                {
                    CB_ASSERT(false);
                    //bErrorInTransform = true;
                }

                if (bErrorInTransform)
                {
                    if (!strProcName.empty())
                    {
                        try
                        {
                            deleteFile(strProcName);
                        }
                        catch (...)
                        { //ttt2 not sure what to do
                        }
                    }
                    m_strErrorFile = strOrigName;
                    m_bWriteError = false;
                    return false;
                }
                //ttt2 perhaps do something similar to strNewOrigName
                //ttt2 review the whole thing

                if (!strNewOrigName.empty())
                {
                    m_vpDel.push_back(pOrigHndl);
                    if ("*" != strNewOrigName && m_pCommonData->m_dirTreeEnum.isIncluded(strNewOrigName))
                    {
                        m_vpAdd.push_back(new Mp3Handler(strNewOrigName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds()));
                    }
                }

                if (!strProcName.empty())
                {
                    if (m_transfConfig.m_options.m_bKeepOrigTime)
                    {
                        setFileDate(strProcName, nOrigTime);
                    }

                    if (m_pCommonData->m_dirTreeEnum.isIncluded(strProcName))
                    {
                        m_vpAdd.push_back(new Mp3Handler(strProcName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds())); // !!! a new Mp3Handler is needed, because pNewHndl has an incorrect file name (but otherwise they should be identical)
                    }
                }
            }
            catch (...)
            {
                if (pNewHndl.get() == pOrigHndl)
                {
                    pNewHndl.release();
                }
                throw;
            }
        }
    }
    catch (...)
    {
        qDebug("Caught unknown excption in Mp3TransformThread::transform()");
        traceToFile("Caught unknown excption in Mp3TransformThread::transform()", 0);
        throw; // !!! needed to restore "erased" files when errors occur, because when an exception is thrown the destructors only get called if that exception is caught; so catching and rethrowing is not a "no-op"
    }

    return !bAborted;
}
//ttt2 try to avoid rescanning the last file in a transform when intermediaries are removed;

} // namespace




// if there are errors the user is notified;
// returns true iff there were no errors;
bool transform(const deque<const Mp3Handler*>& vpHndlr, vector<Transformation*>& vpTransf, const string& strTitle, QWidget* pParent, CommonData* pCommonData, TransfConfig& transfConfig)
{
    vector<const Mp3Handler*> vpDel;
    vector<const Mp3Handler*> vpAdd;

    string strErrorFile;
    bool bWriteError;
    bool bFileChanged;
    {
        Mp3TransformThread* pThread (new Mp3TransformThread(pCommonData, transfConfig, vpHndlr, vpDel, vpAdd, vpTransf));
        ThreadRunnerDlgImpl dlg (pParent, getNoResizeWndFlags(), pThread, ThreadRunnerDlgImpl::SHOW_COUNTER, ThreadRunnerDlgImpl::TRUNCATE_BEGIN);
        dlg.setWindowTitle(convStr(strTitle));
        dlg.exec();
        strErrorFile = pThread->m_strErrorFile;
        bWriteError = pThread->m_bWriteError;
        bFileChanged = pThread->m_bFileChanged;
    }


    //if (dlg.exec()) // !!! even if the dialog was aborted, some files might have been changed already, and this is not undoable
    {
        pCommonData->mergeHandlerChanges(vpAdd, vpDel, CommonData::SEL | CommonData::CURRENT);
    }

    if (!strErrorFile.empty())
    {
        if (bWriteError)
        {
            QMessageBox::critical(pParent, "Error", "There was an error writing to the following file:\n\n" + toNativeSeparators(convStr(strErrorFile)) + "\n\nMake sure that you have write permissions and that there is enough space on the disk.\n\nProcessing aborted.");
        }
        else
        {
            if (bFileChanged)
            {
                QMessageBox::critical(pParent, "Error", "The file \"" + toNativeSeparators(convStr(strErrorFile)) + "\" seems to have been modified since the last scan. You need to rescan it before continuing.\n\nProcessing aborted.");
            }
            else
            {
                QMessageBox::critical(pParent, "Error", "There was an error processing the following file:\n\n" + toNativeSeparators(convStr(strErrorFile)) + "\n\nProbably the file was deleted or modified since the last scan, in which case you should reload / rescan your collection. Or it may be used by another program; if that's the case, you should stop the other program first.\n\nThis may also be caused by access restrictions or a full disk.\n\nProcessing aborted.");
            }
        }
    }

    return strErrorFile.empty();
}


//ttt1 This is a new crash from when the destination folder was full while saving modifications, in particular adding an album image.

