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


#ifdef MSVC_QMAKE
    #pragma warning (disable : 4100)
#endif


#include  <algorithm>
#include  <sstream>
//#include  <iostream>

#include  <QFileDialog>
#include  <QKeyEvent>
#include  <QStackedLayout>
#include  <QHeaderView>
#include  <QTimer>
#include  <QDesktopWidget>
#include  <QToolTip>
#include  <QSettings>
#include  <QTime>
#include  <QHttp>
#include  <QHttpRequestHeader>
#include  <QHttpResponseHeader>
#include  <QDesktopServices>
#include  <QProcess>

#ifndef WIN32
    //#include <sys/utsname.h>
#else
    #include  <windows.h>
    #include  <QDateTime>
#endif

#include  "MainFormDlgImpl.h"

#include  "DirFilterDlgImpl.h"
#include  "NoteFilterDlgImpl.h"
#include  "Helpers.h"
#include  "OsFile.h"
#include  "FilesModel.h"
#include  "NotesModel.h"
#include  "StreamsModel.h"
#include  "UniqueNotesModel.h"
#include  "TagReadPanel.h"
#include  "ThreadRunnerDlgImpl.h"
#include  "ConfigDlgImpl.h"
#include  "AboutDlgImpl.h"
#include  "Widgets.h"
#include  "DataStream.h"
#include  "ExternalToolDlgImpl.h"
#include  "DebugDlgImpl.h"
#include  "TagEditorDlgImpl.h"
#include  "Mp3TransformThread.h"
#include  "FileRenamerDlgImpl.h"
#include  "ScanDlgImpl.h"
#include  "SessionEditorDlgImpl.h"
#include  "Id3Transf.h"
#include  "ExportDlgImpl.h"
#include  "Version.h"
#include  "Translation.h"


using namespace std;
using namespace pearl;
using namespace Version;


//#define LOG_ANYWAY

//ttt2 try to switch from QDialog to QWidget, to see if min/max in gnome show up; or add Qt::Dialog flag (didn't seem to work, though)

MainFormDlgImpl* getGlobalDlg();  //ttt2 review

void trace(const string& s)
{
    MainFormDlgImpl* p (getGlobalDlg());
    //p->m_pContentM->append(convStr(s));
    //p->m_pCommonData->m_qstrContent += convStr(s);
    //p->m_pCommonData->m_qstrContent += "\n";
    if (0 != p && 0 != p->m_pCommonData)
    {
        p->m_pCommonData->trace(s); //ttt if p->m_pCommonData==0 or p==0 use logToGlobalFile(); 2009.09.08 - better not: this is "trace"; it makes sense to log errors before p->m_pCommonData is set up, but this is not the place to do it
    }
}



namespace
{


#ifndef WIN32
    class NativeFile
    {
        string m_strFileName;
    public:
        NativeFile() {}
        bool open(const string& strFileName) // doesn't truncate the file
        {
            m_strFileName = strFileName;
            ofstream_utf8 out (m_strFileName.c_str(), ios_base::app);
            return out;
        }

        void close() { m_strFileName.clear(); }

        bool write(const string& s)
        {
            ofstream_utf8 out (m_strFileName.c_str(), ios_base::app);
            out << s;
            return out;
        }
    };
#else // #ifndef WIN32
    void CB_LIB_CALL displayOsErrorIfExists(const char* szTitle)
    {
        unsigned int nErr (GetLastError());
        if (0 == nErr) return;

        LPVOID lpMsgBuf;

        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            nErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPSTR) &lpMsgBuf,
            0,
            NULL
        );

        string strRes ((LPSTR)lpMsgBuf);

        // Free the buffer.
        LocalFree(lpMsgBuf);
        unsigned int nSize ((unsigned int)strRes.size());
        if (nSize - 2 == strRes.rfind("\r\n"))
        {
            strRes.resize(nSize - 2);
        }

        showCritical(0, szTitle, convStr(strRes));
    }


    class NativeFile
    {
        HANDLE m_hStepFile;
    public:
        NativeFile() : m_hStepFile (INVALID_HANDLE_VALUE) {}

        bool open(const string& strFileName)
        {
            m_hStepFile = CreateFileA(strFileName.c_str(),
                         GENERIC_WRITE,
                         FILE_SHARE_WRITE | FILE_SHARE_READ,
                         0,
                         OPEN_ALWAYS,
                         //CREATE_ALWAYS,
                         FILE_ATTRIBUTE_TEMPORARY,
                         //FILE_ATTRIBUTE_NORMAL,
                         0);
            if (INVALID_HANDLE_VALUE == m_hStepFile)
            {
                displayOsErrorIfExists("Error");
                return false;
            }

            SetFilePointer(m_hStepFile, 0, 0, FILE_END);
            return true;
        }

        void close()
        {
            CloseHandle(m_hStepFile);
            m_hStepFile = INVALID_HANDLE_VALUE;
        }

        bool write(const string& s)
        {
            DWORD dwWrt;
            //WriteFile(s_hStepFile, a, strlen(a), &dwWrt, 0);
            WriteFile(m_hStepFile, s.c_str(), s.size(), &dwWrt, 0);
            //WriteFile(s_hStepFile, "\r\n", 2, &dwWrt, 0);
            return (dwWrt == s.size());
        }
    };
#endif // #ifndef WIN32



    class FileTracer
    {
        string m_strTraceFile;
        vector<string> m_vstrStepFile;

        bool m_bEnabled1, m_bEnabled2; // both have to be true for the trace to actually be enabled; they are not symmetrical: changing m_bEnabled2 while m_bEnabled1 is true also removes the files, but doesn't happen the other way; the reason for doing this is that we want trace files from previous run deleted (regardless of how it finished) but we don't want them deleted too early, so the user has a chance to mail them; to complicate the things, there is loading the settings, which may call enable2(true);

        int m_nStepFile;
        int m_nCrtStepSize;
        int m_nPage;
        int m_nStepLevel;
        int m_nTraceLevel;

        NativeFile m_stepFile;
        void setupFiles();
        void removeFiles();

        NativeFile m_traceFile;

    public:
        FileTracer() : m_bEnabled1(false), m_bEnabled2(false), m_nStepFile(-1), m_nCrtStepSize(-1), m_nPage(-1), m_nStepLevel(-1), m_nTraceLevel(-1)
        {
        }

        //void setupTraceToFile(bool bEnable);
        void setName(const string& strNameRoot); // also disables
        void enable1(bool); // used as a master control, by MainFormDlgImpl and assert; if m_bEnabled1 is true, changing m_bEnabled2 triggers the removal of files
        void enable2(bool); // set based on config
        //void removeFilesIfDisabled();

        void traceToFile(const string& s, int nLevelChange);
        void traceLastStep(const string& s, int nLevelChange);

        const string& getTraceFile() const { return m_strTraceFile; }
        const vector<string>& getStepFiles() const { return m_vstrStepFile; }
    };


    void FileTracer::traceToFile(const string& s, int nLevelChange)
    {
        if (!m_bEnabled1 || !m_bEnabled2 || m_strTraceFile.empty()) { return; }

        static QMutex mutex;
        QMutexLocker lck (&mutex);

        m_nTraceLevel += nLevelChange;
        if (m_nTraceLevel < 0) { m_nTraceLevel = 20; }
        string s1 (m_nTraceLevel, ' '); // !!! may happen in this case: trace gets enabled by assert, then the Tracer destructor runs, trying to decrease what was never increased
        s1 += s;

        QTime t (QTime::currentTime());
        char a [15];
        sprintf(a, "%02d:%02d:%02d.%03d", t.hour(), t.minute(), t.second(), t.msec());

        //ofstream_utf8 out (m_strTraceFile.c_str(), ios_base::app);
        //out << a << s1 << endl;
#ifndef WIN32
        m_traceFile.write(a + s1 + "\n");
#else
        m_traceFile.write(a + s1 + "\r\n");
#endif
    }

    void FileTracer::traceLastStep(const string& s, int nLevelChange)
    {
        if (!m_bEnabled1 || !m_bEnabled2 || m_strTraceFile.empty()) { return; }

        static QMutex mutex;
        QMutexLocker lck (&mutex);

        m_nStepLevel += nLevelChange;
        if (m_nStepLevel < 0) { m_nStepLevel = 20; } // !!! may happen in this case: trace gets enabled by assert, then the LastStepTracer destructor runs, trying to decrease what was never increased
        string s1 (m_nStepLevel, ' ');
        //string s1; char b [20]; sprintf(b, "%d", s_nLevel); s1 = b;
        s1 += s;

        char a [15];
        a[0] = 0;
        //QTime t (QTime::currentTime()); sprintf(a, "%02d:%02d:%02d.%03d ", t.hour(), t.minute(), t.second(), t.msec());

#ifndef WIN32
        m_stepFile.write(a + s1 + "\n");
        m_nCrtStepSize += s1.size() + 1;
#else
        m_stepFile.write(a + s1 + "\r\n");
        m_nCrtStepSize += s1.size() + 2;
#endif


        if (m_nCrtStepSize > 50000)
        {
            m_stepFile.close();

            m_nCrtStepSize = 0;
            m_nStepFile = 1 - m_nStepFile;
            try
            {
                deleteFile(m_vstrStepFile[m_nStepFile]);
            }
            catch (...)
            { //ttt2
            }

            {
                ofstream_utf8 out (m_vstrStepFile[m_nStepFile].c_str(), ios_base::app);
                out << "page " << ++m_nPage << endl;
            }

            m_stepFile.open(m_vstrStepFile[m_nStepFile]);
        }
    }



    void FileTracer::setName(const string& strNameRoot)
    {
        m_stepFile.close();
        m_traceFile.close();
        m_bEnabled1 = m_bEnabled2 = false;

        CB_ASSERT (!strNameRoot.empty());

        m_strTraceFile = strNameRoot + "_trace.txt";

        m_vstrStepFile.clear();
        m_vstrStepFile.push_back(strNameRoot + "_step1.txt");
        m_vstrStepFile.push_back(strNameRoot + "_step2.txt");
    }

    /*void FileTracer::removeFilesIfDisabled()
    {
        if (!m_bEnabled1 || !m_bEnabled2)
        {
            removeFiles();
        }
    }*/

    void FileTracer::removeFiles()
    {
        m_nStepFile = 0;
        m_nCrtStepSize = 0;
        m_nPage = 0;
        m_nStepLevel = 0;
        m_nTraceLevel = 0;

        try
        {
            deleteFile(m_strTraceFile);
            deleteFile(m_vstrStepFile[0]);
            deleteFile(m_vstrStepFile[1]);
        }
        catch (...)
        { //ttt2
        }
    }

    void FileTracer::setupFiles()
    {
        if (!m_bEnabled1 || !m_bEnabled2)
        {
            if (m_bEnabled1)
            {
                removeFiles();
            }
            return;
        }

        removeFiles();

        m_stepFile.open(m_vstrStepFile[0]);
        m_traceFile.open(m_strTraceFile);

        traceToFile(convStr(getSystemInfo()), 0);
        traceLastStep(convStr(getSystemInfo()), 0);
    }

    void FileTracer::enable1(bool b)
    {
        if (b == m_bEnabled1 || m_vstrStepFile.empty()) { return; }
        m_bEnabled1 = b;
        setupFiles();
    }

    void FileTracer::enable2(bool b)
    {
        if (b == m_bEnabled2 || m_vstrStepFile.empty()) { return; }
        m_bEnabled2 = b;
        setupFiles();
    }

    FileTracer s_fileTracer;
}



void traceToFile(const string& s, int nLevelChange)
{
    s_fileTracer.traceToFile(s, nLevelChange);
}

void setupTraceToFile(bool b)
{
    s_fileTracer.enable2(b);
}



void traceLastStep(const string& s, int nLevelChange)
{
    s_fileTracer.traceLastStep(s, nLevelChange);
}



//ttt2 add checkbox to uninst on wnd to remove data and settings; //ttt2 uninst should remove log file as well, including the file created when "debug/log transf" is turned on
//static QString s_strAssertTitle ("Assertion failure");
//static QString s_strCrashWarnTitle ("Crash detected");
static QString s_qstrErrorMsg;
static bool s_bMainAssertOut;

static QString g_qstrCrashMail ("<a href=\"mailto:mp3diags@gmail.com?subject=000 MP3 Diags crash/\">mp3diags@gmail.com</a>");
static QString g_qstrSupportMail ("<a href=\"mailto:mp3diags@gmail.com?subject=000 MP3 Diags support note/\">mp3diags@gmail.com</a>");
static QString g_qstrBugReport ("<a href=\"http://sourceforge.net/apps/mantisbt/mp3diags/\">http://sourceforge.net/apps/mantisbt/mp3diags/</a>");


static void showAssertMsg(QWidget* pParent)
{
    HtmlMsg::msg(pParent, 0, 0, 0, HtmlMsg::SHOW_SYS_INFO, MainFormDlgImpl::tr("Assertion failure"),
        Qt::escape(s_qstrErrorMsg) +
            "<p style=\"margin-bottom:1px; margin-top:12px; \">" +
            (
                s_fileTracer.getStepFiles().empty() ?
                    MainFormDlgImpl::tr("Plese report this problem to the project's Issue Tracker at %1").arg(g_qstrBugReport) :
                    MainFormDlgImpl::tr("Please restart the application for instructions about how to report this issue")
            ) +
            "</p>",
        750, 300, MainFormDlgImpl::tr("Exit"));
}



void MainFormDlgImpl::showRestartAfterCrashMsg(const QString& qstrText, const QString& qstrCloseBtn)
{
    HtmlMsg::msg(this, 0, 0, 0, HtmlMsg::SHOW_SYS_INFO, tr("Restarting after crash"), qstrText, 750, 450, qstrCloseBtn);
}


void logAssert(const char* szFile, int nLine, const char* szCond)
{
    //showCritical(0, "Assertion failure", QString("Assertion failure in file %1, line %2: %3").arg(szFile).arg(nLine).arg(szCond), QMessageBox::Close);
    /*QMessageBox dlg (QMessageBox::Critical, "Assertion failure", QString("Assertion failure in file %1, line %2: %3").arg(szFile).arg(nLine).arg(szCond), QMessageBox::Close, getThreadLocalDlgList().getDlg(), Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);*/

    s_qstrErrorMsg = QString("Assertion failure in file %1, line %2: %3. The program will exit.").arg(szFile).arg(nLine).arg(szCond);
    s_fileTracer.enable1(true);
    s_fileTracer.enable2(true);
    traceToFile(convStr(s_qstrErrorMsg), 0);
    qDebug("Assertion failure in file %s, line %d: %s", szFile, nLine, szCond);
    s_fileTracer.enable1(false); // !!! to avoid logging irrelevant info about cells drawn after the message was shown (there is some risk of not detecting that messages aren't shown although they should be, but this has never been reported, while trace files full of FilesModel::headerData and the like are common)

    MainFormDlgImpl* p (getGlobalDlg());

    if (0 != p)
    {
        s_bMainAssertOut = false;
        //QTimer::singleShot(1, p, SLOT(onShowAssert())); //ttt2 see why this doesn't work
        AssertSender s (p);

        for (;;)
        {
            if (s_bMainAssertOut) { break; }
            //sleep(1);
#ifndef WIN32
            timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 100000000; // 0.1s
            nanosleep(&ts, 0);
#else
            Sleep(100);
#endif
        }
    }
    else
    {
        /*QMessageBox dlg (QMessageBox::Critical, s_strAssertTitle, s_strErrorMsg, QMessageBox::Close, 0, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint); // ttt2 this might fail / crash, as it may be called from a secondary thread

        dlg.exec();*/
        showAssertMsg(0);
    }
}

void logAssert(const char* szFile, int nLine, const char* szCond, const std::string& strAddtlInfo)
{
    string s (string(szCond) + "; additional info: " + strAddtlInfo);
    logAssert(szFile, nLine, s.c_str());
}


void MainFormDlgImpl::onShowAssert()
{
    /*QMessageBox dlg (QMessageBox::Critical, s_strAssertTitle, s_strErrorMsg, QMessageBox::Close, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);

    dlg.exec();*/
    showAssertMsg(this);

    s_bMainAssertOut = true;
}



// resizes a dialog with inexisting/invalid size settings, so it covers an area slightly smaller than MainWnd; however, if the dialog is alrady bigger than that, it doesn't get shrinked
void defaultResize(QDialog& dlg)
{
    QSize s (dlg.size());
    QDialog& mainDlg (*getGlobalDlg());
    s.rwidth() = max(s.rwidth(), mainDlg.width() - 100); //ttt2  doesn't do what it should for the case when working with small fonts and small resolutions
    s.rheight() = max(s.rheight(), mainDlg.height() - 100);
    dlg.resize(s.width(), s.height());
}



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================





