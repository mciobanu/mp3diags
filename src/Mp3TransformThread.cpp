/***************************************************************************
 *   MP3 Insight - diagnosis, repairs and tag editing for MP3 files        *
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
        m_bWriteError(true)
    {
    }

    string m_strErrorFile; // normally this is empty; if it's not, writing to the specified file failed
    bool m_bWriteError;

    /*override*/ void run()
    {
        CompleteNotif notif(this);

        notif.setSuccess(transform());
    }

    bool transform();
};


bool Mp3TransformThread::transform()
{
    bool bAborted (false);

    for (int i = 0, n = cSize(m_vpHndlr); i < n; ++i)
    {
        if (isAborted())
        {
            return false;
        }
        checkPause();

        const Mp3Handler* pOrigHndl (m_vpHndlr[i]);
        string strOrigName (pOrigHndl->getName());
        string strTempName;
        string strPrevTempName;
        StrList l;
        l.push_back(convStr(strOrigName));
        emit stepChanged(l);
        auto_ptr<const Mp3Handler> pNewHndl (pOrigHndl);

        long long nSize, nOrigTime;
        getFileInfo(strOrigName.c_str(), nOrigTime, nSize);

        for (int j = 0, m = cSize(m_vpTransf); j < m; ++j)
        {
            Transformation& t (*m_vpTransf[j]);
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
                return false;
            }

            if (eTransf != Transformation::NOT_CHANGED)
            {
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
                pNewHndl.reset(new Mp3Handler(strTempName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds())); //ttt1 try..catch
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

        //bool bChanged (true);
        if (pNewHndl.get() == pOrigHndl)
        { // nothing changed
            pNewHndl.release();
            switch (m_transfConfig.getUnprocOrigAction())
            {
            case TransfConfig::ORIG_DONT_CHANGE: break;
            case TransfConfig::ORIG_ERASE: { strNewOrigName = "*"; deleteFile(strOrigName); } break; //ttt2 try ...
            case TransfConfig::ORIG_MOVE: { m_transfConfig.getUnprocOrigName(strOrigName, strNewOrigName); renameFile(strOrigName, strNewOrigName); } break;
            default: CB_ASSERT (false);
            }
        }
        else
        { // at least a processed file exists
            CB_ASSERT (!strTempName.empty());

            // first we have to handle the original file;

            switch (m_transfConfig.getProcOrigAction())
            {
            case TransfConfig::ORIG_DONT_CHANGE: break;
            case TransfConfig::ORIG_ERASE: { strNewOrigName = "*"; deleteFile(strOrigName); } break; //ttt2 try ...
            case TransfConfig::ORIG_MOVE: { m_transfConfig.getProcOrigName(strOrigName, strNewOrigName); renameFile(strOrigName, strNewOrigName); } break;
            case TransfConfig::ORIG_MOVE_OR_ERASE:
                {
                    m_transfConfig.getProcOrigName(strOrigName, strNewOrigName);
                    if (fileExists(strNewOrigName))
                    {
                        strNewOrigName = "*";
                        deleteFile(strOrigName);
                    }
                    else
                    {
                        renameFile(strOrigName, strNewOrigName);
                    }
                }
                break;
            default: CB_ASSERT (false);
            }

            // the last processed file exists in the "temp" folder, its name is in strTempName, and we have to see what to do with it (erase, rename, or copy);
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
        }

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
            if (m_transfConfig.m_optionsWrp.m_opt.m_bKeepOrigTime)
            {
                setFileDate(strProcName.c_str(), nOrigTime);
            }

            if (m_pCommonData->m_dirTreeEnum.isIncluded(strProcName))
            {
                m_vpAdd.push_back(new Mp3Handler(strProcName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds())); // !!! a new Mp3Handler is needed, because pNewHndl has an incorrect file name (but otherwise they should be identical)
            }
        }

    }

    return !bAborted;
}


} // namespace



// if there are errors the user is notified;
// returns true iff there were no errors;
bool transform(const deque<const Mp3Handler*>& vpHndlr, vector<Transformation*>& vpTransf, const string& strTitle, QWidget* pParent, CommonData* pCommonData, TransfConfig& transfConfig)
{
    vector<const Mp3Handler*> vpDel;
    vector<const Mp3Handler*> vpAdd;

    string strErrorFile;
    bool bWriteError;
    {
        Mp3TransformThread* pThread (new Mp3TransformThread(pCommonData, transfConfig, vpHndlr, vpDel, vpAdd, vpTransf));
        ThreadRunnerDlgImpl dlg (pParent, 0, pThread, ThreadRunnerDlgImpl::SHOW_COUNTER, ThreadRunnerDlgImpl::TRUNCATE_BEGIN);
        dlg.setWindowTitle(convStr(strTitle));
        dlg.exec();
        strErrorFile = pThread->m_strErrorFile;
        bWriteError = pThread->m_bWriteError;
    }


    //if (dlg.exec()) // !!! even if the dialog was aborted, some files might have been changed already, and this is not undoable
    {
        pCommonData->mergeHandlerChanges(vpAdd, vpDel, CommonData::SEL | CommonData::CURRENT);
    }

    if (!strErrorFile.empty())
    {
        if (bWriteError)
        {
            QMessageBox::critical(pParent, "Error", "There was an error writing to the following file:\n\n" + convStr(strErrorFile) + "\n\nMake sure that you have write permissions and that there is enough space on the disk.\n\nProcessing aborted.");
        }
        else
        {
            QMessageBox::critical(pParent, "Error", "There was an error reading from the following file:\n\n" + convStr(strErrorFile) + "\n\nProbably the file was deleted or modified since the last scan.\n\nProcessing aborted.");
        }
    }

    return strErrorFile.empty();
}



