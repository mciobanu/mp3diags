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
#include  "Widgets.h"


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


struct Mp3TransformThread;

struct Mp3TransformerGui : public Mp3Transformer
{
    Mp3TransformThread* m_pMp3TransformThread;

    Mp3TransformerGui(
        CommonData* pCommonData,
        const TransfConfig& transfConfig,
        const deque<const Mp3Handler*>& vpHndlr,
        vector<const Mp3Handler*>& vpDel,
        vector<const Mp3Handler*>& vpAdd,
        vector<Transformation*>& vpTransf,
        Mp3TransformThread* pMp3TransformThread) :

        Mp3Transformer(
            pCommonData,
            transfConfig,
            vpHndlr,
            vpDel,
            vpAdd,
            vpTransf,
            0),
        m_pMp3TransformThread(pMp3TransformThread)
    {
    }

    /*override*/ bool isAborted();
    /*override*/ void checkPause();
    /*override*/ void emitStepChanged(const StrList& v, int nStep);
};


struct Mp3TransformThread : public PausableThread
{
    Mp3TransformerGui m_mp3TransformerGui;

    Mp3TransformThread(
        CommonData* pCommonData,
        const TransfConfig& transfConfig,
        const deque<const Mp3Handler*>& vpHndlr,
        vector<const Mp3Handler*>& vpDel,
        vector<const Mp3Handler*>& vpAdd,
        vector<Transformation*>& vpTransf) :

        m_mp3TransformerGui(
            pCommonData,
            transfConfig,
            vpHndlr,
            vpDel,
            vpAdd,
            vpTransf,
            this)
    {
    }

    /*override*/ void run()
    {
        try
        {
            CompleteNotif notif(this);

            notif.setSuccess(m_mp3TransformerGui.transform());
        }
        catch (...)
        {
            LAST_STEP("Mp3TransformThread::run()");
            CB_ASSERT (false);
        }
    }

    using PausableThread::emitStepChanged;
};


/*override*/ bool Mp3TransformerGui::isAborted()
{
    return m_pMp3TransformThread->isAborted();
}

/*override*/ void Mp3TransformerGui::checkPause()
{
    m_pMp3TransformThread->checkPause();
}

/*override*/ void Mp3TransformerGui::emitStepChanged(const StrList& v, int nStep)
{
    //emit m_pMp3TransformThread->stepChanged(v, nStep);
    m_pMp3TransformThread->emitStepChanged(v, nStep);
}


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


} // namespace