void MainFormDlgImpl::showBackupWarn()
{
    if (m_pCommonData->m_bWarnedAboutBackup) { return; }

    HtmlMsg::msg(this, 0, 0, &m_pCommonData->m_bWarnedAboutBackup, HtmlMsg::CRITICAL, tr("Warning"), "<p>" + tr("Because MP3 Diags changes the content of your MP3 files if asked to, it has a significant destructive potential, especially in cases where the user doesn't read the documentation and simply expects the program to do other things than what it was designed to do.") + "</p><p>" + tr("Therefore, it is highly advisable to back up your files first.") + "</p><p>" + tr("Also, although MP3 Diags is very stable on the developer's computer, who hasn't experienced a crash in a long time and never needed to restore MP3 files from a backup, there are several crash reports that haven't been addressed, as the developer couldn't reproduce the crashes and those who reported the crashes didn't answer the developer's questions that might have helped isolate the problem.") + "</p>", 520, 300, tr("O&K"));

    if (!m_pCommonData->m_bWarnedAboutBackup) { return; }

    m_settings.saveMiscConfigSettings(m_pCommonData);
}



void MainFormDlgImpl::showSelWarn()
{
    if (m_pCommonData->m_bWarnedAboutSel) { return; }

    HtmlMsg::msg(this, 0, 0, &m_pCommonData->m_bWarnedAboutSel, HtmlMsg::DEFAULT, tr("Note"), tr("If you simply left-click, all the visible files get processed. However, it is possible to process only the selected files. To do that, either keep SHIFT pressed down while clicking or use the right button, as described at %1").arg("<a href=\"http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/140_main_window_tools.html\">http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/140_main_window_tools.html</a>"), 520, 300, tr("O&K"));

    if (!m_pCommonData->m_bWarnedAboutSel) { return; }

    m_settings.saveMiscConfigSettings(m_pCommonData);
}




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

void MainFormDlgImpl::saveIgnored()
{
    const vector<int>& v (m_pCommonData->getIgnoredNotes());

    vector<string> u;
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        u.push_back(Notes::getNote(v[i])->getDescription());
    }

    m_settings.saveVector("ignored/list", u);
}


void MainFormDlgImpl::loadIgnored()
{
    bool bRes (true); //ttt2 ? use
    vector<string> v (m_settings.loadVector("ignored/list", bRes));

    vector<int> vnIgnored;
    vector<string> vstrNotFoundNotes;

    int n (cSize(v));
    if (0 == n)
    { // use default
        vnIgnored = Notes::getDefaultIgnoredNoteIds();
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            const string& strDescr (v[i]);
            //qDebug("%s", strDescr.c_str());
            const Note* pNote (Notes::getNote(strDescr));
            if (0 == pNote)
            {
                vstrNotFoundNotes.push_back(strDescr);
            }
            else
            {
                vnIgnored.push_back(pNote->getNoteId());
            }
        }
    }

    {
        m_pCommonData->setIgnoredNotes(vnIgnored);
        int n (cSize(vstrNotFoundNotes));
        if (n > 0)
        {
            QString s;
            if (1 == n)
            {
                s = tr("An unknown note was found in the configuration. This note is unknown:\n\n%1").arg(convStr(vstrNotFoundNotes[0]));
            }
            else
            {
                QString s1;
                for (int i = 0; i < n; ++i)
                {
                    s1 += convStr(vstrNotFoundNotes[i]);
                    if (i < n - 1)
                    {
                        s1 += "\n";
                    }
                }
                s = tr("Unknown notes were found in the configuration. These notes are unknown:\n\n%1").arg(s1);
            }

            showWarning(this, tr("Error setting up the \"ignored notes\" list"), s + tr("\n\nYou may want to check again the list and add any notes that you want to ignore.\n\n(If you didn't change the settings file manually, this is probably due to a code enhanement that makes some notes no longer needed, and you can safely ignore this message.)")); //ttt2 use MP3 Diags icon

            saveIgnored();
        }
    }
}


static bool s_bToldAboutSupportInCrtRun (false); // to limit to 1 per run the number of times the user is told about support

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


/*

Signals:

currentFileChanged() - sent by m_pCommonData->m_pFilesModel and received by both MainFormDlgImpl (to create a new m_pTagDetailsW) and CommonData (to update current notes and streams, which calls m_pStreamsModel->emitLayoutChanged() and m_pNotesModel->emitLayoutChanged())

filterChanged() - sent by m_pCommonData->m_filter and received by m_pCommonData; it updates m_pCommonData->m_vpFltHandlers and calls m_pCommonData->updateWidgets(); updateWidgets() calls m_pFilesModel->selectRow(), which triggers currentFileChanged(), which causes the note and stream grids to be updated


// ttt2 finish documenting the signal flow (mainly changing of "current" for both main window and tag editor); then check that it is properly implemented; pay attention to not calling signal handlers directly unless there's a very good reason to do so


*/


static MainFormDlgImpl* s_pGlobalDlg (0); //ttt2 review this

MainFormDlgImpl* getGlobalDlg()
{
    return s_pGlobalDlg;
}

QWidget* getMainForm()
{
    return getGlobalDlg();
}

static PausableThread* s_pSerThread;
PausableThread* getSerThread() //ttt2 global function
{
    return s_pSerThread;
}


namespace {

struct SerLoadThread : public PausableThread
{
    CommonData* m_pCommonData;
    const string& m_strSession;
    string& m_strErr;
    SerLoadThread(CommonData* pCommonData, const string& strSession, string& strErr) : m_pCommonData(pCommonData), m_strSession(strSession), m_strErr(strErr) {}

    /*override*/ void run()
    {
        try
        {
            CompleteNotif notif(this);

            bool bAborted (!load());

            notif.setSuccess(!bAborted);
        }
        catch (...)
        {
            LAST_STEP("SerLoadThread::run()");
            CB_ASSERT (false);
        }
    }

    bool load()
    {
        m_strErr = m_pCommonData->load(SessionEditorDlgImpl::getDataFileName(m_strSession));
        //m_pCommonData->m_strTransfLog = SessionEditorDlgImpl::getLogFileName(m_strSession);
        //{ TRACER("001 m_strTransfLog=" + m_pCommonData->m_strTransfLog);  }
        return true;
    }
};


struct SerSaveThread : public PausableThread
{
    CommonData* m_pCommonData;
    const string& m_strSession;
    string& m_strErr;
    SerSaveThread(CommonData* pCommonData, const string& strSession, string& strErr) : m_pCommonData(pCommonData), m_strSession(strSession), m_strErr(strErr) {}

    /*override*/ void run()
    {
        try
        {
            CompleteNotif notif(this);

            bool bAborted (!save());

            notif.setSuccess(!bAborted);
        }
        catch (...)
        {
            LAST_STEP("SerSaveThread::run()");
            CB_ASSERT (false);
        }
    }

    bool save()
    {
        m_strErr = m_pCommonData->save(SessionEditorDlgImpl::getDataFileName(m_strSession));
        return true;
    }
};

//ttt1 "unstable" in html for analytics

void listKnownFormats()
{
    for (unsigned i = 0x240; i < 1024; ++i) // everything below 0x240 is invalid
    {
        unsigned x (0xffe00000);
        x += (i & 0x300) << (19 - 8);
        x += (i & 0x0c0) << (17 - 6);

        x += (i & 0x03c) << (12 - 2);
        x += (i & 0x003) << (6 - 0);
        qDebug("%x: %s", x, decodeMpegFrame(x, ", ").c_str());
    }
}


} // namespace



MainFormDlgImpl::MainFormDlgImpl(const string& strSession, bool bDefaultForVisibleSessBtn) : QDialog(0, getMainWndFlags()), m_settings(strSession), m_nLastKey(0)/*, m_settings("Ciobi", "Mp3Diags_v01")*/ /*, m_nPrevTabIndex(-1), m_bTagEdtWasEntered(false)*/, m_pCommonData(0), m_strSession(strSession), m_bShowMaximized(false), m_nScanWidth(0), m_pQHttp(0), m_nGlobalX(0), m_nGlobalY(0)
{
//int x (2); CB_ASSERT(x > 4);
//CB_ASSERT("345" == "ab");
//CB_ASSERT(false);
    s_fileTracer.setName(SessionEditorDlgImpl::getBaseName(strSession)); // also disables both flags

    s_pGlobalDlg = 0;
    setupUi(this);

    //listKnownFormats(); // ttt2 sizes for many formats seem way too low (e.g. "MPEG-1 Layer I, 44100Hz 32000bps" or "MPEG-2 Layer III, 22050Hz 8000bps")

    {
        /*KbdNotifTableView* pStreamsG (new KbdNotifTableView(m_pStreamsG));
        connect(pStreamsG, SIGNAL(keyPressed(int)), this, SLOT(onStreamsGKeyPressed(int)));

        m_pStreamsG = pStreamsG;*/
        m_pStreamsG->installEventFilter(this);
        m_pFilesG->installEventFilter(this);
    }

    m_pCommonData = new CommonData(m_settings, m_pFilesG, m_pNotesG, m_pStreamsG, m_pUniqueNotesG, /*m_pCurrentFileG, m_pCurrentAlbumG,*/ /*m_pLogG,*/ /*m_pAssignedB,*/ m_pNoteFilterB, m_pDirFilterB, m_pModeAllB, m_pModeAlbumB, m_pModeSongB, bDefaultForVisibleSessBtn);

    m_settings.loadMiscConfigSettings(m_pCommonData, SessionSettings::INIT_GUI);

    m_pCommonData->m_bScanAtStartup = m_settings.loadScanAtStartup();

    string strVersion;
    m_settings.loadVersion(strVersion);
    if (strVersion == getAppVer())
    {
        bool bDirty;
        m_settings.loadDbDirty(bDirty);
        bool bCrashedAtStartup;
        m_settings.loadCrashedAtStartup(bCrashedAtStartup);

        if (bDirty || bCrashedAtStartup)
        {
            if (m_pCommonData->isTraceToFileEnabled() || fileExists(s_fileTracer.getTraceFile())) // !!! fileExists(s_strTraceFile) allows new asserts to be reported (when m_pCommonData->isTraceToFileEnabled() would still return false)
            {
                if (fileExists(s_fileTracer.getTraceFile()))
                {
                    vector<string> v;
                    if (fileExists(s_fileTracer.getTraceFile())) { v.push_back(s_fileTracer.getTraceFile()); }
                    if (fileExists(s_fileTracer.getStepFiles()[0])) { v.push_back(s_fileTracer.getStepFiles()[0]); }
                    if (fileExists(s_fileTracer.getStepFiles()[1])) { v.push_back(s_fileTracer.getStepFiles()[1]); }
                    CB_ASSERT (!v.empty()); // really v should have at least 2 elements

                    QString qstrFiles ("<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("MP3 Diags is restarting after a crash."));
                    switch (v.size())
                    {
                    case 1:
                            qstrFiles += tr("Information in the file %1%5%2 may help identify the cause of the crash so please make it available to the developer by mailing it to %3, by reporting an issue to the project's Issue Tracker at %4 and attaching the files to the report, or by some other means (like putting it on a file sharing site.)", "%1 and %2 are HTML elements")
                                .arg("<b>")
                                .arg("</b>")
                                .arg(g_qstrCrashMail)
                                .arg(g_qstrBugReport)
                                .arg(Qt::escape(toNativeSeparators(convStr(v[0])))); break;
                    case 2:
                        qstrFiles += tr("Information in the files %1%5%2 and %1%6%2 may help identify the cause of the crash so please make them available to the developer by mailing them to %3, by reporting an issue to the project's Issue Tracker at %4 and attaching the files to the report, or by some other means (like putting them on a file sharing site.)", "%1 and %2 are HTML elements")
                                .arg("<b>")
                                .arg("</b>")
                                .arg(g_qstrCrashMail)
                                .arg(g_qstrBugReport)
                                .arg(Qt::escape(toNativeSeparators(convStr(v[0]))))
                                .arg(Qt::escape(toNativeSeparators(convStr(v[1]))));
                    case 3:
                        qstrFiles += tr("Information in the files %1%5%2, %1%6%2, and %1%7%2 may help identify the cause of the crash so please make them available to the developer by mailing them to %3, by reporting an issue to the project's Issue Tracker at %4 and attaching the files to the report, or by some other means (like putting them on a file sharing site.)", "%1 and %2 are HTML elements")
                                .arg("<b>")
                                .arg("</b>")
                                .arg(g_qstrCrashMail)
                                .arg(g_qstrBugReport)
                                .arg(Qt::escape(toNativeSeparators(convStr(v[0]))))
                                .arg(Qt::escape(toNativeSeparators(convStr(v[1]))))
                                .arg(Qt::escape(toNativeSeparators(convStr(v[2]))));
                        break;
                    }
                    qstrFiles += " </p>";

                    showRestartAfterCrashMsg(qstrFiles +

                                             "<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("These are plain text files, which you can review before sending, if you have privacy concerns.") + "</p>"
                                             "<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("After getting the files, the developer will probably want to contact you for more details, so please check back on the status of your report.") + "</p>"
                                             "<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("Note that these files <b>will be removed</b> when you close this window.") + "</p>" +

                                             (m_pCommonData->isTraceToFileEnabled() ?
                                                  "<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("If there is a name of an MP3 file at the end of <b>%1</b>, that might be a file that consistently causes a crash. Please check if it is so. Then, if confirmed, please make that file available by mailing it to %2 or by putting it on a file sharing site.").arg(Qt::escape(toNativeSeparators(convStr(v[0])))).arg(g_qstrCrashMail) + "</p>" :
                                                  "<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("Please also try to <b>repeat the steps that led to the crash</b> before reporting the crash, which will probably result in a new set of files being generated; these files are more likely to contain relevant information than the current set of files, because they will also have information on what happened before the crash, while the current files only tell where the crash occured.") + "</p>"
                                             )

                                             + "<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("You should include in your report any other details that seem relevant (what might have caused the failure, steps to reproduce it, ...)") + "</p>", tr("Remove these files and continue"));
                }
                else
                {
                    showRestartAfterCrashMsg("<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("MP3 Diags is restarting after a crash. There was supposed to be some information about what led to the crash in the file <b>%1</b>, but that file cannot be found. Please report this issue to the project's Issue Tracker at %2.").arg(Qt::escape(toNativeSeparators(convStr(s_fileTracer.getTraceFile())))).arg(g_qstrBugReport) + "</p>"
                                             + "<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("The developer will probably want to contact you for more details, so please check back on the status of your report.</p><p style=\"margin-bottom:8px; margin-top:1px; \">Make sure to include the data below, as well as any other detail that seems relevant (what might have caused the failure, steps to reproduce it, ...)") + "</p>", "OK");
                }
            }


            if (!m_pCommonData->isTraceToFileEnabled())
            {
                showRestartAfterCrashMsg("<p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("MP3 Diags is restarting after a crash. To help determine the reason for the crash, the <i>Log program state to _trace and _step files</i> option has been activated. This logs to 3 files what the program is doing, which might make it slightly slower.") + "</p><p style=\"margin-bottom:8px; margin-top:1px; \">" + tr("It is recommended to not process more than several thousand MP3 files while this option is turned on. You can turn it off manually, in the configuration dialog, in the <i>Others</i> tab, but keeping it turned on may provide very useful feedback to the developer, should the program crash again. With this feedback, future versions of MP3 Diags will get closer to being bug free.") + "</p>", "OK");

                m_pCommonData->setTraceToFile(true);
                m_settings.saveMiscConfigSettings(m_pCommonData);
            }

            // !!! don't change "dirty"; 1) there's no point; 2) ser needs it
        }

        // !!! nothing to do if not dirty
    }
    else
    { // it's a new version, so we start over //ttt2 perhaps also use some counter, like "20 runs without crash"
        m_pCommonData->setTraceToFile(false);
        m_settings.saveMiscConfigSettings(m_pCommonData);
    }

#ifdef LOG_ANYWAY
    s_fileTracer.enable2(true);
#else
    s_fileTracer.enable2(m_pCommonData->isTraceToFileEnabled()); // this might get called a second time (the first time is from within m_pCommonData->setTraceToFile()), but that's OK
#endif
    s_fileTracer.enable1(true); // !!! after loadMiscConfigSettings(), so the previous files aren't deleted too early

    m_settings.saveVersion(getAppVer());

    TRACER("MainFormDlgImpl constr");

    {
        m_pCommonData->m_pFilesModel = new FilesModel(m_pCommonData);
        m_pFilesG->setModel(m_pCommonData->m_pFilesModel);

        FilesGDelegate* pDel (new FilesGDelegate(m_pCommonData, m_pFilesG));
        m_pFilesG->setItemDelegate(pDel);

        m_pFilesG->setHorizontalHeader(new FileHeaderView(m_pCommonData, m_pFilesG));

        m_pFilesG->horizontalHeader()->setMinimumSectionSize(CELL_WIDTH);
        m_pFilesG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        m_pFilesG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);

        connect(m_pFilesG->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), m_pCommonData->m_pFilesModel, SLOT(onFilesGSelChanged()));
        connect(m_pFilesG, SIGNAL(clicked(const QModelIndex &)), m_pCommonData->m_pFilesModel, SLOT(onFilesGSelChanged()));

        m_pFilesG->horizontalHeader()->setDefaultSectionSize(CELL_WIDTH);
        m_pFilesG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);

        m_pFilesG->verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);

        connect(m_pCommonData->m_pFilesModel, SIGNAL(currentFileChanged()), this, SLOT(onCrtFileChanged()));
        connect(m_pCommonData->m_pFilesModel, SIGNAL(currentFileChanged()), m_pCommonData, SLOT(onCrtFileChanged()));
    }

    {
        m_pCommonData->m_pNotesModel = new NotesModel(m_pCommonData);
        m_pNotesG->setModel(m_pCommonData->m_pNotesModel);

        NotesGDelegate* pNotesGDelegate = new NotesGDelegate(m_pCommonData);
        m_pNotesG->setItemDelegate(pNotesGDelegate);

        m_pNotesG->horizontalHeader()->setMinimumSectionSize(CELL_WIDTH + 10);
        m_pNotesG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        m_pNotesG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);

        m_pNotesG->verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);

        connect(m_pNotesG->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), m_pCommonData->m_pNotesModel, SLOT(onNotesGSelChanged()));
        connect(m_pNotesG, SIGNAL(clicked(const QModelIndex &)), m_pCommonData->m_pNotesModel, SLOT(onNotesGSelChanged()));

        connect(m_pNotesG->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), m_pNotesG, SLOT(resizeRowsToContents()));
    }

    {
        m_pCommonData->m_pStreamsModel = new StreamsModel(m_pCommonData);
        m_pStreamsG->setModel(m_pCommonData->m_pStreamsModel);

        StreamsGDelegate* pStreamsGDelegate = new StreamsGDelegate(m_pCommonData);
        m_pStreamsG->setItemDelegate(pStreamsGDelegate);

        m_pStreamsG->horizontalHeader()->setMinimumSectionSize(CELL_WIDTH + 10);
        m_pStreamsG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        m_pStreamsG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);

        m_pStreamsG->verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);

        connect(m_pStreamsG->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), m_pCommonData->m_pStreamsModel, SLOT(onStreamsGSelChanged()));
        connect(m_pStreamsG, SIGNAL(clicked(const QModelIndex &)), m_pCommonData->m_pStreamsModel, SLOT(onStreamsGSelChanged()));

        connect(m_pStreamsG->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), m_pStreamsG, SLOT(resizeRowsToContents()));
    }

    {
        m_pCommonData->m_pUniqueNotesModel = new UniqueNotesModel(m_pCommonData);
        m_pUniqueNotesG->setModel(m_pCommonData->m_pUniqueNotesModel);

        UniqueNotesGDelegate* pDel = new UniqueNotesGDelegate(m_pCommonData);
        m_pUniqueNotesG->setItemDelegate(pDel);

        m_pUniqueNotesG->horizontalHeader()->setMinimumSectionSize(CELL_WIDTH + 10);
        m_pUniqueNotesG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        m_pUniqueNotesG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);

        m_pUniqueNotesG->verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);

        connect(m_pUniqueNotesG->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), m_pUniqueNotesG, SLOT(resizeRowsToContents()));
    }


    m_pTagDetailsLayout = new QHBoxLayout(m_pTagDetailsTab);
    m_pTagDetailsTab->setLayout(m_pTagDetailsLayout);

    m_pTagDetailsW = new QWidget(m_pTagDetailsTab);
    m_pTagDetailsLayout->addWidget(m_pTagDetailsW);
    //m_pTagDetailsLayout->setContentsMargins(1, 1, 1, 1);
    //m_pTagDetailsLayout->setContentsMargins(6, 6, 6, 6);
    m_pTagDetailsLayout->setContentsMargins(0, 0, 0, 0);



    /*connect(m_pFilesG->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), m_pCommonData->m_pFilesModel, SLOT(onFilesGSelChanged())); //ttt2 see if needed (in addition to selectionChanged); apparently not: this signal is sent "the next time", so first clicking in a grid doesn't send any message, regardless of what was "current" before; then, when the message is sent, the value in the model is not yet updated, so using "m_pCommonData->m_pNotesG->selectionModel()->selection().indexes()" returns the indexes from the last time; perhaps using the first QModelIndex parameter would allow getting to the "current" cell, but not to the whole selection anyway;
    connect(m_pNotesG->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), m_pCommonData->m_pNotesModel, SLOT(onNotesGSelChanged()));
    connect(m_pStreamsG->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), m_pCommonData->m_pStreamsModel, SLOT(onStreamsGSelChanged()));*/


    /* !!! There's this use case:

        1) the user selects a file that has multiple instances of the same error
        2) the user clicks on one of the duplicates errors in m_pNotesG; the corresponding cell in m_pFilesG gets selected;
        3) the user clicks on the selected cell in m_pFilesG; both of the duplicate errors should get selected;

    If nothing is done, nothing happens, and just one error stays selected; making m_pFilesG's current be (-1,-1) in FilesModel::matchSelToNotes() doesn't work anyaway because this doesn't change the selection either and it messes up keyboard navigation (assuming that it can be done).

    Instead, m_pFilesG's clicked() signal is connected to onFilesGSelChanged().
    */

    //cout << convStr(m_settings.fileName()) << endl;

    { ModifInfoToolButton* p (new ModifInfoToolButton(m_pCustomTransform1B)); m_vpTransfButtons.push_back(p); connect(p, SIGNAL(clicked()), this, SLOT(on_m_pCustomTransform1B_clicked())); m_pCustomTransform1B = p; }
    { ModifInfoToolButton* p (new ModifInfoToolButton(m_pCustomTransform2B)); m_vpTransfButtons.push_back(p); connect(p, SIGNAL(clicked()), this, SLOT(on_m_pCustomTransform2B_clicked())); m_pCustomTransform2B = p; }
    { ModifInfoToolButton* p (new ModifInfoToolButton(m_pCustomTransform3B)); m_vpTransfButtons.push_back(p); connect(p, SIGNAL(clicked()), this, SLOT(on_m_pCustomTransform3B_clicked())); m_pCustomTransform3B = p; }
    { ModifInfoToolButton* p (new ModifInfoToolButton(m_pCustomTransform4B)); m_vpTransfButtons.push_back(p); connect(p, SIGNAL(clicked()), this, SLOT(on_m_pCustomTransform4B_clicked())); m_pCustomTransform4B = p; } // CUSTOM_TRANSF_CNT

    { m_pModifNormalizeB = new ModifInfoToolButton(m_pNormalizeB); connect(m_pModifNormalizeB, SIGNAL(clicked()), this, SLOT(on_m_pNormalizeB_clicked())); m_pNormalizeB = m_pModifNormalizeB; }
    { m_pModifReloadB = new ModifInfoToolButton(m_pReloadB); connect(m_pModifReloadB, SIGNAL(clicked()), this, SLOT(on_m_pReloadB_clicked())); m_pReloadB = m_pModifReloadB; }

    { m_pModifRenameFilesB = new ModifInfoToolButton(m_pRenameFilesB); connect(m_pModifRenameFilesB, SIGNAL(clicked()), this, SLOT(on_m_pRenameFilesB_clicked())); m_pRenameFilesB = m_pModifRenameFilesB; }

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }

    /*{ QAction* p (new QAction(this)); p->setShortcut(QKeySequence("Ctrl+N")); connect(p, SIGNAL(triggered()), this, SLOT(onNext())); addAction(p); } //p->setShortcutContext(Qt::ApplicationShortcut);
    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("Ctrl+P")); connect(p, SIGNAL(triggered()), this, SLOT(onPrev())); addAction(p); }
    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("Ctrl+V")); connect(p, SIGNAL(triggered()), this, SLOT(onPaste())); addAction(p); }
    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("Ctrl+S")); connect(p, SIGNAL(triggered()), this, SLOT(on_m_pScanB_clicked())); addAction(p); }*/

    //{ QAction* p (new QAction(this)); p->setShortcut(QKeySequence(Qt::Key_Escape)); connect(p, SIGNAL(triggered()), this, SLOT(emptySlot())); addAction(p); } // !!! 2009.01.13 - no longer usable, because this also prevents edits in QTableView from exiting with ESC; so the m_nLastKey alternative is used; // 2009.03.31 - probably usable again, since the tag editor got moved to a separate window

    //m_pCurrentAlbumG->setEditTriggers(QAbstractItemView::AllEditTriggers);//EditKeyPressed);

    loadIgnored();

    {
        if (!m_settings.loadTransfConfig(m_transfConfig))
        {
            m_settings.saveTransfConfig(m_transfConfig);
        }

        for (int i = 0; i < CUSTOM_TRANSF_CNT; ++i)
        {
            loadCustomTransf(i);
        }
    }

//ttt2 perhaps have "experimental" transforms, different color (or just have the names begin with "experimental")
    {
        loadVisibleTransf();
        if (m_pCommonData->getVisibleTransf().empty())
        {
            vector<int> v;
            initDefaultVisibleTransf(v, m_pCommonData);
            m_pCommonData->setVisibleTransf(v);
        }
    }

    {
        loadExternalTools();
    }

    {
        initializeUi();
    }

    setTransfTooltips();

    {
        delete m_pRemovableL;

        delete m_pLowerHalfTablesW->layout();
        m_pLowerHalfLayout = new QStackedLayout(m_pLowerHalfTablesW);
        //m_pLowerHalfLayout->setContentsMargins(0, 50, 50, 0);
        //m_pLowerHalfTablesW->setLayout(m_pLowerHalfLayout);
        m_pLowerHalfLayout->addWidget(m_pFileInfoTab);
        m_pLowerHalfLayout->addWidget(m_pAllNotesTab);
        m_pLowerHalfLayout->addWidget(m_pTagDetailsTab);


        delete m_pDetailsTabWidget;

        int nHeight (QApplication::fontMetrics().height() + 7);

        m_pLowerHalfBtnW->setMinimumHeight(nHeight);
        m_pLowerHalfBtnW->setMaximumHeight(nHeight);
    }

    connect(this, SIGNAL(tagEditorClosed()), m_pCommonData, SLOT(onFilterChanged())); // !!! needed because CommonData::mergeHandlerChanges() adds changed files that shouldn't normally be there in album / filter mode; the reason it does this is to allow comparisons after making changes, but this doesn't make much sense when those changes are done in the tag editor, (saving in the tag editor is different from regular transformations because album navigation is allowed)

    s_pGlobalDlg = this;

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("Ctrl+A")); connect(p, SIGNAL(triggered()), this, SLOT(emptySlot())); addAction(p); } // !!! needed because it takes a lot of time, during which the app seems frozen (caused by the cells being selected and unselected automatically) // ttt2 see if possible to disable selecting "note" cells with SHIFT pressed


    QTimer::singleShot(1, this, SLOT(onShow()));
}








/*override*/ void MainFormDlgImpl::keyPressEvent(QKeyEvent* pEvent)
{
//qDebug("key prs %x", pEvent->key());
    m_nLastKey = pEvent->key();

    pEvent->ignore();
}


void MainFormDlgImpl::onHelp()
{
    if (m_pViewFileInfoB->isChecked())
    {
        openHelp("130_main_window.html");
    }
    else if (m_pViewAllNotesB->isChecked())
    {
        openHelp("150_main_window_all_notes.html");
    }
    else if (m_pViewTagDetailsB->isChecked())
    {
        openHelp("160_main_window_tag_details.html");
    }
    else
    {
        CB_ASSERT(false);
    }
}

/*override*/ void MainFormDlgImpl::keyReleaseEvent(QKeyEvent* pEvent)
{
//qDebug("key rel %d", pEvent->key());
    if (Qt::Key_Escape == pEvent->key())
    {
        //on_m_pAbortB_clicked();
        pEvent->ignore();
    }
    else
    {
        QDialog::keyReleaseEvent(pEvent);
    }
    //pEvent->ignore(); // ttt2 not sure this is the way to do it, but the point is to disable the ESC key
}




void MainFormDlgImpl::initializeUi()
{
    int nWidth, nHeight;
    int nNotesGW0, nNotesGW2, nStrmsGW0, nStrmsGW1, nStrmsGW2, nStrmsGW3, nUnotesGW0;
    int nIconSize;
    QByteArray stateMainSpl, stateLwrSpl;
    m_settings.loadMainSettings(nWidth, nHeight, nNotesGW0, nNotesGW2, nStrmsGW0, nStrmsGW1, nStrmsGW2, nStrmsGW3, nUnotesGW0, stateMainSpl, stateLwrSpl, nIconSize, m_nScanWidth);

    if (m_nScanWidth <= 400)
    {
        QRect r (QApplication::desktop()->availableGeometry());
        m_nScanWidth = min(1600, r.width());
        if (m_nScanWidth > 1200)
        {
            m_nScanWidth = m_nScanWidth*3/4;
        }
        else
        {
            m_nScanWidth = m_nScanWidth*4/5;
        }
        //qDebug("%d %d %d %d", r.x(), r.y(), r.width(), r.height());
    }

    if (nWidth > 400 && nHeight > 400)
    {
        QRect r (QApplication::desktop()->availableGeometry());
        const int nApprox (16);

#ifndef WIN32
        int nTitleHeight(0);
#else
        int nTitleHeight(GetSystemMetrics(SM_CYSIZE)); // ttt2 actually there's a pixel missing but not obvious where to get it from; nApprox should allow enough tolerance, though
#endif

        if (r.width() - nWidth < nApprox && r.height() - nHeight - nTitleHeight < nApprox)
        {
            m_bShowMaximized = true;
        }
        else
        {
            resize(nWidth, nHeight);
        }
    }
    else
    {
        //QRect r (QApplication::desktop()->availableGeometry());
        //qDebug("%d %d %d %d", r.x(), r.y(), r.width(), r.height());
        m_bShowMaximized = true; // ttt2 perhaps implement m_bShowMaximized for all windows
    }

    m_pCommonData->m_nMainWndIconSize = nIconSize;

    {
        if (nNotesGW0 < CELL_WIDTH + 8) { nNotesGW0 = CELL_WIDTH + 8; }
        if (nNotesGW2 < 10) { nNotesGW2 = 65; }

        m_pNotesG->horizontalHeader()->resizeSection(0, nNotesGW0); // ttt2 apparently a call to resizeColumnsToContents() in NotesModel::updateCurrentNotes() should make columns 0 and 2 have the right size, but that's not the case at all; (see further notes there)
        m_pNotesG->horizontalHeader()->resizeSection(2, nNotesGW2);
        m_pNotesG->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
        //m_pNotesG->horizontalHeader()->setResizeMode(2, QHeaderView::Stretch);
    }

    {
        if (nStrmsGW0 < 10) { nStrmsGW0 = 80; } // ttt2 "80" hard-coded
        if (nStrmsGW1 < 10) { nStrmsGW1 = 80; }
        if (nStrmsGW2 < 10) { nStrmsGW2 = 80; }
        if (nStrmsGW3 < 10) { nStrmsGW3 = 110; }

        m_pStreamsG->horizontalHeader()->resizeSection(0, nStrmsGW0);
        m_pStreamsG->horizontalHeader()->resizeSection(1, nStrmsGW1);
        m_pStreamsG->horizontalHeader()->resizeSection(2, nStrmsGW2);
        m_pStreamsG->horizontalHeader()->resizeSection(3, nStrmsGW3);
        m_pStreamsG->horizontalHeader()->setResizeMode(4, QHeaderView::Stretch);
    }

    {
        if (nUnotesGW0 < CELL_WIDTH + 8) { nUnotesGW0 = CELL_WIDTH + 8; } // ttt2 replace CELL_WIDTH

        m_pUniqueNotesG->horizontalHeader()->resizeSection(0, nUnotesGW0);
        m_pUniqueNotesG->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    }


    {
        if (!stateMainSpl.isNull())
        {
            m_pMainSplitter->restoreState(stateMainSpl);
        }
        m_pMainSplitter->setOpaqueResize(false);

        if (!stateLwrSpl.isNull())
        {
            m_pLowerSplitter->restoreState(stateLwrSpl);
        }
    }


    m_pCrtDirE->setFocus();

    if (!m_pCommonData->m_bShowExport)
    {
        m_pExportB->hide();
    }

    if (!m_pCommonData->m_bShowDebug)
    {
        m_pDebugB->hide();
    }

    if (!m_pCommonData->m_bShowSessions)
    {
        m_pSessionsB->hide();
        if (!m_pCommonData->m_bShowExport)
        {
            m_pOptBtn1W->hide();
        }
    }

    resizeIcons();
}