bool Mp3Transformer::transform()
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
            //TRACER1A("transf ", 1);
            if (isAborted())
            {
                return false;
            }
            checkPause();
            //TRACER1A("transf ", 2);

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

            //TRACER1A("transf ", 3);


            try
            {
                long long nSize, nOrigTime;
                getFileInfo(strOrigName, nOrigTime, nSize);

                for (int j = 0, m = cSize(m_vpTransf); j < m; ++j)
                {
                    //TRACER1A("transf ", 4);
                    Transformation& t (*m_vpTransf[j]);
                    TRACER("Mp3TransformThread::transform()" + strOrigName + "/" + t.getActionName());
                    l[1] = t.getVisibleActionName();
                    //emit stepChanged(l, i + 1);
                    emitStepChanged(l, i + 1);
                    Transformation::Result eTransf;
                    int nRetryCount (0);
                e1:
                    try
                    {
                    //TRACER1A("transf ", 5);
                        eTransf = t.apply(*pNewHndl, m_transfConfig, strOrigName, strTempName);
                        //TRACER1A("transf ", 6);
                        if (nRetryCount > 0)
                        {
                            TRACER("YYYYYYYYYYYYY Write retry succeeded for the second time to " + strTempName + ". Exiting to capture this ...");
                            throw WriteError(); //ttt0 remove
                        }
                    }
                    catch (const WriteError&)
                    {
        //qDebug("disk err");
        //TRACER1A("transf ", 7);
                        TRACER1("YYYYYYYYYYYYY Error copying from file " + strOrigName, 1);
                        TRACER1("YYYYYYYYYYYYY Error writing to file " + strTempName, 3);

                        //TRACER1A("transf ", 9);
                        TempFileEraser er (strTempName);
                        //TRACER1A("transf ", 10);
                        if (nRetryCount < 1)
                        {
                            TRACER("Retrying writing to file " + strTempName);
                            ++nRetryCount;
                            PausableThread::msleep(30);
                            strTempName.clear();
                            goto e1;
                        }
                        else
                        {
                            m_strErrorFile = strTempName;
                            m_bWriteError = true;
                            if (pNewHndl.get() == pOrigHndl)
                            {
                            //TRACER1A("transf ", 8);
                                pNewHndl.release();
                            }
                            TRACER1("Too many errors trying to write to file " + strTempName + ". Aborting ...", 2);
                            return false; //ttt2 review what happens to pNewHndl
                        }
                    }
                    catch (const EndOfFile&) //ttt2 catch other exceptions, perhaps in the outer loop
                    {
                    //TRACER1A("transf ", 11);
                        m_strErrorFile = strOrigName;
                        m_bWriteError = false;
                        if (pNewHndl.get() == pOrigHndl)
                        {
                        //TRACER1A("transf ", 12);
                            pNewHndl.release();
                        }
                        TempFileEraser er (strTempName);
                        //TRACER1A("transf ", 13);
                        return false;
                    }
//TRACER1A("transf ", 14);
                    //cout << "trying to apply " << t.getActionName() << " to " << pNewHndl.get()->getName() << endl;
                    if (eTransf != Transformation::NOT_CHANGED)
                    {
                    //TRACER1A("transf ", 15);
                        CB_ASSERT (!m_pCommonData->m_strTransfLog.empty()); //ttt0 triggered according to http://sourceforge.net/apps/mantisbt/mp3diags/view.php?id=45 ; however, the code is quite simple and it doesn't seem to be a valid reason for m_strTransfLog to be empty (aside from corrupted memory) ; according to https://sourceforge.net/apps/mantisbt/mp3diags/view.php?id=49 removing the .dat file solved the issue
                        if (0 != m_pLog)
                        {
                            (*m_pLog) << "applied " << t.getActionName() << " to " << pNewHndl.get()->getName() << endl;
                        }
                        if (m_pCommonData->m_bLogTransf)
                        {
                            logTransformation(m_pCommonData->m_strTransfLog, t.getActionName(), pNewHndl.get());
                        }
                        //TRACER1A("transf ", 16);

                        if (pNewHndl.get() == pOrigHndl)
                        {
                        //TRACER1A("transf ", 17);
                            pNewHndl.release();
                            CB_ASSERT (strPrevTempName.empty());
                            //TRACER1A("transf ", 18);
                        }
                        else
                        {
                        //TRACER1A("transf ", 19);
                            CB_ASSERT (!strPrevTempName.empty());
                            switch (m_transfConfig.getTempAction())
                            {
                            case TransfConfig::TRANSF_DONT_CREATE: deleteFile(strPrevTempName); break; //ttt2 try ... // or perhaps a try-catch for file errors for the whole block
                            case TransfConfig::TRANSF_CREATE: break;
                            default: CB_ASSERT (false);
                            }
                            //TRACER1A("transf ", 20);
                        }
//TRACER1A("transf ", 21);
                        strPrevTempName = strTempName;
                        pNewHndl.reset(new Mp3Handler(strTempName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds())); //ttt2 try..catch
                        checkPause();
                        //TRACER1A("transf ", 22);
                        if (isAborted())
                        {
                        //TRACER1A("transf ", 23);
                            bAborted = true;
                            break; // needed because it is possible that a bug in a transformation to make it create "transformations" that are equal to the original, so the loop never exits;
                            // not 100% right, but seems better anyway than just returning; // ttt3 fix: we should delete all the temp and comp files and return false, but for now it can stay as is;
                        }
                        //TRACER1A("transf ", 24);

                        if (Transformation::CHANGED_NO_RECALL != eTransf)
                        {
                        //TRACER1A("transf ", 25);
                            CB_ASSERT (Transformation::CHANGED == eTransf);
                            j = -1; // !!! start again with the first transformation
                            //ttt2 consider: 5 transforms, 1 and 2 are CHANGE_NO_RECALL, 3 is CHANGE, causing 1 and 2 to be called again; probably OK, but review
                        }
                        //TRACER1A("transf ", 26);
                    }
                    //TRACER1A("transf ", 27);
                }
                //TRACER1A("transf ", 28);

                string strNewOrigName;  // new name for the orig file; if this is empty, the original file wasn't changed; if it's "*", it was erased; if it's something else, it was renamed;
                string strProcName;     // name for the proc file; if this is empty, a proc file doesn't exist; if it's something else, it's the file name;

                bool bErrorInTransform (false);

                FileEraser fileEraser;
                //TRACER1A("transf ", 29);

                try
                {
                    //bool bChanged (true);
                    if (pNewHndl.get() == pOrigHndl)
                    { // nothing changed
                    //TRACER1A("transf ", 30);
                        pNewHndl.release();
                        //TRACER1A("transf ", 31);
                        switch (m_transfConfig.getUnprocOrigAction())
                        {
                        case TransfConfig::ORIG_DONT_CHANGE: break;
                        case TransfConfig::ORIG_ERASE: { strNewOrigName = "*"; fileEraser.erase(strOrigName); } break; //ttt2 try ...
                        case TransfConfig::ORIG_MOVE: { m_transfConfig.getUnprocOrigName(strOrigName, strNewOrigName); renameFile(strOrigName, strNewOrigName); } break;
                        default: CB_ASSERT (false);
                        }
                        //TRACER1A("transf ", 32);
                    }
                    else
                    { // at least a processed file exists
                    //TRACER1A("transf ", 33);
                        CB_ASSERT (!strTempName.empty());
//TRACER1A("transf ", 34);
                        TempFileEraser tmpEraser (strTempName);

                        // first we have to handle the original file;
//TRACER1A("transf ", 35);
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
//TRACER1A("transf ", 36);
                        // the last processed file exists (usualy in the same folder as the source), its name is in strTempName, and we have to see what to do with it (erase, rename, or copy);
                        switch (m_transfConfig.getProcessedAction())
                        {
                        case TransfConfig::TRANSF_DONT_CREATE: deleteFile(strTempName); break;
                        case TransfConfig::TRANSF_CREATE:
                            {//TRACER1A("transf ", 37);
                            //Tracer t1 (strOrigName);
                                m_transfConfig.getProcessedName(strOrigName, strProcName);
                                //Tracer t2 (strProcName);
                                //TRACER1A("transf ", 371);
                                switch (m_transfConfig.getTempAction())
                                {
                                case TransfConfig::TRANSF_DONT_CREATE: { /*TRACER1A("transf ", 372);*/ renameFile(strTempName, strProcName); /*TRACER1A("transf ", 373);*/ break; }
                                case TransfConfig::TRANSF_CREATE: { /*TRACER1A("transf ", 374);*/ copyFile(strTempName, strProcName); /*TRACER1A("transf ", 375);*/ break; }
                                default: { /*TRACER1A("transf ", 376);*/ CB_ASSERT (false); }
                                }
                                //TRACER1A("transf ", 377);
                            }
                            break;

                        default: CB_ASSERT (false);
                        }
                        //TRACER1A("transf ", 38);

                        tmpEraser.release();
                        //TRACER1A("transf ", 39);
                    }
//TRACER1A("transf ", 40);
                    fileEraser.finalize();
                    //TRACER1A("transf ", 41);
                }
                catch (const CannotDeleteFile&)
                {
                //TRACER1A("transf ", 42);
                    bErrorInTransform = true;
                }
                catch (const CannotRenameFile&) //ttt2 perhaps also NameNotFound, AlreadyExists, ...
                {
                //TRACER1A("transf ", 43);
                    bErrorInTransform = true;
                }
                catch (const CannotCreateDir& ex)
                {
                //TRACER1A("transf ", 44);
                    bErrorInTransform = true;
                    m_strErrorDir = ex.m_strDir;
                }
                catch (const CannotCopyFile&)
                {
                //TRACER1A("transf ", 45);
                    CB_ASSERT(false);
                    //bErrorInTransform = true;
                }
//TRACER1A("transf ", 46);
                if (bErrorInTransform)
                {
                //TRACER1A("transf ", 47);
                    if (!strProcName.empty())
                    {
                    //TRACER1A("transf ", 48);
                        try
                        {
                        //TRACER1A("transf ", 49);
                            deleteFile(strProcName);
                            //TRACER1A("transf ", 50);
                        }
                        catch (...)
                        { //ttt2 not sure what to do
                        }
                    }
                    m_strErrorFile = strOrigName;
                    m_bWriteError = false;
                    //TRACER1A("transf ", 51);
                    return false;
                }
                //ttt2 perhaps do something similar to strNewOrigName
                //ttt2 review the whole thing
                //TRACER1A("transf ", 52);

                if (!strNewOrigName.empty())
                {
                //TRACER1A("transf ", 53);
                    m_vpDel.push_back(pOrigHndl);
                    if ("*" != strNewOrigName && m_pCommonData->m_dirTreeEnum.isIncluded(strNewOrigName))
                    {
                    //TRACER1A("transf ", 54);
                        m_vpAdd.push_back(new Mp3Handler(strNewOrigName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds()));
                        //TRACER1A("transf ", 55);
                    }
                }

                if (!strProcName.empty())
                {
                //TRACER1A("transf ", 56);
                    if (m_transfConfig.m_options.m_bKeepOrigTime)
                    {
                    //TRACER1A("transf ", 57);
                        setFileDate(strProcName, nOrigTime);
                    }
                    //TRACER1A("transf ", 58);

                    if (m_pCommonData->m_dirTreeEnum.isIncluded(strProcName))
                    {
                    //TRACER1A("transf ", 59);
                        m_vpAdd.push_back(new Mp3Handler(strProcName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds())); // !!! a new Mp3Handler is needed, because pNewHndl has an incorrect file name (but otherwise they should be identical)
                        //TRACER1A("transf ", 60);
                    }
                    //TRACER1A("transf ", 61);
                }
                //TRACER1A("transf ", 62);
            }
            catch (...)
            {
            //TRACER1A("transf ", 63);
                if (pNewHndl.get() == pOrigHndl)
                {
                //TRACER1A("transf ", 64);
                    pNewHndl.release();
                    //TRACER1A("transf ", 65);
                }
                throw;
            }
        }
    }
    catch (...)
    {
    //TRACER1A("transf ", 66);
        qDebug("Caught unknown exception in Mp3TransformThread::transform()");
        traceToFile("Caught unknown exception in Mp3TransformThread::transform()", 0);
        throw; // !!! needed to restore "erased" files when errors occur, because when an exception is thrown the destructors only get called if that exception is caught; so catching and rethrowing is not a "no-op"
    }

    return !bAborted;
}
//ttt2 try to avoid rescanning the last file in a transform when intermediaries are removed;