void MainFormDlgImpl::onShow()
{
    TRACER("MainFormDlgImpl::onShow()");
    bool bLoadErr (false);

    bool bCrashedAtStartup;
    m_pCommonData->m_strTransfLog = SessionEditorDlgImpl::getLogFileName(m_strSession);
    m_settings.loadCrashedAtStartup(bCrashedAtStartup);
    if (bCrashedAtStartup)
    {
        showCritical(this, tr("Error"), tr("MP3 Diags crashed while reading song data from the disk. The whole collection will be rescanned."));
    }
    else
    {
        m_settings.saveCrashedAtStartup(true);
        string strErr;
        SerLoadThread* p (new SerLoadThread(m_pCommonData, m_strSession, strErr));

        ThreadRunnerDlgImpl dlg (this, getNoResizeWndFlags(), p, ThreadRunnerDlgImpl::SHOW_COUNTER, ThreadRunnerDlgImpl::TRUNCATE_BEGIN, ThreadRunnerDlgImpl::HIDE_PAUSE_ABORT);
        CB_ASSERT (m_nScanWidth > 400);
        dlg.resize(m_nScanWidth, dlg.height());
        dlg.setWindowIcon(QIcon(":/images/logo.svg"));

        dlg.setWindowTitle(tr("Loading data"));
        s_pSerThread = p;
        dlg.exec();
        s_pSerThread = 0;
        m_nScanWidth = dlg.width();
        m_pCommonData->setCrtAtStartup();

        if (!strErr.empty())
        {
            bLoadErr = true;
            showCritical(this, tr("Error"), tr("An error occured while loading the MP3 information. Your files will be rescanned.\n\n").arg(convStr(strErr)));
        }
    }

    m_settings.saveCrashedAtStartup(false);

    bool bDirty;
    m_settings.loadDbDirty(bDirty);
    if (bDirty)
    {
        /*s_strErrorMsg = "Rescanning files after crash.";
        showErrorDlg(this, false);*/

        if (m_transfConfig.m_options.m_bKeepOrigTime)
        {
            showWarning(this, tr("Warning"), tr("It seems that MP3 Diags is restarting after a crash. Your files will be rescanned.\n\n(Since this may take a long time for large collections, you may want to abort the full rescanning and apply a filter to include only the files that you changed since the last time the program closed correctly, then manually rescan only those files.)"));
        }
        else
        {
            bDirty = false; // !!! if original time is not kept, any changes will be detected anyway, no need to force a full reload
        }
    }

    m_settings.saveDbDirty(true);

    if (m_pCommonData->m_bScanAtStartup || bDirty)
    {
        fullReload(bDirty || bLoadErr || bCrashedAtStartup ? FORCE : DONT_FORCE);
    }

    resizeEvent(0);

    // !!! without these the the file grid may look bad if it has a horizontal scrollbar and one of the last files is current
    string strCrt (m_pCommonData->getCrtName());
    m_pFilesG->setCurrentIndex(m_pFilesG->model()->index(0, 0));
    m_pCommonData->updateWidgets(strCrt);

#ifndef DISABLE_CHECK_FOR_UPDATES
    checkForNewVersion();
#endif
}


void MainFormDlgImpl::fullReload(bool bForceReload)
{
    CommonData::ViewMode eMode (m_pCommonData->getViewMode());
    m_pCommonData->setViewMode(CommonData::ALL, m_pCommonData->getCrtMp3Handler());
    m_pCommonData->m_filter.disableAll();
    reload(IGNORE_SEL, bForceReload);
    m_pCommonData->m_filter.restoreAll();
    m_pCommonData->setViewMode(eMode, m_pCommonData->getCrtMp3Handler());
}

/*override*/ void MainFormDlgImpl::closeEvent(QCloseEvent*)
{
    string strErr;
    SerSaveThread* p (new SerSaveThread(m_pCommonData, m_strSession, strErr));
//Qt::WindowStaysOnTopHint
    ThreadRunnerDlgImpl dlg (this, getNoResizeWndFlags(), p, ThreadRunnerDlgImpl::SHOW_COUNTER, ThreadRunnerDlgImpl::TRUNCATE_BEGIN, ThreadRunnerDlgImpl::HIDE_PAUSE_ABORT);
    CB_ASSERT (m_nScanWidth > 400);
    dlg.resize(m_nScanWidth, dlg.height());
    dlg.setWindowIcon(QIcon(":/images/logo.svg"));

    dlg.setWindowTitle(tr("Saving data"));
    s_pSerThread = p;
    dlg.exec();
    s_pSerThread = 0;
    m_nScanWidth = dlg.width();

    if (!strErr.empty())
    {
        showCritical(this, tr("Error"), tr("An error occured while saving the MP3 information. You will have to scan your files again.\n\n%1").arg(convStr(strErr)));
    }
}



MainFormDlgImpl::~MainFormDlgImpl()
{
    TRACER("MainFormDlgImpl destr");
    s_pGlobalDlg = 0;

    m_settings.saveMainSettings(
        width(),
        height(),
        m_pNotesG->horizontalHeader()->sectionSize(0),
        m_pNotesG->horizontalHeader()->sectionSize(2),

        m_pStreamsG->horizontalHeader()->sectionSize(0),
        m_pStreamsG->horizontalHeader()->sectionSize(1),
        m_pStreamsG->horizontalHeader()->sectionSize(2),
        m_pStreamsG->horizontalHeader()->sectionSize(3),

        m_pUniqueNotesG->horizontalHeader()->sectionSize(0),

        m_pMainSplitter->saveState(),
        m_pLowerSplitter->saveState(),

        m_pCommonData->m_nMainWndIconSize,

        m_nScanWidth
        );

    m_settings.saveDbDirty(false); // !!! it would seem better to delay marking the data clean until a full reload is completed; however, if the user aborted the rescan, it was probably for a good reason (e.g. to rescan only a part of the files), so it makes more sense to mark the data as clean regardless of how it was when it was loaded and if the rescan completed or not


    //QMessageBox dlg (this); dlg.show();
    //CursorOverrider crs;// (Qt::ArrowCursor);


    delete m_pCommonData;
}

void SessionSettings::saveDbDirty(bool bDirty)
{
    m_pSettings->setValue("main/dirty", bDirty);
    m_pSettings->sync();
}

void SessionSettings::loadDbDirty(bool& bDirty)
{
    bDirty = m_pSettings->value("main/dirty", false).toBool();
}



void SessionSettings::saveCrashedAtStartup(bool bCrashedAtStartup)
{
    m_pSettings->setValue("debug/crashedAtStartup", bCrashedAtStartup);
    m_pSettings->sync();
}

void SessionSettings::loadCrashedAtStartup(bool& bCrashedAtStartup)
{
    bCrashedAtStartup = m_pSettings->value("debug/crashedAtStartup", false).toBool();
}



void SessionSettings::saveVersion(const string& strVersion)
{
    m_pSettings->setValue("main/version", convStr(strVersion));
}

void SessionSettings::loadVersion(string& strVersion)
{
    strVersion = convStr(m_pSettings->value("main/version", "x").toString());
}





MainFormDlgImpl::CloseOption MainFormDlgImpl::run()
{
    if (m_bShowMaximized)
    {
        showMaximized();
    }
    return QDialog::Accepted == exec() ? OPEN_SESS_DLG : EXIT;
}





void MainFormDlgImpl::onCrtFileChanged()
{
    delete m_pTagDetailsW;
    m_pTagDetailsW = new QWidget(m_pTagDetailsTab);
    m_pTagDetailsLayout->addWidget(m_pTagDetailsW);

    int nCrtFile (m_pCommonData->getFilesGCrtRow());
    if (-1 == nCrtFile)
    {
        m_pCrtDirE->setText("");
        return;
    }

    QHBoxLayout* pLayout = new QHBoxLayout();
    pLayout->setSpacing(12);
    //pLayout->setContentsMargins(6, 6, 6, 6);
    pLayout->setContentsMargins(1, 1, 1, 1);
    m_pTagDetailsW->setLayout(pLayout);

    //pLayout->setSpacing(1);

    const Mp3Handler* pMp3Handler (m_pCommonData->getViewHandlers()[nCrtFile]);
    const vector<DataStream*>& vpStreams (pMp3Handler->getStreams());

    if (m_pViewTagDetailsB->isChecked())
    {
        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* pStream (vpStreams[i]);
            TagReader* pReader (dynamic_cast<TagReader*>(pStream));
            if (0 != pReader)
            {
                TagReadPanel* pPanel (new TagReadPanel(m_pTagDetailsW, pReader));
                pLayout->addWidget(pPanel, 10);
            }
        }
    }

    QString qs (toNativeSeparators(convStr(m_pCommonData->getViewHandlers()[nCrtFile]->getDir())));
#ifndef WIN32
#else
    if (2 == qs.size() && ':' == qs[1])
    {
        qs += "\\";
    }
#endif
    m_pCrtDirE->setText(qs);

    pLayout->addStretch(0);
}




/*override*/ void MainFormDlgImpl::resizeEvent(QResizeEvent* pEvent)
{
    if (m_pCommonData->m_bAutoSizeIcons || m_pCommonData->m_nMainWndIconSize < 16)
    {
        //const QRect& r (QApplication::desktop()->availableGeometry());
        int w (width());
        int k (w <= 800 ? 28 : w <= 1180 ? 32 : w <= 1400 ? 40 : w <= 1600 ? 48 : w <= 1920 ? 56 : 64);
        m_pCommonData->m_nMainWndIconSize = k;
        resizeIcons();
    }

    m_pCommonData->resizeFilesGCols();

    m_pUniqueNotesG->resizeRowsToContents();
    m_pNotesG->resizeRowsToContents();
    m_pStreamsG->resizeRowsToContents();

    QDialog::resizeEvent(pEvent);
}


namespace {

struct Mp3ProcThread : public PausableThread
{
    FileEnumerator& m_fileEnum;
    bool m_bForce;
    CommonData* m_pCommonData;
    vector<const Mp3Handler*> m_vpAdd, m_vpDel;
    deque<const Mp3Handler*> m_vpExisting; // some (or all) of these will may get copied to m_vpDel
    vector<const Mp3Handler*> m_vpKeep; // subset of m_vpExisting

    Mp3ProcThread(FileEnumerator& fileEnum, bool bForce, CommonData* pCommonData, deque<const Mp3Handler*> vpExisting) : m_fileEnum(fileEnum), m_bForce(bForce), m_pCommonData(pCommonData), m_vpExisting(vpExisting) {}

    /*override*/ void run()
    {
        try
        {
            CompleteNotif notif(this);

            bool bAborted (!scan());

            if (!bAborted)
            {
                sort(m_vpKeep.begin(), m_vpKeep.end(), CmpMp3HandlerPtrByName());
                set_difference(m_vpExisting.begin(), m_vpExisting.end(), m_vpKeep.begin(), m_vpKeep.end(), back_inserter(m_vpDel), CmpMp3HandlerPtrByName());
            }

            notif.setSuccess(!bAborted);
        }
        catch (...)
        {
            LAST_STEP("Mp3ProcThread::run()");
            CB_ASSERT (false); //ttt0 triggered according to https://sourceforge.net/apps/mantisbt/mp3diags/view.php?id=50 and https://sourceforge.net/apps/mantisbt/mp3diags/view.php?id=54
        }
    }

    bool scan();
};


// a subset of m_vpExisting  gets copied to m_vpDel; so if m_vpExisting is empty, m_vpDel will be empty too;
bool Mp3ProcThread::scan()
{
    //cout << "################### procRec(" << strDir << ")\n";
    //FileSearcher fs ((strDir + "/*").c_str());
    m_fileEnum.reset();

    for (;;)
    {
        string strName (m_fileEnum.next());
        if (strName.empty()) { return true; }
        QString qs;
        if (strName.size() > 4)
        {
            qs = convStr(strName.substr(strName.size() - 4)).toLower();
        }

        if (qs == ".mp3" || qs == ".id3")
        {
            if (isAborted()) { return false; }
            checkPause();

            StrList l;
            l.push_back(toNativeSeparators(convStr(strName)));
            emit stepChanged(l, -1);
            if (!m_bForce)
            {
                deque<const Mp3Handler*>::iterator it (lower_bound(m_vpExisting.begin(), m_vpExisting.end(), strName, CmpMp3HandlerPtrByName()));
                if (m_vpExisting.end() != it && (*it)->getName() == strName && !(*it)->needsReload(Mp3Handler::DONT_USE_FAST_SAVE))
                {
                    m_vpKeep.push_back(*it);
                    continue;
                }
            }

            try
            {
                const Mp3Handler* p (new Mp3Handler(strName, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds()));
                m_vpAdd.push_back(p);
            }
            catch (const Mp3Handler::FileNotFound&) //ttt2 see if it should catch more
            {
            }
        }
    }
}

} // namespace

//ttt2 perhaps: Too many notes that you don't care about are cluttering your screen? You can hide such notes, so you don't see them again. To do this, open the configuration dialog and go to the "Ignored notes" tab. See more details at ...

//ttt2 album detection: folder / tags /both

void MainFormDlgImpl::scan(FileEnumerator& fileEnum, bool bForce, deque<const Mp3Handler*> vpExisting, int nKeepWhenUpdate)
{
    m_pCommonData->clearLog();

    //m_pModeAllB->setChecked(true);

    vector<const Mp3Handler*> vpAdd, vpDel;
    {
        Mp3ProcThread* p (new Mp3ProcThread(fileEnum, bForce, m_pCommonData, vpExisting));

        ThreadRunnerDlgImpl dlg (this, getNoResizeWndFlags(), p, ThreadRunnerDlgImpl::SHOW_COUNTER, ThreadRunnerDlgImpl::TRUNCATE_BEGIN);
        CB_ASSERT (m_nScanWidth > 400);
        dlg.resize(m_nScanWidth, dlg.height());
        dlg.setWindowIcon(QIcon(":/images/logo.svg"));

        dlg.setWindowTitle(tr("Scanning MP3 files"));
        dlg.exec(); //ttt2 perhaps see if it ended with ok/reject and clear all on reject
        m_nScanWidth = dlg.width();
        vpAdd = p->m_vpAdd;
        vpDel = p->m_vpDel;
    }

    m_pCommonData->mergeHandlerChanges(vpAdd, vpDel, nKeepWhenUpdate);

    if (!m_pCommonData->m_bToldAboutSupport && !s_bToldAboutSupportInCrtRun)
    {
        const vector<const Note*>& v (m_pCommonData->getUniqueNotes().getFltVec());
        vector<const Note*>::const_iterator it (lower_bound(v.begin(), v.end(), &Notes::unsupportedFound(), CmpNotePtrById()));
        if (v.end() != it && &Notes::unsupportedFound() == *it)
        {
            s_bToldAboutSupportInCrtRun = true;

            HtmlMsg::msg(this, 0, 0, &m_pCommonData->m_bToldAboutSupport, HtmlMsg::DEFAULT, tr("Info"), "<p style=\"margin-bottom:1px; margin-top:12px; \">" + tr("Your files are not fully supported by the current version of MP3 Diags. The main reason for this is that the developer is aware of some MP3 features but doesn't have actual MP3 files to implement support for those features and test the code.") + "</p>"

            "<p style=\"margin-bottom:1px; margin-top:12px; \">" + tr("You can help improve MP3 Diags by making files with unsupported notes available to the developer. The preferred way to do this is to report an issue on the project's Issue Tracker at %1, after checking if others made similar files available. To actually send the files, you can mail them to %2 or put them on a file sharing site. It would be a good idea to make sure that you have the latest version of MP3 Diags.").arg(g_qstrBugReport).arg(g_qstrSupportMail) + "</p>"

            "<p style=\"margin-bottom:1px; margin-top:12px; \">" + tr("You can identify unsupported notes by the blue color that is used for their labels.") + "</p>", 750, 300, tr("O&K"));

            if (m_pCommonData->m_bToldAboutSupport)
            {
                m_settings.saveMiscConfigSettings(m_pCommonData);
            }
        }
    }
}





void MainFormDlgImpl::on_m_pScanB_clicked()
{
    bool bForce;
    bool bOk;
    {
        ScanDlgImpl dlg (this, m_pCommonData);
        bOk = dlg.run(bForce);
    }

    if (!bOk) { return; }

    scan(bForce);
}


void MainFormDlgImpl::scan(bool bForce)
{
    long nPrevMem(getMemUsage());

    m_pCommonData->m_filter.disableNote();
    m_pCommonData->m_filter.setDirs(vector<string>());
    m_pCommonData->setViewMode(CommonData::ALL);

    //scan(m_pCommonData->m_dirTreeEnum, bForce, m_pCommonData->getViewHandlers(), CommonData::SEL | CommonData::CURRENT);
    scan(m_pCommonData->m_dirTreeEnum, bForce, m_pCommonData->getViewHandlers(), CommonData::NOTHING);
    long nCrtMem(getMemUsage());

    ostringstream out;
    time_t t (time(0));
    out << "current memory used by the whole program: " << nCrtMem << "; memory used by the current data (might be very inaccurate): " << nCrtMem - nPrevMem << "; time: " << ctime(&t);

    string s (out.str());
    s.erase(s.size() - 1); // needed because ctime() uses a terminating '\n'

    qDebug("%s", s.c_str());

    trace("");
    trace("************************* " + s);
    //exportAsText();
}









void MainFormDlgImpl::on_m_pNoteFilterB_clicked()
{
    if (m_pCommonData->m_filter.isNoteEnabled())
    {
        m_pCommonData->m_filter.disableNote();
        return;
    }

    NoteFilterDlgImpl dlg (m_pCommonData, this);

    if (QDialog::Accepted == dlg.exec())
    {
        // !!! no need to do anything with m_pCommonData->m_filter, because dlg.exec() took care of that
    }
    else
    {
        m_pNoteFilterB->setChecked(false); // !!! no guard needed, because the event that calls the filter is "clicked", not "checked"
    }
}