// if there are errors the user is notified;
// returns true iff there were no errors;
bool transform(const deque<const Mp3Handler*>& vpHndlr, vector<Transformation*>& vpTransf, const string& strTitle, QWidget* pParent, CommonData* pCommonData, TransfConfig& transfConfig)
{
    vector<const Mp3Handler*> vpDel;
    vector<const Mp3Handler*> vpAdd;

    string strError;

    {
        Mp3TransformThread* pThread (new Mp3TransformThread(pCommonData, transfConfig, vpHndlr, vpDel, vpAdd, vpTransf));
        ThreadRunnerDlgImpl dlg (pParent, getNoResizeWndFlags(), pThread, ThreadRunnerDlgImpl::SHOW_COUNTER, ThreadRunnerDlgImpl::TRUNCATE_BEGIN);
        dlg.setWindowTitle(convStr(strTitle));
        dlg.exec();
        strError = pThread->m_mp3TransformerGui.getError();
    }


    //if (dlg.exec()) // !!! even if the dialog was aborted, some files might have been changed already, and this is not undoable
    {
        pCommonData->mergeHandlerChanges(vpAdd, vpDel, CommonData::SEL | CommonData::CURRENT);
    }

    if (!strError.empty())
    {
        showCritical(pParent, Mp3Transformer::tr("Error"), convStr(strError));
    }

    return strError.empty();
}