void MainFormDlgImpl::on_m_pDirFilterB_clicked()
{
    if (m_pCommonData->m_filter.isDirEnabled())
    {
        m_pCommonData->m_filter.disableDir();
        return;
    }

    DirFilterDlgImpl dlg (m_pCommonData, this);

    if (QDialog::Accepted == dlg.exec())
    {
        // !!! no need to do anything with m_pCommonData->m_filter, because dlg.exec() took care of that
    }
    else
    {
        m_pDirFilterB->setChecked(false); // !!! no guard needed, because the event that calls the filter is "clicked", not "checked"
    }
}




//ttt2 ? operation to "add null frame", which would allow to rescue some frames in case of overwriting



void MainFormDlgImpl::saveCustomTransf(int k)
{
    const vector<int>& v (m_pCommonData->getCustomTransf()[k]);
    int n (cSize(v));
    vector<string> vstrActionNames;
    for (int i = 0; i < n; ++i)
    {
        vstrActionNames.push_back(m_pCommonData->getAllTransf()[v[i]]->getActionName());
    }
    char bfr [50];
    sprintf(bfr, "customTransf/set%04d", k);
    m_settings.saveVector(bfr, vstrActionNames);
}




void MainFormDlgImpl::loadCustomTransf(int k)
{
    char bfr [50];
    sprintf(bfr, "customTransf/set%04d", k);
    //vector<string> vstrNames (m_settings.loadCustomTransf(k));
    bool bErr;
    vector<string> vstrNames (m_settings.loadVector(bfr, bErr));
    vector<int> v;
    const vector<Transformation*>& u (m_pCommonData->getAllTransf());
    int m (cSize(u));
    for (int i = 0, n = cSize(vstrNames); i < n; ++i)
    {
        string strName (vstrNames[i]);
        int j (0);
        for (; j < m; ++j)
        {
            if (u[j]->getActionName() == strName)
            {
                v.push_back(j);
                break;
            }
        }

        if (j == m)
        {
            showWarning(this, tr("Error setting up custom transformations"), tr("Couldn't find a transformation with the name \"%1\". The program will proceed, but you should review the custom transformations lists.").arg(convStr(strName)));
        }
    }


    if (v.empty())
    {
        vector<vector<int> > vv (CUSTOM_TRANSF_CNT);
        initDefaultCustomTransf(k, vv, m_pCommonData);
        v = vv[k];
    }

    m_pCommonData->setCustomTransf(k, v);
}



void MainFormDlgImpl::saveVisibleTransf()
{
    const vector<int>& v (m_pCommonData->getVisibleTransf());
    int n (cSize(v));
    vector<string> vstrActionNames;
    for (int i = 0; i < n; ++i)
    {
        vstrActionNames.push_back(m_pCommonData->getAllTransf()[v[i]]->getActionName());
    }
    m_settings.saveVector("visibleTransf", vstrActionNames);
}


void MainFormDlgImpl::loadVisibleTransf()
{
    bool bErr;
    vector<string> vstrNames (m_settings.loadVector("visibleTransf", bErr)); //ttt2 check bErr
    vector<int> v;
    const vector<Transformation*>& u (m_pCommonData->getAllTransf());
    int m (cSize(u));
    for (int i = 0, n = cSize(vstrNames); i < n; ++i)
    {
        string strName (vstrNames[i]);
        int j (0);
        for (; j < m; ++j)
        {
            if (u[j]->getActionName() == strName)
            {
                v.push_back(j);
                break;
            }
        }

        if (j == m)
        {
            showWarning(this, tr("Error setting up visible transformations"), tr("Couldn't find a transformation with the name \"%1\". The program will proceed, but you should review the visible transformations list.").arg(convStr(strName)));
        }
    }

    m_pCommonData->setVisibleTransf(v);
}



void MainFormDlgImpl::saveExternalTools()
{
    vector<string> vstrExternalTools;
    for (int i = 0, n = cSize(m_pCommonData->m_vExternalToolInfos); i < n; ++i)
    {
        vstrExternalTools.push_back(m_pCommonData->m_vExternalToolInfos[i].asString());
    }
    m_settings.saveVector("externalTools", vstrExternalTools);
}


void MainFormDlgImpl::loadExternalTools()
{
    bool bErr;
    vector<string> vstrExternalTools (m_settings.loadVector("externalTools", bErr)); //ttt2 check bErr
    m_pCommonData->m_vExternalToolInfos.clear();
    for (int i = 0, n = cSize(vstrExternalTools); i < n; ++i)
    {
        try
        {
            m_pCommonData->m_vExternalToolInfos.push_back(ExternalToolInfo(vstrExternalTools[i]));
        }
        catch (const ExternalToolInfo::InvalidExternalToolInfo&)
        {
            showWarning(this, tr("Error setting up external tools"), tr("Unable to parse \"%1\". The program will proceed, but you should review the external tools list.").arg(convStr(vstrExternalTools[i])));
        }
    }
}






//ttt2 keyboard shortcuts: next, prev, ... ;
//ttt3 set focus on some edit box when changing tabs;


//ttt2 perhaps add "whatsThis" texts
//ttt2 perhaps add "what's this" tips



void MainFormDlgImpl::on_m_pTransformB_clicked() //ttt2 an alternative is to use QToolButton::setMenu(); see if that is really simpler
{
    showBackupWarn();
    showSelWarn();

    ModifInfoMenu menu;
    vector<QAction*> vpAct;

    const vector<Transformation*>& vpTransf (m_pCommonData->getAllTransf());
    const vector<int>& vnVisualNdx (m_pCommonData->getVisibleTransf());

    for (int i = 0, n = cSize(vnVisualNdx); i < n; ++i)
    {
        Transformation* pTransf (vpTransf.at(vnVisualNdx[i]));
        QAction* pAct (new QAction(pTransf->getVisibleActionName(), &menu));
        pAct->setToolTip(makeMultiline(Transformation::tr(pTransf->getDescription())));

        //connect(pAct, SIGNAL(triggered()), this, SLOT(onExecTransform(i))); // !!! Qt doesn't seem to support parameter binding
        menu.addAction(pAct);
        vpAct.push_back(pAct);
    }

    connect(&menu, SIGNAL(hovered(QAction*)), this, SLOT(onMenuHovered(QAction*)));

    QAction* p (menu.exec(m_pTransformB->mapToGlobal(QPoint(0, m_pTransformB->height()))));
    if (0 != p)
    {
        int nIndex (std::find(vpAct.begin(), vpAct.end(), p) - vpAct.begin());
        vector<Transformation*> v;
        v.push_back(vpTransf.at(vnVisualNdx[nIndex]));
        transform(v, 0 == (Qt::ShiftModifier & menu.getModifiers()) ? ALL : SELECTED);
    }
}


void MainFormDlgImpl::onMenuHovered(QAction* pAction)
{
    //QToolTip::showText(QCursor::pos(), ""); // this was needed initially but at some time tooltips on menus stopped working (e.g. in 11.4) and commenting this out fixed the problem
    QToolTip::showText(QCursor::pos(), pAction->toolTip());
    // see http://www.mail-archive.com/pyqt@riverbankcomputing.com/msg17214.html and http://www.mail-archive.com/pyqt@riverbankcomputing.com/msg17245.html ; apparently there's some inconsistency in when the menus are shown
}


void MainFormDlgImpl::on_m_pNormalizeB_clicked()
{
    showBackupWarn();
    showSelWarn();

    bool bSel (0 != (Qt::ShiftModifier & m_pModifNormalizeB->getModifiers()));
    const deque<const Mp3Handler*>& vpHndlr (bSel ? m_pCommonData->getSelHandlers() : m_pCommonData->getViewHandlers());

    QStringList l;
    for (int i = 0, n = cSize(vpHndlr); i < n; ++i)
    {
        l << vpHndlr[i]->getUiName();
    }
    if (l.isEmpty())
    {
        showCritical(this, tr("Error"), tr("There are no files to normalize."));
        return;
    }

    int nIssueCount (0);
    QString qstrWarn;
    if (bSel && cSize(m_pCommonData->getViewHandlers()) != cSize(m_pCommonData->getSelHandlers()))
    {
        ++nIssueCount;
        qstrWarn += "\n- " + tr("you are requesting to normalize only some of the files");
    }

    if (CommonData::FOLDER != m_pCommonData->getViewMode())
    {
        ++nIssueCount;
        qstrWarn += "\n- " + tr("the \"Album\" mode is not selected");
    }

    if (m_pCommonData->m_filter.isNoteEnabled() || m_pCommonData->m_filter.isDirEnabled())
    {
        ++nIssueCount;
        if (m_pCommonData->m_filter.isNoteEnabled() && m_pCommonData->m_filter.isDirEnabled())
        {
            qstrWarn += "\n- " + tr("filters are applied");
        }
        else
        {
            qstrWarn += "\n- " + tr("a filter is applied");
        }
    }

    if (cSize(vpHndlr) > 50)
    {
        ++nIssueCount;
        qstrWarn += "\n- " + tr("the normalization will process more than 50 files, which is more than what an album usually has");
    }

    if (0 != nIssueCount)
    {
        QString s;
        if (1 == nIssueCount)
        {
            qstrWarn.remove(0, 3);
            s = tr("Normalization should process one whole album at a time, so it should only be run in \"Album\" mode, when no filters are active, and it should be applied to all the files in that album. But in the current case %1.").arg(qstrWarn);
        }
        else
        {
            s = tr("Normalization should process one whole album at a time, so it should only be run in \"Album\" mode, when no filters are active, and it should be applied to all the files in that album. But in the current case  there are some issues:\n%1").arg(qstrWarn);
        }

        int k (showMessage(this, QMessageBox::Warning, 1, 1, tr("Warning"), s + "\n\n" + tr("Normalize anyway?"), tr("Normalize"), tr("Cancel")));

        if (k != 0)
        {
            return;
        }
    }

    if (0 == nIssueCount)
    {
        if (0 != showMessage(this, QMessageBox::Question, 1, 1, tr("Confirm"), tr("Normalize all the files in the current album? (Note that normalization is done \"in place\", by an external program, so it doesn't care about the transformation settings for original and modified files.)"), tr("Normalize"), tr("Cancel")))
        {
            return;
        }
    }

    ExternalToolDlgImpl dlg (this, m_pCommonData->m_bKeepNormWndOpen, m_settings, m_pCommonData, "Normalize", "230_normalize.html");
    dlg.run(convStr(m_pCommonData->m_strNormalizeCmd), l);

    reload(bSel, FORCE);
}





void MainFormDlgImpl::on_m_pReloadB_clicked()
{
    bool bSel (0 != (Qt::ShiftModifier & m_pModifReloadB->getModifiers()));
    bool bForce (0 != (Qt::ControlModifier & m_pModifReloadB->getModifiers()));
    reload(bSel, bForce);
}



void MainFormDlgImpl::reload(bool bSelOnly, bool bForce)
{
    auto_ptr<FileEnumerator> ptr;
    FileEnumerator* pEnum;
    const deque<const Mp3Handler*>* pvpExisting;
    bool bNoteFlt (m_pCommonData->m_filter.isNoteEnabled());
    bool bDirFlt (m_pCommonData->m_filter.isDirEnabled());

    if (bSelOnly || bNoteFlt || bDirFlt)
    {
        vector<string> v;
        const deque<const Mp3Handler*>& vpHndlr (bSelOnly ? m_pCommonData->getSelHandlers() : m_pCommonData->getViewHandlers());
        for (int i = 0, n = cSize(vpHndlr); i < n; ++i)
        {
            v.push_back(vpHndlr[i]->getName());
        }

        pEnum = new ListEnumerator(v);
        ptr.reset(pEnum);
    }
    else
    {
        CB_ASSERT (!m_pCommonData->m_filter.isNoteEnabled() && !m_pCommonData->m_filter.isDirEnabled());

        const deque<const Mp3Handler*>& vpHndlr (m_pCommonData->getViewHandlers());

        switch (m_pCommonData->getViewMode())
        {
        case CommonData::ALL:
            pEnum = &m_pCommonData->m_dirTreeEnum;
            break;

        case CommonData::FOLDER:
            pEnum = new ListEnumerator(vpHndlr[0]->getDir());
            ptr.reset(pEnum);
            break;

        case CommonData::FILE:
            {
                vector<string> v;
                v.push_back(vpHndlr[0]->getName());
                pEnum = new ListEnumerator(v);
                ptr.reset(pEnum);
            }
            break;

        default:
            CB_ASSERT (false);
        }
    }

    pvpExisting = &(bSelOnly ? m_pCommonData->getSelHandlers() : m_pCommonData->getViewHandlers());

    scan(*pEnum, bForce, *pvpExisting, CommonData::CURRENT | CommonData::SEL);

    if (bNoteFlt || bDirFlt)
    {
        m_pCommonData->m_filter.disableAll();
        m_pCommonData->m_filter.restoreAll();

        if (!m_pCommonData->m_filter.isNoteEnabled() && !m_pCommonData->m_filter.isDirEnabled() && !bSelOnly)
        { // the filter got removed, because nothing matched; so wee need to repeat; (otherwise the user has to press twice: once to remove the filter and a second time to see what's new)
            reload(false, bForce);
        }
    }
}



void MainFormDlgImpl::applyCustomTransf(int k)
{
    showBackupWarn();
    showSelWarn();

    vector<Transformation*> v;
    for (int i = 0, n = cSize(m_pCommonData->getCustomTransf()[k]); i < n; ++i)
    {
        v.push_back(m_pCommonData->getAllTransf()[m_pCommonData->getCustomTransf()[k][i]]);
    }
    transform(v, 0 == (Qt::ShiftModifier & m_vpTransfButtons[k]->getModifiers()) ? ALL : SELECTED);
}




// The file list is updated in the sense that if a file was changed or removed, this is reflected in the UI. However, new files are not seen. For one thing, rebuilding a 10000-file list takes a lot of time. OTOH perhaps just the new files could be added. Also, perhaps the user could be asked about updating the list.
//ttt2 review
void MainFormDlgImpl::transform(std::vector<Transformation*>& vpTransf, Subset eSubset)
{
    if (m_pCommonData->getViewHandlers().empty())
    {
        showWarning(this, tr("Warning"), tr("The file list is empty, therefore no transformations can be applied.\n\nExiting ..."));
        return;
    }

    QString qstrListInfo;
    switch (eSubset)
    {
        case ALL:
        {
            int nCnt (cSize(m_pCommonData->getViewHandlers()));
            if (nCnt < 10)
            {
                qstrListInfo = tr("all the files shown in the file list");
            }
            else
            {
                qstrListInfo = tr("all %1 files shown in the file list").arg(nCnt);
            }

            break;
        }

    case SELECTED:
        {
            int nCnt (cSize(m_pCommonData->getSelHandlers()));
            if (0 == nCnt)
            {
                showWarning(this, tr("Warning"), tr("No file is selected, therefore no transformations can be applied.\n\nExiting ..."));
                return;
            }
            else if (1 == nCnt)
            {
                qstrListInfo = "\"" + convStr(m_pCommonData->getSelHandlers()[0]->getShortName()) + "\"";
            }
            else if (2 == nCnt)
            {
                qstrListInfo = "\"" + convStr(m_pCommonData->getSelHandlers()[0]->getShortName()) + "\" " + tr("and the other selected file");
            }
            else
            {
                qstrListInfo = "\"" + convStr(m_pCommonData->getSelHandlers()[0]->getShortName()) + "\" " + tr("and the other %1 selected files").arg(nCnt - 1);
            }
            break;
        }

    case CURRENT:
        {
            CB_ASSERT (0 != m_pCommonData->getCrtMp3Handler());
            qstrListInfo = "\"" + convStr(m_pCommonData->getCrtMp3Handler()->getShortName()) + "\"";
            break;
        }

    default:
        CB_ASSERT (false);
    }

    QString qstrConf;
    if (vpTransf.empty())
    {
        if (TransfConfig::Options::UPO_DONT_CHG == m_transfConfig.m_options.m_eUnprocOrigChange)
        {
            showWarning(this, tr("Warning"), tr("The transformation list is empty.\n\nBased on the configuration, it is possible for changes to the files in the list to be performed, even in this case (the files may still be moved, renamed or erased). However, the current settings are to leave the original files unchanged, so currently there's no point in applying an empty transformation list.\n\nExiting ..."));
            return;
        }
        qstrConf = tr("Apply an empty transformation list to all the files shown in the file list? (Note that even if no transformations are performed, the files may still be moved, renamed or erased, based on the current settings.)");
    }
    else if (1 == cSize(vpTransf))
    {
        qstrConf = tr("Apply transformation \"%1\" to %2?").arg(vpTransf[0]->getVisibleActionName()).arg(qstrListInfo);
    }
    else
    {
        qstrConf = tr("Apply the following transformations to %1?").arg(qstrListInfo);
        for (int i = 0, n = cSize(vpTransf); i < n; ++i)
        {
            qstrConf += "\n      ";
            qstrConf += vpTransf[i]->getVisibleActionName();
        }
    }

    {
        const char* aOrig[] = {
            QT_TR_NOOP("don't change"),
            QT_TR_NOOP("erase"),
            QT_TR_NOOP("move"),
            QT_TR_NOOP("move"),
            QT_TR_NOOP("rename"),
            QT_TR_NOOP("move if destination doesn't exist") };

        if ((m_transfConfig.m_options.m_eProcOrigChange != TransfConfig::Options::PO_MOVE_OR_ERASE && m_transfConfig.m_options.m_eProcOrigChange != TransfConfig::Options::PO_ERASE) || m_transfConfig.m_options.m_eUnprocOrigChange != TransfConfig::Options::UPO_DONT_CHG) //ttt2 improve
        {
            qstrConf += "\n\n" + tr("Actions to be taken:");

            if (!vpTransf.empty())
            {
                qstrConf += "\n- " + tr("original file that has been transformed: %1").arg(tr(aOrig[m_transfConfig.m_options.m_eProcOrigChange]));
            }

            qstrConf += "\n- " + tr("original file that has not been transformed: %1").arg(tr(aOrig[m_transfConfig.m_options.m_eUnprocOrigChange]));
        }
    }

    if (showMessage(this, QMessageBox::Question, 1, 1, tr("Confirm"), qstrConf, tr("&Yes"), tr("&No")) != 0) { return; }

    deque<const Mp3Handler*> vpCrt;
    const deque<const Mp3Handler*>* pvpHandlers;
    switch (eSubset)
    {
    case SELECTED: pvpHandlers = &m_pCommonData->getSelHandlers(); break;
    case ALL: pvpHandlers = &m_pCommonData->getViewHandlers(); break;
    case CURRENT: vpCrt.push_back(m_pCommonData->getCrtMp3Handler()); pvpHandlers = &vpCrt; break;
    default: CB_ASSERT (false);
    }

    ::transform(*pvpHandlers, vpTransf, convStr(tr("Applying transformations to MP3 files")), this, m_pCommonData, m_transfConfig);
}