std::string Mp3Transformer::getError() const
{
    if (!m_strErrorFile.empty())
    {
        if (m_bWriteError)
        {
            return convStr(tr("There was an error writing to the following file:\n\n%1\n\nMake sure that you have write permissions and that there is enough space on the disk.\n\nProcessing aborted.").arg(convStr(toNativeSeparators(m_strErrorFile))));
        }
        else
        {
            if (m_bFileChanged)
            {
                return convStr(tr("The file \"%1\" seems to have been modified since the last scan. You need to rescan it before continuing.\n\nProcessing aborted.").arg(convStr(toNativeSeparators(m_strErrorFile))));
            }
            else
            {
                if (m_strErrorDir.empty())
                {
                    return convStr(tr("There was an error processing the following file:\n\n%1\n\nProbably the file was deleted or modified since the last scan, in which case you should reload / rescan your collection. Or it may be used by another program; if that's the case, you should stop the other program first.\n\nThis may also be caused by access restrictions or a full disk.\n\nProcessing aborted.").arg(convStr(toNativeSeparators(m_strErrorFile))));
                }
                else
                {
                    return convStr(tr("There was an error processing the following file:\n%1\n\nThe following folder couldn't be created:\n\n%2\n\nProcessing aborted.").arg(convStr(toNativeSeparators(m_strErrorFile))).arg(convStr(toNativeSeparators(m_strErrorDir))));
                }
            }
        }
    }

    return "";
}


//ttt1 This is a new crash from when the destination folder was full while saving modifications, in particular adding an album image.

/*
17:47:09.836 > TagEditorDlgImpl::run()
17:47:58.031  > Mp3TransformThread::transform()F:/TMP/MP3/MP3DiagsTest/Testfile.mp3/Save ID3V2.3.0 tags
17:47:58.183   > Mp3Handler constr: F:/TMP/MP3/MP3DiagsTest/Testfile.mp3.EQgf73
17:47:58.420   < Mp3Handler constr: F:/TMP/MP3/MP3DiagsTest/Testfile.mp3.EQgf73
17:47:58.422  < Mp3TransformThread::transform()F:/TMP/MP3/MP3DiagsTest/Testfile.mp3/Save ID3V2.3.0 tags
17:47:58.433  > Mp3Handler destr: F:/TMP/MP3/MP3DiagsTest/Testfile.mp3.EQgf73
17:47:58.435  < Mp3Handler destr: F:/TMP/MP3/MP3DiagsTest/Testfile.mp3.EQgf73
17:47:58.435 Caught unknown exception in Mp3TransformThread::transform()
17:47:58.443 Assertion failure in file Mp3TransformThread.cpp, line 98: false. The program will exit.
*/