//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


void MainFormDlgImpl::on_m_pConfigB_clicked()
{
    ConfigDlgImpl dlg (m_transfConfig, m_pCommonData, this, ConfigDlgImpl::ALL_TABS);
    string s (m_pCommonData->getCrtName());

    if (dlg.run())
    {
        m_settings.saveMiscConfigSettings(m_pCommonData);
        m_settings.saveScanAtStartup(m_pCommonData->m_bScanAtStartup);
        m_settings.saveTransfConfig(m_transfConfig); // transformation

        updateUi(s);
    }
}


void MainFormDlgImpl::setTransfTooltip(int k)
{
    QString s1 (tr("Apply custom transformation list #%1\n").arg(k + 1));
    QString s2;
    for (int i = 0, n = cSize(m_pCommonData->getCustomTransf()[k]); i < n; ++i)
    {
        s2 += "    ";
        s2 += m_pCommonData->getAllTransf()[m_pCommonData->getCustomTransf()[k][i]]->getVisibleActionName();
        if (i < n - 1) { s2 += "\n"; }
    }
    if (s2.isEmpty()) { s2 = tr("   <empty list>\n\n(you can edit the list in the Settings dialog)"); } // well; at startup it will get repopulated, so the only way for this to be empty is if it was configured like this in the current session
    m_vpTransfButtons[k]->setToolTip(s1 + s2);
}


void MainFormDlgImpl::on_m_pModeAllB_clicked()
{
    m_pCommonData->setViewMode(CommonData::ALL, m_pCommonData->getCrtMp3Handler());
    m_pFilesG->setFocus();
}


void MainFormDlgImpl::on_m_pModeAlbumB_clicked()
{
    m_pCommonData->setViewMode(CommonData::FOLDER, m_pCommonData->getCrtMp3Handler());
    m_pFilesG->setFocus();
}


void MainFormDlgImpl::on_m_pModeSongB_clicked()
{
    m_pCommonData->setViewMode(CommonData::FILE, m_pCommonData->getCrtMp3Handler());
    m_pFilesG->setFocus();
}


void MainFormDlgImpl::on_m_pPrevB_clicked()
{
//LAST_STEP("MainFormDlgImpl::on_m_pPrevB_clicked");
//CB_ASSERT("345" == "ab");
//traceLastStep("tsterr", 0); char* p (0); *p = 11;
//throw 1;
//int x (2), y (3); CB_ASSERT(x >= y);

    m_pCommonData->previous();
    //updateWidgets();
    m_pFilesG->setFocus();
}

void MainFormDlgImpl::on_m_pNextB_clicked()
{
    m_pCommonData->next();
    //updateWidgets();
    m_pFilesG->setFocus();
}


void MainFormDlgImpl::on_m_pTagEdtB_clicked()
{
    showBackupWarn();

    if (m_pCommonData->getViewHandlers().empty())
    {
        showCritical(this, tr("Error"), tr("The file list is empty. You need to populate it before opening the tag editor."));
        return;
    }

    m_pCommonData->setSongInCrtAlbum();

    bool bDataSaved (false);
    TagEditorDlgImpl dlg (this, m_pCommonData, m_transfConfig, bDataSaved);

    //if (QDialog::Accepted == dlg.exec())
    bool bToldAboutPatterns (m_pCommonData->m_bToldAboutPatterns);
    string strCrt (dlg.run());
    if (!bToldAboutPatterns && m_pCommonData->m_bToldAboutPatterns)
    {
        m_settings.saveMiscConfigSettings(m_pCommonData);
    }

    updateUi(strCrt); // needed because the tag editor might have called the config and changed things; it would be nicer to send a signal when config changes, but IIRC in Qt 4.3.1 resizing things in a dialog that opened another one doesn't work very well; (see also TagEditorDlgImpl::on_m_pQueryDiscogsB_clicked())

    if (m_pCommonData->useFastSave())
    {
        if (bDataSaved) { fullReload(DONT_FORCE); }
    }
    else
    {
        emit tagEditorClosed();
    }
}


void MainFormDlgImpl::on_m_pRenameFilesB_clicked()
{
    bool bUseCrtView (0 != (Qt::ControlModifier & m_pModifRenameFilesB->getModifiers()));

    if (m_pCommonData->getViewHandlers().empty())
    {
        showCritical(this, tr("Error"), tr("The file list is empty. You need to populate it before opening the file rename tool."));
        return;
    }

    m_pCommonData->setSongInCrtAlbum();

    FileRenamerDlgImpl dlg (this, m_pCommonData, bUseCrtView);

    //if (QDialog::Accepted == dlg.exec())
    string strCrt (dlg.run());

    updateUi(strCrt);
}


void MainFormDlgImpl::updateUi(const string& strCrt) // strCrt may be empty
{
    saveIgnored();

    { // custom transf
        for (int i = 0; i < CUSTOM_TRANSF_CNT; ++i) { saveCustomTransf(i); }
        setTransfTooltips();
    }

    saveVisibleTransf();
    saveExternalTools();

    if (m_pCommonData->m_bShowExport || m_pCommonData->m_bShowSessions)
    {
        m_pOptBtn1W->show();
    }
    else
    {
        m_pOptBtn1W->hide();
    }


    if (m_pCommonData->m_bShowExport)
    {
        m_pExportB->show();
        m_pExportB->parentWidget()->layout()->update(); // it is probably a Qt bug the fact that this is needed; should have been automatic;
    }
    else
    {
        m_pExportB->hide();
    }


    if (m_pCommonData->m_bShowDebug)
    {
        m_pDebugB->show();
        m_pDebugB->parentWidget()->layout()->update(); // it is probably a Qt bug the fact that this is needed; should have been automatic;
    }
    else
    {
        m_pDebugB->hide();
    }


    if (m_pCommonData->m_bShowSessions)
    {
        m_pSessionsB->show();
        m_pSessionsB->parentWidget()->layout()->update(); // it is probably a Qt bug the fact that this is needed; should have been automatic;
    }
    else
    {
        m_pSessionsB->hide();
    }


    resizeIcons();
    m_pCommonData->updateWidgets(strCrt); // needed for filters and unique notes
}


void MainFormDlgImpl::on_m_pDebugB_clicked()
{
    DebugDlgImpl dlg (this, m_pCommonData);
    dlg.run();
    m_settings.saveMiscConfigSettings(m_pCommonData);
}



void MainFormDlgImpl::on_m_pExportB_clicked()
{
    //TranslatorHandler& hndl (TranslatorHandler::getGlobalTranslator());
    //hndl.setTranslation(hndl.getTranslations().back());
    //retranslateUi(this);

    ExportDlgImpl dlg (this);
    dlg.run(); //ttt2 perhaps use ModifInfoToolButton
    m_settings.saveMiscConfigSettings(m_pCommonData);
}






void MainFormDlgImpl::on_m_pAboutB_clicked()
{
    AboutDlgImpl dlg (this);
    dlg.exec();
}


class AddrRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const { return m_spToRemove.count(p) > 0; }
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return QT_TRANSLATE_NOOP("Transformation", "Removes selected streams."); }

    static const char* getClassName() { return QT_TRANSLATE_NOOP("Transformation", "Remove selected stream(s)"); }

    set<DataStream*> m_spToRemove;
};



//void MainFormDlgImpl::onStreamsGKeyPressed(int nKey)
/*override*/ bool MainFormDlgImpl::eventFilter(QObject* pObj, QEvent* pEvent)
{
//qDebug("type %d", pEvent->type());
//if (pObj == m_pFilesG) qDebug("type %d", pEvent->type());
    QKeyEvent* pKeyEvent (dynamic_cast<QKeyEvent*>(pEvent));
    int nKey (0 == pKeyEvent ? 0 : pKeyEvent->key());
    if (m_pStreamsG == pObj && 0 != pKeyEvent && Qt::Key_Delete == nKey && QEvent::ShortcutOverride == pKeyEvent->type())
    {
        showBackupWarn();
        //showSelWarn();

    //qDebug("type %d", pKeyEvent->type());
        QItemSelectionModel* pSelModel (m_pStreamsG->selectionModel());
        QModelIndexList lstSel (pSelModel->selection().indexes());

        set<int> sStreams;
        for (QModelIndexList::iterator it = lstSel.begin(), end = lstSel.end(); it != end; ++it)
        {
            sStreams.insert(it->row());
        }
        if (sStreams.empty()) { return true; }

        AddrRemover rmv;
        for (set<int>::iterator it = sStreams.begin(), end = sStreams.end(); it != end; ++it)
        {
            rmv.m_spToRemove.insert(m_pCommonData->getCrtStreams()[*it]);
        }

        vector<Transformation*> v;
        v.push_back(&rmv);
        transform(v, CURRENT);
        return true;
    }
    else if (m_pFilesG == pObj && 0 != pKeyEvent && Qt::Key_Delete == nKey && QEvent::ShortcutOverride == pKeyEvent->type())
    {
        const deque<const Mp3Handler*>& vpSelHandlers (m_pCommonData->getSelHandlers());
        if (askConfirm(vpSelHandlers, tr("Delete %1?")))
        {
            for (int i = 0; i < cSize(vpSelHandlers); ++i)
            {
                try
                {
                    deleteFile(vpSelHandlers[i]->getName());
                }
                catch (const CannotDeleteFile&)
                {
                    showCritical(this, tr("Error"), tr("Cannot delete file %1").arg(convStr(vpSelHandlers[i]->getName())));
                    break;
                }
            }
            reload(IGNORE_SEL, DONT_FORCE);
        }

        return true;
    }
    else if (pObj == m_pFilesG)
    {
        //qDebug("type %d", pEvent->type());
        QContextMenuEvent* pCtx (dynamic_cast<QContextMenuEvent*>(pEvent));
        if (0 != pCtx)
        {
            m_nGlobalX = pCtx->globalX();
            m_nGlobalY = pCtx->globalY();
            QTimer::singleShot(1, this, SLOT(onMainGridRightClick()));
        }
    }
    return QDialog::eventFilter(pObj, pEvent);
}


void MainFormDlgImpl::on_m_pViewFileInfoB_clicked()
{
    m_pLowerHalfLayout->setCurrentWidget(m_pFileInfoTab);
    m_pViewFileInfoB->setChecked(true); //ttt2 use autoExclusive instead
    m_pViewAllNotesB->setChecked(false);
    m_pViewTagDetailsB->setChecked(false);
    m_pNotesG->resizeRowsToContents();
    m_pStreamsG->resizeRowsToContents();
}


void MainFormDlgImpl::on_m_pViewAllNotesB_clicked()
{
    m_pLowerHalfLayout->setCurrentWidget(m_pAllNotesTab);
    m_pViewFileInfoB->setChecked(false);
    m_pViewAllNotesB->setChecked(true);
    m_pViewTagDetailsB->setChecked(false);
    m_pUniqueNotesG->resizeRowsToContents();
}


void MainFormDlgImpl::on_m_pViewTagDetailsB_clicked()
{
    m_pLowerHalfLayout->setCurrentWidget(m_pTagDetailsTab);
    m_pViewFileInfoB->setChecked(false);
    m_pViewAllNotesB->setChecked(false);
    m_pViewTagDetailsB->setChecked(true);

    onCrtFileChanged(); // to populate m_pTagDetailsW
}


void MainFormDlgImpl::resizeIcons()
{
    vector<QToolButton*> v;
    v.push_back(m_pScanB);
    v.push_back(m_pSessionsB);
    v.push_back(m_pNoteFilterB);
    v.push_back(m_pDirFilterB);
    v.push_back(m_pModeAllB);
    v.push_back(m_pModeAlbumB);
    v.push_back(m_pModeSongB);
    v.push_back(m_pPrevB);
    v.push_back(m_pNextB);
    v.push_back(m_pTransformB);
    v.push_back(m_pCustomTransform1B);
    v.push_back(m_pCustomTransform2B);
    v.push_back(m_pCustomTransform3B);
    v.push_back(m_pCustomTransform4B);
    v.push_back(m_pTagEdtB);
    v.push_back(m_pNormalizeB);
    v.push_back(m_pRenameFilesB);
    v.push_back(m_pReloadB);
    v.push_back(m_pConfigB);
    v.push_back(m_pDebugB);
    v.push_back(m_pAboutB);
    v.push_back(m_pExportB);

    int k (m_pCommonData->m_nMainWndIconSize);
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        QToolButton* p (v[i]);
        p->setMaximumSize(k, k);
        p->setMinimumSize(k, k);
        p->setIconSize(QSize(k - 4, k - 4));
    }
}



void MainFormDlgImpl::on_m_pSessionsB_clicked()
{
    closeEvent(0);
    accept();
}




void MainFormDlgImpl::checkForNewVersion() // returns immediately; when the request completes it will send a signal
{
    const int MIN_INTERVAL_BETWEEN_CHECKS (24); // hours

    if ("yes" != m_pCommonData->m_strCheckForNewVersions && "no" != m_pCommonData->m_strCheckForNewVersions)
    {
        int nRes (HtmlMsg::msg(this, 0, 0, 0, HtmlMsg::DEFAULT, tr("Info"),
        "<p style=\"margin-bottom:1px; margin-top:12px; \">" + tr("MP3 Diags can check at startup if a new version of the program has been released. Here's how this is supposed to work:") +
            "<ul>"
                "<li>" + tr("The check is done in the background, when the program starts, so there should be no performance penalties") + "</li>"
                "<li>" + tr("A notification message is displayed only if there's a new version available") + "</li>"
                "<li>" + tr("The update is manual. You are told that there is a new version and are offered links to see what's new, but nothing gets downloaded and / or installed automatically") + "</li>"
                "<li>" + tr("There is no System Tray process checking periodically for updates") + "</li>"
                "<li>" + tr("You can turn the notifications on and off from the configuration dialog") + "</li>"
                "<li>" + tr("If you restart the program within a day after a check, no new check is done") + "</li>"
            "</ul>"
        "</p>"
        /*"
        "<p style=\"margin-bottom:1px; margin-top:12px; \">QQQ</p>"
        "<p style=\"margin-bottom:1px; margin-top:12px; \">QQQ</p>"
        "<p style=\"margin-bottom:1px; margin-top:12px; \">QQQ</p>"
        */
        , 750, 300, tr("Disable checking for new versions"), tr("Enable checking for new versions")));
        qDebug("ret %d", nRes);

        m_pCommonData->m_strCheckForNewVersions = (1 == nRes ? "yes" : "no");
        m_settings.saveMiscConfigSettings(m_pCommonData);
    }

    if ("yes" != m_pCommonData->m_strCheckForNewVersions) { return; }

    QDateTime t1 (QDateTime::currentDateTime());
    t1 = t1.addSecs(-MIN_INTERVAL_BETWEEN_CHECKS*3600);
    //qDebug("ini: %s, crt: %s", m_pCommonData->m_timeLastNewVerCheck.toString().toUtf8().constData(), t1.toString().toUtf8().constData());
    if (t1 < m_pCommonData->m_timeLastNewVerCheck)
    {
        return;
    }

    m_pCommonData->m_timeLastNewVerCheck = QDateTime(QDateTime::currentDateTime());

    m_pQHttp = new QHttp (this);

    connect(m_pQHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(onNewVersionQueryFinished(int, bool)));
    //connect(m_pQHttp, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(readResponseHeader(const QHttpResponseHeader&)));

    m_pQHttp->setHost("mp3diags.sourceforge.net");
    //http://mp3diags.sourceforge.net/010_getting_the_program.html
    QHttpRequestHeader header ("GET", QString(getWebBranch()) + "/version.txt"); header.setValue("Host", "mp3diags.sourceforge.net");
    //QHttpRequestHeader header ("GET", "/mciobanu/mp3diags/010_getting_the_program.html"); header.setValue("Host", "web.clicknet.ro");
    m_pQHttp->request(header);
}
//mp3diags.sourceforge.net/010_getting_the_program.html
//web.clicknet.ro/mciobanu/mp3diags/010_getting_the_program.html

void MainFormDlgImpl::readResponseHeader(const QHttpResponseHeader& h)
{
    qDebug("status %d", h.statusCode());
}




void MainFormDlgImpl::onNewVersionQueryFinished(int /*nId*/, bool bError)
{
    if (bError)
    { //ttt2 log something, tell user after a while
        qDebug("HTTP error");
        return;
    }

    qint64 nAv (m_pQHttp->bytesAvailable());
    if (0 == nAv)
    {
        // !!! JUST return; empty responses come for no identifiable requests and they should be just ignored
        return;
    }

    QByteArray b (m_pQHttp->readAll());
    CB_ASSERT (b.size() == nAv);

    qDebug("ver: %s", b.constData());

    m_qstrNewVer = b;
    m_qstrNewVer = m_qstrNewVer.trimmed();

    if (m_qstrNewVer.size() > 50)
    { //ttt2
        return; // most likely some error message
    }

    if (getAppVer() == m_qstrNewVer)
    {
        return;
    }

#if 0
    //const int WAIT (15); //crashes
    //const int WAIT (12); // doesn't crash
    const int WAIT (14); //crashes
#ifndef WIN32
    qDebug("wait %d seconds", WAIT); sleep(WAIT); showCritical(this, "resume", "resume resume resume resume resume"); // crashes
    //ttt2 see if this crash affects discogs dwnld //??? why doesn't crash if no message is shown?
#else
    //Sleep(WAIT*1000); // Qt 4.5 doesn't seem to crash
#endif
#endif

    QTimer::singleShot(1, this, SLOT(onNewVersionQueryFinished2()));
}


void MainFormDlgImpl::onNewVersionQueryFinished2()
{
    //QMessageBox::critical(this, "wait", "bb urv huervhuervhuerv erve rvhu ervervhuer vher vrhe rv evr ev erv erv");
    //qDebug("ver: %s", b.constData());
    //QMessageBox::critical(this, "resume", "bb urv huervhuervhuerv erve rvhu ervervhuer vher vrhe rv evr ev erv erv");
    if (m_pCommonData->m_strDontTellAboutVer == convStr(m_qstrNewVer)) { return; }

    QString qstrMsg;

    qstrMsg = "<p style=\"margin-bottom:1px; margin-top:12px; \">" +
            tr("Version %1 has been published. You are running %2. You can see what's new in %3. A more technical list with changes can be seen in %4.")
            .arg(m_qstrNewVer)
            .arg(getSimpleAppVer())
            .arg(
                tr("the %1MP3 Diags blog%2", "arguments are HTML elements")
                .arg("<a href=\"http://mp3diags.blogspot.com/\">")
                .arg("</a>"))
            .arg(
                tr("the %1change log%2", "arguments are HTML elements")
                .arg("<a href=\"http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/015_changelog.html\">")
                .arg("</a>"))
            + "</p>";


#ifndef WIN32
    qstrMsg += "<p style=\"margin-bottom:1px; margin-top:12px; \">" + tr("This notification is about the availability of the source code. Binaries may or may not be available at this time, depending on your particular platform.") + "</p>";
#else
#endif
    qstrMsg += "<p style=\"margin-bottom:1px; margin-top:12px; \">" + tr("You should review the changes and decide if you want to upgrade or not.") + "</p>";
    qstrMsg += "<p style=\"margin-bottom:1px; margin-top:12px; \">" + tr("Note: if you want to upgrade, you should %1close MP3 Diags%2 first.", "arguments are HTML elements").arg("<b>").arg("</b>") + "</p>";
    qstrMsg += "<hr/><p style=\"margin-bottom:1px; margin-top:12px; \">" + tr("Choose what do you want to do:") + "</p>";
    /*"<p style=\"margin-bottom:1px; margin-top:12px; \">QQQ</p>"*/

    int nRes (HtmlMsg::msg(this, 0, 0, 0, HtmlMsg::VERT_BUTTONS, tr("Info"), qstrMsg
        , 600, 400, tr("Just close this message"), tr("Don't tell me about version %1 again").arg(m_qstrNewVer), tr("Disable checking for new versions")));

    //qDebug("ret %d", nRes);
    switch (nRes)
    {
    case 0: m_pCommonData->m_strDontTellAboutVer.clear(); break;
    case 1: m_pCommonData->m_strDontTellAboutVer = convStr(m_qstrNewVer); break;
    case 2: m_pCommonData->m_strDontTellAboutVer.clear(); m_pCommonData->m_strCheckForNewVersions = "no"; break;
    default: CB_ASSERT (false);
    }

    m_settings.saveMiscConfigSettings(m_pCommonData);
}



//=============================================================================================================================
//=============================================================================================================================
//=============================================================================================================================


namespace
{
    struct CmpTransfAndName
    {
        const char* m_szName;
        CmpTransfAndName(const char* szName) : m_szName(szName) {}

        bool operator()(const Transformation* p) const
        {
            return 0 == strcmp(p->getActionName(), m_szName);
        }
    };


    class FixedAddrRemover : public GenericRemover
    {
        Q_DECLARE_TR_FUNCTIONS(FixedAddrRemover)
        /*override*/ bool matches(DataStream* p) const
        {
            return m_pStream == p; // !!! normally there might be an issue with comparing pointers, in case something else gets allocated at the same address as a deleted object; however, in this case the stream passed on setStream() doesn't get destroyed
        }
        streampos m_pos;
        QString m_qstrAction;
        const DataStream* m_pStream;
    public:
        FixedAddrRemover() : m_pos(-1), m_pStream(0) {}

        void setStream(const DataStream* p)
        {
            m_pStream = p;
            m_pos = p->getPos();
            m_qstrAction = tr("Remove stream %1 at address 0x%2").arg(p->getTranslatedDisplayName()).arg(m_pos, 0, 16);
        }

        /*override*/ const char* getActionName() const { return getClassName(); }
        /*override*/ QString getVisibleActionName() const { return m_qstrAction; }
        /*override*/ const char* getDescription() const { return QT_TRANSLATE_NOOP("Transformation", "Removes specified stream."); }

        static const char* getClassName() { return QT_TRANSLATE_NOOP("Transformation", "Remove specified stream"); }
    };
}


vector<Transformation*> MainFormDlgImpl::getFixes(const Note* pNote, const Mp3Handler* pHndl) const // what might fix a note
{
//qDebug("strm %s", pStream ? pStream->getDisplayName() : "");
    //CB_ASSERT (0 == pStream || (pNote->getPos() >= pStream->getPos() && pNote->getPos() < pStream->getPos() + pStream->getSize()));
    //CB_ASSERT (0 == pHndl || -1 != pNote->getPos());

    static map<int, vector<Transformation*> > s_mFixes;
    static bool s_bInitialized (false);
    const vector<Transformation*>& vpAllTransf (m_pCommonData->getAllTransf());

    if (!s_bInitialized)
    {
        s_bInitialized = true;

        vector<Transformation*>::const_iterator it;

        #define ADD_FIX(NOTE, TRANSF) \
        it = find_if(vpAllTransf.begin(), vpAllTransf.end(), CmpTransfAndName(TRANSF::getClassName())); \
        CB_ASSERT (vpAllTransf.end() != it); \
        s_mFixes[Notes::NOTE().getNoteId()].push_back(*it);

        ADD_FIX(twoAudio, InnerNonAudioRemover);
        ADD_FIX(twoAudio, SingleBitRepairer);
        ADD_FIX(incompleteFrameInAudio, TruncatedMpegDataStreamRemover);
        ADD_FIX(incompleteFrameInAudio, TruncatedAudioPadder);

        //ADD_FIX(twoLame, VbrRepairer);
        //ADD_FIX(twoLame, VbrRebuilder);

        ADD_FIX(xingAddedByMp3Fixer, VbrRepairer);
        ADD_FIX(xingFrameCountMismatch, VbrRepairer);
        ADD_FIX(xingFrameCountMismatch, MismatchedXingRemover);
        //ADD_FIX(twoXing, VbrRepairer); //ttt2
        ADD_FIX(xingNotBeforeAudio, VbrRepairer);
        ADD_FIX(incompatXing, VbrRebuilder);
        ADD_FIX(missingXing, VbrRebuilder);

        ADD_FIX(vbriFound, VbrRepairer);
        ADD_FIX(foundVbriAndXing, VbrRepairer);

        ADD_FIX(id3v2FrameTooShort, Id3V2Rescuer);
        //ADD_FIX(id3v2FrameTooShort, Id3V2Cleaner);
        ADD_FIX(id3v2InvalidName, Id3V2Rescuer);
        //ADD_FIX(id3v2InvalidName, Id3V2Cleaner);
        ADD_FIX(id3v2TextError, Id3V2Rescuer);
        //ADD_FIX(id3v2TextError, Id3V2Cleaner);
        ADD_FIX(id3v2HasLatin1NonAscii, Id3V2UnicodeTransformer);
        ADD_FIX(id3v2EmptyTcon, Id3V2Rescuer);
        //ADD_FIX(id3v2EmptyTcon, Id3V2Cleaner);
        ADD_FIX(id3v2MultipleFramesWithSameName, Id3V2Rescuer);
        ADD_FIX(id3v2MultipleFramesWithSameName, Id3V2Cleaner);
        ADD_FIX(id3v2PaddingTooLarge, Id3V2Compactor);
        ADD_FIX(id3v2UnsuppVer, UnsupportedId3V2Remover);
        ADD_FIX(id3v2UnsuppFlag, UnsupportedDataStreamRemover);
        ADD_FIX(id3v2UnsuppFlags1, UnsupportedDataStreamRemover);
        ADD_FIX(id3v2UnsuppFlags2, UnsupportedDataStreamRemover);

        ADD_FIX(id3v2CouldntLoadPic, Id3V2Rescuer);
        //ADD_FIX(id3v2CouldntLoadPic, Id3V2Cleaner);
        ADD_FIX(id3v2NotCoverPicture, SmallerImageRemover);
        ADD_FIX(id3v2ErrorLoadingApic, Id3V2Rescuer);
        //ADD_FIX(id3v2ErrorLoadingApic, Id3V2Cleaner);
        ADD_FIX(id3v2ErrorLoadingApicTooShort, Id3V2Rescuer);
        //ADD_FIX(id3v2ErrorLoadingApicTooShort, Id3V2Cleaner);
        ADD_FIX(id3v2DuplicatePic, Id3V2Rescuer);
        ADD_FIX(id3v2DuplicatePic, Id3V2Cleaner);
        ADD_FIX(id3v2DuplicatePic, SmallerImageRemover);
        ADD_FIX(id3v2MultipleApic, Id3V2Rescuer);
        ADD_FIX(id3v2MultipleApic, Id3V2Cleaner);
        ADD_FIX(id3v2MultipleApic, SmallerImageRemover);
        ADD_FIX(id3v2UnsupApicTextEnc, Id3V2Rescuer);
        ADD_FIX(id3v2UnsupApicTextEnc, Id3V2Cleaner);
        ADD_FIX(id3v2LinkInApic, Id3V2Rescuer);
        ADD_FIX(id3v2LinkInApic, Id3V2Cleaner);

        ADD_FIX(twoId3V230, MultipleId3StreamRemover);
        ADD_FIX(bothId3V230_V240, MultipleId3StreamRemover);
        ADD_FIX(id3v230UsesUtf8, Id3V2Rescuer);
        //ADD_FIX(id3v230UsesUtf8, Id3V2Cleaner);

        ADD_FIX(twoId3V240, MultipleId3StreamRemover);
        ADD_FIX(id3v240IncorrectSynch, Id3V2Rescuer);
        ADD_FIX(id3v240IncorrectSynch, Id3V2Cleaner);

        ADD_FIX(id3v240DeprTyerAndTdrc, Id3V2Rescuer);
        ADD_FIX(id3v240DeprTyerAndTdrc, Id3V2Cleaner);
        ADD_FIX(id3v240DeprTyer, Id3V2Rescuer);
        ADD_FIX(id3v240DeprTyer, Id3V2Cleaner);
        ADD_FIX(id3v240DeprTdatAndTdrc, Id3V2Rescuer);
        ADD_FIX(id3v240DeprTdatAndTdrc, Id3V2Cleaner);
        ADD_FIX(id3v240DeprTdat, Id3V2Rescuer);
        ADD_FIX(id3v240DeprTdat, Id3V2Cleaner);

        ADD_FIX(twoId3V1, MultipleId3StreamRemover);

        ADD_FIX(brokenAtTheEnd, BrokenDataStreamRemover);
        ADD_FIX(brokenInTheMiddle, BrokenDataStreamRemover);

        ADD_FIX(truncAudioWithWholeFile, TruncatedMpegDataStreamRemover);
        ADD_FIX(truncAudioWithWholeFile, TruncatedAudioPadder);
        ADD_FIX(truncAudio, TruncatedMpegDataStreamRemover);
        ADD_FIX(truncAudio, TruncatedAudioPadder);

        ADD_FIX(unknownAtTheEnd, UnknownDataStreamRemover);
        ADD_FIX(unknownInTheMiddle, UnknownDataStreamRemover);
        ADD_FIX(foundNull, NullStreamRemover);
    }

    vector<Transformation*> vpTransf (s_mFixes[pNote->getNoteId()]);


/*    if (0 != pStream)
    {
        if (pNote->getNoteId() == Notes::audioTooShort().getNoteId())
        {
            if (pStream->getDisplayName() == TruncatedMpegDataStream::getClassDisplayName())
            {
                vector<Transformation*>::const_iterator it (find_if(vpAllTransf.begin(), vpAllTransf.end(), CmpTransfAndName(TruncatedMpegDataStreamRemover::getClassName())));
                CB_ASSERT (vpAllTransf.end() != it);
                vpTransf.push_back(*it);
            }
        }
    }*/

    #define ADD_CUSTOM_FIX(NOTE, STREAM, TRANSF) \
    if (pNote->getNoteId() == Notes::NOTE().getNoteId()) \
    { \
        if (pCrtStream->getDisplayName() == STREAM::getClassDisplayName()) \
        { \
            vector<Transformation*>::const_iterator it (find_if(vpAllTransf.begin(), vpAllTransf.end(), CmpTransfAndName(TRANSF::getClassName()))); \
            CB_ASSERT (vpAllTransf.end() != it); \
            Transformation* pTransf (*it); \
            if (0 == spTransf.count(pTransf)) \
            { \
                vpTransf.push_back(pTransf); \
                spTransf.insert(pTransf); \
            } \
        } \
    }


    //ttt2 None of the ADD_CUSTOM_FIX fixes is shown for header (e.g. SingleBitRepairer is shown for validFrameDiffVer only when clicking on circle, not when clicking on header), maybe we should drop the stream test; OTOH the case of audioTooShort shows that it matters what stream the error is occuring on; so maybe drop the stream check only for some ... // workaround: see what's available for single song, then use the menu for all;
    //ttt2 other example: mismatched xing fixable by SingleBitRepairer to other stream; probably document that all the notes should be looked at;
    // or: add all transforms that in some context might fix a note
    // or: extend ADD_CUSTOM_FIX to look at the other notes, similarly to how it looks at the stream it's in; add some transform if some other note is present; (note: use a set to not have duplicates entered via different rules)
    // perhaps unknown size 16 between xing and audio -> repair vbr, but seems too specific
    // unknown between audio and audio -> restore flipped
    //ttt2 perhaps just this: if there's a chance that by using transf T the note N will disappear, show it; well, this is again context-dependant

    if (0 != pHndl)
    { // for each matching note, see if additional fixes exist that take into account the stream the note is in and (in the future) other streams, their sizes, ...
        const vector<Note*>& vpHndlNotes (pHndl->getNotes().getList());
        const vector<DataStream*>& vpStreams (pHndl->getStreams());

        set<Transformation*> spTransf (vpTransf.begin(), vpTransf.end());

        static vector<FixedAddrRemover> s_vFixedAddrRemovers;
        s_vFixedAddrRemovers.clear();
        set<const DataStream*> spRemovableStreams;

        for (int i = 0; i < cSize(vpHndlNotes); ++i)
        {
            if (-1 == vpHndlNotes[i]->getNoteId()) { goto e1; } // takes care of trace notes

            const Note* pCrtNote (vpHndlNotes[i]);

            if (pCrtNote->getNoteId() == pNote->getNoteId() && -1 != pCrtNote->getPos())
            {
                for (int i = 0, n = cSize(vpStreams); i < n; ++i)
                {
                    //qDebug("s %s", v[i]->getDisplayName());
                    if (n - 1 == i || pCrtNote->getPos() < vpStreams[i + 1]->getPos()) //ttt2 lower_bound
                    {
                        const DataStream* pCrtStream (vpStreams[i]);
                        CB_ASSERT (pCrtNote->getPos() >= pCrtStream->getPos() && pCrtNote->getPos() < pCrtStream->getPos() + pCrtStream->getSize());

                        ADD_CUSTOM_FIX(validFrameDiffVer, UnknownDataStream, SingleBitRepairer);
                        ADD_CUSTOM_FIX(validFrameDiffLayer, UnknownDataStream, SingleBitRepairer);
                        ADD_CUSTOM_FIX(validFrameDiffMode, UnknownDataStream, SingleBitRepairer);
                        ADD_CUSTOM_FIX(validFrameDiffFreq, UnknownDataStream, SingleBitRepairer);
                        ADD_CUSTOM_FIX(validFrameDiffCrc, UnknownDataStream, SingleBitRepairer);

                        ADD_CUSTOM_FIX(audioTooShort, TruncatedMpegDataStream, TruncatedMpegDataStreamRemover);
                        ADD_CUSTOM_FIX(audioTooShort, TruncatedMpegDataStream, TruncatedAudioPadder);

                        ADD_CUSTOM_FIX(audioTooShort, UnknownDataStream, UnknownDataStreamRemover);
                        ADD_CUSTOM_FIX(audioTooShort, UnknownDataStream, SingleBitRepairer);

                        // ADD_CUSTOM_FIX(brokenInTheMiddle, ??? , Id3V2Rescuer); //ttt2 see about this


                        if (pCrtNote->allowErase() && 0 == spRemovableStreams.count(pCrtStream))
                        {
                            spRemovableStreams.insert(pCrtStream);
                            s_vFixedAddrRemovers.push_back(FixedAddrRemover());
                            s_vFixedAddrRemovers.back().setStream(pCrtStream);
                        }

                        break;
                    }
                }
            }
        }
e1:;
        for (int i = 0; i < cSize(s_vFixedAddrRemovers); ++i)
        {
            vpTransf.push_back(&s_vFixedAddrRemovers[i]);
        }
    }


    return vpTransf;
}


int getHeaderDrawOffset();


void MainFormDlgImpl::onMainGridRightClick()
{
    QPoint coords (m_pFilesG->mapFromGlobal(QPoint(m_nGlobalX, m_nGlobalY)));
    int nCol (m_pFilesG->columnAt(coords.x() - m_pFilesG->verticalHeader()->width()));
    if (nCol >= 1)
    {
        fixCurrentNote(coords);
        return;
    } // header or file name

    if (0 == nCol && coords.y() >= m_pFilesG->horizontalHeader()->height())
    {
        showExternalTools();
    }
}

void MainFormDlgImpl::fixCurrentNote(const QPoint& coords)
{
LAST_STEP("MainFormDlgImpl::onFixCurrentNote()");
    //QPoint coords (m_pFilesG->mapFromGlobal(QPoint(m_nGlobalX, m_nGlobalY)));
    //int nHorHdrHght ();
    //if (coords.x() < nVertHdrWdth) { return; }
    int nCol (m_pFilesG->columnAt(coords.x() - m_pFilesG->verticalHeader()->width()));
    //if (nCol < 1) { return; } // header or file name
    CB_ASSERT(nCol >= 1);

    if (coords.y() < m_pFilesG->horizontalHeader()->height())
    {
        int x (coords.x());
        x -= 2; // hard-coded
        x += getHeaderDrawOffset() / 2; //ttt2 this "/2" doesn't make sense but it works best
        //qDebug("offs %d", getHeaderDrawOffset());
        nCol = m_pFilesG->columnAt(x - m_pFilesG->verticalHeader()->width());

        if (nCol != m_pFilesG->columnAt(x - m_pFilesG->verticalHeader()->width() - 3) ||
                nCol != m_pFilesG->columnAt(x - m_pFilesG->verticalHeader()->width() + 3))
        { // too close to adjacent cells; return to avoid confusions
            return;
        }

        fixCurrentNoteAllFiles(nCol - 1);
    }
    else
    {
        fixCurrentNoteOneFile();
    }

    //qDebug("r %d, c %d", m_pFilesG->rowAt(coords.y() - nHorHdrHght), );
}


void MainFormDlgImpl::fixCurrentNoteOneFile()
{
    QModelIndex ndx (m_pFilesG->currentIndex());
    if (!ndx.isValid() || 0 == ndx.column()) { return; }

    //qDebug("fixCurrentNoteOneFile %d %d", nGlobalX, nGlobalY);
    const Mp3Handler* pHndl (m_pCommonData->getCrtMp3Handler());

    const vector<const Note*>& vpNotes (m_pCommonData->getUniqueNotes().getFltVec());
    const Note* pNote (vpNotes.at(ndx.column() - 1));
    //qDebug("fixing note '%s' for file '%s'", pNote->getDescription(), pHndl->getName().c_str());

    //int nCnt (0);

    vector<Transformation*> vpTransf (getFixes(pNote, pHndl));
    if (vpTransf.empty()) { return; }

    showFixes(vpTransf, CURRENT);

}


void MainFormDlgImpl::fixCurrentNoteAllFiles(int nCol)
{
    const vector<const Note*>& vpNotes (m_pCommonData->getUniqueNotes().getFltVec());
    const Note* pNote (vpNotes.at(nCol));

    vector<Transformation*> vpTransf (getFixes(pNote, 0));
    if (vpTransf.empty()) { return; }

    showFixes(vpTransf, ALL);
}


void MainFormDlgImpl::showFixes(vector<Transformation*>& vpTransf, Subset eSubset)
{
    ModifInfoMenu menu;
    vector<QAction*> vpAct;

    for (int i = 0, n = cSize(vpTransf); i < n; ++i)
    {
        Transformation* pTransf (vpTransf[i]);
        QAction* pAct (new QAction(pTransf->getVisibleActionName(), &menu));
        pAct->setToolTip(makeMultiline(Transformation::tr(pTransf->getDescription())));

        //connect(pAct, SIGNAL(triggered()), this, SLOT(onExecTransform(i))); // !!! Qt doesn't seem to support parameter binding
        menu.addAction(pAct);
        vpAct.push_back(pAct);
    }

    connect(&menu, SIGNAL(hovered(QAction*)), this, SLOT(onMenuHovered(QAction*)));

    //QAction* p (menu.exec(m_pTransformB->mapToGlobal(QPoint(0, m_pTransformB->height()))));
    QAction* p (menu.exec(QPoint(m_nGlobalX, m_nGlobalY + 10)));
    if (0 != p)
    {
        int nIndex (std::find(vpAct.begin(), vpAct.end(), p) - vpAct.begin());
        vector<Transformation*> v;
        v.push_back(vpTransf.at(nIndex));

        if (ALL == eSubset)
        {
            eSubset = 0 == (Qt::ShiftModifier & menu.getModifiers()) ? ALL : SELECTED;
        }

        transform(v, eSubset);
    }
}


void MainFormDlgImpl::showExternalTools()
{
    ModifInfoMenu menu;
    vector<QAction*> vpAct;

    QAction* pAct (new QAction(tr("Open containing folder ..."), &menu));
    menu.addAction(pAct);
    vpAct.push_back(pAct);

    if (!m_pCommonData->m_vExternalToolInfos.empty())
    {
        menu.addSeparator();
    }
    for (int i = 0; i < cSize(m_pCommonData->m_vExternalToolInfos); ++i)
    {
        QAction* pAct (new QAction(convStr(m_pCommonData->m_vExternalToolInfos[i].m_strName), &menu));
        menu.addAction(pAct);
        vpAct.push_back(pAct);
    }

    QAction* p (menu.exec(QPoint(m_nGlobalX, m_nGlobalY + 10)));
    if (0 != p)
    {
        int nIndex (std::find(vpAct.begin(), vpAct.end(), p) - vpAct.begin());
        //qDebug("pressed %d", nIndex);
        if (0 == nIndex)
        {
            CB_ASSERT (0 != m_pCommonData->getCrtMp3Handler());
            QString qstrDir (convStr(m_pCommonData->getCrtMp3Handler()->getDir()));
#if defined(WIN32) || defined(__OS2__)
            //qstrDir = QDir::toNativeSeparators(qstrDir);
            QDesktopServices::openUrl(QUrl("file:///" + qstrDir, QUrl::TolerantMode));
#else
            QDesktopServices::openUrl(QUrl("file://" + qstrDir, QUrl::TolerantMode));
#endif
        }
        else
        { // ttt1 copied from void MainFormDlgImpl::transform(std::vector<Transformation*>& vpTransf, Subset eSubset)
            const ExternalToolInfo& info (m_pCommonData->m_vExternalToolInfos[nIndex - 1]);
            Subset eSubset (0 != (Qt::ControlModifier & menu.getModifiers()) ? ALL : SELECTED); //ttt0 it's confusing that the external tools apply to selected files by default (you have to press CTRL to get all) while transformations apply to all files by default (you have to right-click to process selected ones; OTOH it seems to make sense that the default for transforms to be all the files while the default for external tools to be a single file
            //deque<const Mp3Handler*> vpCrt;
            const deque<const Mp3Handler*>* pvpHandlers;
            switch (eSubset)
            {
            case SELECTED: pvpHandlers = &m_pCommonData->getSelHandlers(); break;
            case ALL: pvpHandlers = &m_pCommonData->getViewHandlers(); break;
            //case CURRENT: vpCrt.push_back(m_pCommonData->getCrtMp3Handler()); pvpHandlers = &vpCrt; break;
            default: CB_ASSERT (false);
            }
            //qDebug("ctrl=%d", eSubset);

            QString qstrAction (tr("Run \"%1\" on %2?").arg(convStr(info.m_strName)));
            qstrAction.replace("%2", "%1");

            if (info.m_bConfirmLaunch && !askConfirm(*pvpHandlers, qstrAction))
            {
                return;
            }

            QStringList lFiles;
            for (int i = 0; i < cSize(*pvpHandlers); ++i)
            {
                lFiles << convStr((*pvpHandlers)[i]->getName());
            }
            switch (info.m_eLaunchOption)
            {
            case ExternalToolInfo::WAIT_AND_KEEP_WINDOW_OPEN:
            case ExternalToolInfo::WAIT_THEN_CLOSE_WINDOW:
                {
                    ExternalToolDlgImpl dlg (this, info.m_eLaunchOption == ExternalToolInfo::WAIT_AND_KEEP_WINDOW_OPEN, m_settings, m_pCommonData, info.m_strName, "299_ext_tools.html");
                    dlg.run(convStr(info.m_strCommand), lFiles);
                }
                break;
            case ExternalToolInfo::DONT_WAIT:
                {
                    QString qstrProg;
                    QStringList lArgs;
                    ExternalToolDlgImpl::prepareArgs(convStr(info.m_strCommand), lFiles, qstrProg, lArgs);
                    if (!QProcess::startDetached(qstrProg, lArgs))
                    {
                        showCritical(this, tr("Error"), tr("Cannot start process. Check that the executable name and the parameters are correct.")); //ttt0 add process name
                    }
                }
                break;
            }

            //l << "p1" << "p 2" << "p:3'" << "p\"4" << "p5";
            //QProcess proc (this);
            //proc.startDetached("konqueror");
            //proc.start("konqueror"); PausableThread::sleep(10);
            //proc.start("ParamsGui", l); PausableThread::sleep(10);
            //proc.startDetached("ParamsGui", l);

            //proc.kill();
        }
    }
    //add external tools
}


bool MainFormDlgImpl::askConfirm(const deque<const Mp3Handler*>& vpHandlers, const QString& qstrAction)
{
    if (vpHandlers.empty())
    {
        return false;
    }

    QString qstrList;


    if (vpHandlers.size() == 1)
    {
        qstrList = convStr(vpHandlers[0]->getShortName());
    }
    else if (vpHandlers.size() == 2)
    {
        qstrList = tr("%1 and %2").arg(convStr(vpHandlers[0]->getShortName())).arg(convStr(vpHandlers[1]->getShortName()));
    }
    else if (vpHandlers.size() == 3)
    {
        qstrList = tr("%1, %2 and %3").arg(convStr(vpHandlers[0]->getShortName())).arg(convStr(vpHandlers[1]->getShortName())).arg(convStr(vpHandlers[2]->getShortName()));
    }
    else
    {
        qstrList = tr("%1, %2 and %3 other files").arg(convStr(vpHandlers[0]->getShortName())).arg(convStr(vpHandlers[1]->getShortName())).arg((int)vpHandlers.size() - 2);
    }

    return showMessage(this, QMessageBox::Question, 1, 1, tr("Confirm"), qstrAction.arg(qstrList), tr("&Yes"), tr("&No")) == 0;
}

//=============================================================================================================================
//=============================================================================================================================
//=============================================================================================================================


void MainFormDlgImpl::testSlot()
{
    static int c (0);
    qDebug("%d", c++);
}





//=============================================================================================================================
//=============================================================================================================================
//=============================================================================================================================





struct TestThread01 : public PausableThread
{
    /*override*/ void run()
    {
        CompleteNotif notif(this);
        for (int i = 0; i < 7; ++i)
        {
            if (isAborted()) return;
            checkPause();

            QString s;
            s.sprintf("step %d", i);
            StrList l;
            l.push_back(s);
            emit stepChanged(l, -1);
//qDebug("step %d", i);

            sleep(1);
        }

        notif.setSuccess(true);
    }
};









//=============================================================================================================================
//=============================================================================================================================
//=============================================================================================================================





/*
ttt2 To look at:
QDir
QDirModel

QFontComboBox

QStyle ; ./myapplication -style motif

*/

//ttt2 ? option to discard errors in unknown streams: probably not; at any rate, that's the only chance to learn that there was an error there (instead of a really unknown stream)





// ttt2 make sure that everything that doesn't take a "parent" on the constructor gets deleted;



/*

Development machine:

-style=Plastique
-style=Cleanlooks
-style=CDE
-style=Motif
-style=Oxygen

?? Windows, Windows Vista, Plastik




*/

//ttt2 build: not sure is possible, but have the .pro file include another file, which is generated by a "pre-config" script, which figures out if the lib is installed

//ttt2 perhaps a "consider unknown" function for streams; then it would be possible for a truncated id3v1 tag that seems ok to be marked as unknown, thus allowing the following tag's start to be detected (see "c09 Mark Knopfler - The Long Road.mp3")


//ttt2 handle symbolic links to ancestors


//ttt2 fix on right-click for notes table





//ttt0 look at /d/test_mp3/1/tmp4/tmp2/unsupported/bad-decoding

// favicon.ico : not sure how to create it; currently it's done with GIMP, with 8bpp/1 bit alpha, not compressed; however, konqueror doesn't show it when using a local page

//ttt0 run CppCheck






//ttt1 warn when folders are missing (perhaps as network drives are not mounted, usb sticks not inserted, ...), to avoid erasing the database




//ttt0 link from stable to unstable in doc. perhaps also have a notification popup


/*

  ttt0:
http://doc.qt.nokia.com/4.7-snapshot/internationalization.html

Use QKeySequence() for Accelerator Values
Accelerator values such as Ctrl+Q or Alt+F need to be translated too. If you hardcode Qt::CTRL + Qt::Key_Q for "quit" in your application, translators won't be able to override it. The correct idiom is
     exitAct = new QAction(tr("E&xit"), this);
     exitAct->setShortcuts(QKeySequence::Quit);



Typically, your application's main() function will look like this:
 int main(int argc, char *argv[])
 {
     QApplication app(argc, argv);

     QTranslator qtTranslator;
     qtTranslator.load("qt_" + QLocale::system().name(),
             QLibraryInfo::location(QLibraryInfo::TranslationsPath));
     app.installTranslator(&qtTranslator);

     QTranslator myappTranslator;
     myappTranslator.load("myapp_" + QLocale::system().name());
     app.installTranslator(&myappTranslator);

     ...
     return app.exec();
 }
Note the use of QLibraryInfo::location() to locate the Qt translations. Developers should request the path to the translations at run-time by passing QLibraryInfo::TranslationsPath to this function instead of using the QTDIR environment variable in their applications.
*/

//ttt0 doc & screenshots for translation
//ttt0 something to add to SF in unstable, to get the date to change


//ttt0 german translates "Previous [Ctrl+P]" as "Vorherige [Strg+V]"

//ttt0 copy ID3V2 to ID3V1

//ttt0 update references based on traffic volume

//ttt0 delete LAME for CBR - https://sourceforge.net/apps/mantisbt/mp3diags/view.php?id=117

//ttt0 don't scan backup dir if it's inside the source
//ttt0 compute bitrate in VBR headers //ttt0 see why the bitrate computed manually based on VBR data doesn't match exactly the one computed for the audio (see mail sent on 2012.10.14)

