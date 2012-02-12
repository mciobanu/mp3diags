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

/***************************************************************************
 *                                                                         *
 * Command-line mode Copyright (C) 2011 by Michael Elsdörfer               *
 *                                                                         *
 ***************************************************************************/



#include  <algorithm>
#include  <cstdio>
#include  <iostream>
#include  <locale>

#include  <boost/program_options.hpp>

#include  <QApplication>
#include  <QSettings>
#include  <QFileInfo>
#include  <QMessageBox>
#include  <QFileInfo>
#include  <QDir>

#include  "MainFormDlgImpl.h"
#include  "SessionEditorDlgImpl.h"
#include  "SessionsDlgImpl.h"
#include  "Helpers.h"
#include  "StoredSettings.h"
#include  "OsFile.h"
#include  "Mp3Manip.h"
#include  "Notes.h"
#include  "Version.h"
#include  "Widgets.h"
#include  "CommonData.h"
#include  "ConfigDlgImpl.h"
#include  "Mp3TransformThread.h"
#include  "Translation.h"


//#include  "Profiler.h"

using namespace std;
using namespace Version;

namespace po = boost::program_options;


GlobalSettings::GlobalSettings()
{
    m_pSettings = new QSettings (getOrganization(), getSettingsAppName());
}


GlobalSettings::~GlobalSettings()
{
    delete m_pSettings;
}


void GlobalSettings::saveSessions(const vector<string>& vstrSess1, const string& strLast, bool bOpenLast, const string& strTempSessTempl, const string& strDirSessTempl, const string& strTranslation, bool bLoadExternalChanges)
{
    vector<string> vstrSess (vstrSess1);

    if (bLoadExternalChanges)
    { // add sessions that might have been added by another instance of the program
        vector<string> vstrSess2;
        string strLast2, strTempSessTempl2, strDirSessTempl2, strTranslation2;
        bool bOpenLast2;
        loadSessions(vstrSess2, strLast2, bOpenLast2, strTempSessTempl2, strDirSessTempl2, strTranslation2);
        for (int i = 0, n = cSize(vstrSess2); i < n; ++i)
        {
            if (vstrSess.end() == find(vstrSess.begin(), vstrSess.end(), vstrSess2[i]))
            {
                vstrSess.push_back(vstrSess2[i]); // ttt2 perhaps add a return value to the function, so the caller would know to update the UI; however, what is done here is more important, because it prevents data loss
            }
        }
    }

    if (!strLast.empty() && vstrSess.end() == find(vstrSess.begin(), vstrSess.end(), strLast)) // e.g. for a new session created using "folder-session" option
    {
        vstrSess.push_back(strLast);
    }

    int n (cSize(vstrSess));
    m_pSettings->setValue("main/lastSession", convStr(strLast));
    m_pSettings->setValue("main/openLast", bOpenLast);
    m_pSettings->remove("main/sessions");
    m_pSettings->setValue("main/sessions/count", n);
    char a [50];
    for (int i = 0; i < n; ++i)
    {
        sprintf(a, "main/sessions/val%04d", i);
        m_pSettings->setValue(a, convStr(vstrSess[i]));
    }
    m_pSettings->setValue("main/tempSessionTemplate", convStr(strTempSessTempl));
    m_pSettings->setValue("main/dirSessionTemplate", convStr(strDirSessTempl));
    m_pSettings->setValue("main/translation", convStr(strTranslation));
}


void GlobalSettings::loadSessions(vector<string>& vstrSess, string& strLast, bool& bOpenLast, string& strTempSessTempl, string& strDirSessTempl, string& strTranslation) const
{
    vstrSess.clear();
    int n (m_pSettings->value("main/sessions/count", 0).toInt());
    strLast = convStr(m_pSettings->value("main/lastSession").toString());
    bOpenLast = m_pSettings->value("main/openLast", true).toBool();
    strTempSessTempl = convStr(m_pSettings->value("main/tempSessionTemplate").toString());
    strDirSessTempl = convStr(m_pSettings->value("main/dirSessionTemplate").toString());
    char a [50];
    for (int i = 0; i < n; ++i)
    {
        sprintf(a, "main/sessions/val%04d", i);
        QString qs (m_pSettings->value(a, "").toString());
        if (QFileInfo(qs).isFile())
        {
            string s (convStr(qs));
            vstrSess.push_back(s);
        }
    }

    if (!QFileInfo(convStr(strLast)).isFile() || vstrSess.end() == find(vstrSess.begin(), vstrSess.end(), strLast))
    {
        // pick something else if the session got deleted; otherwise there's an assertion failure;
        strLast = vstrSess.empty() ? "" : vstrSess.back();
        // ttt1 generally improve handling of missing sessions (simply erasing them is probably not the best option);
    }

    strTranslation = convStr(m_pSettings->value("main/translation", "").toString());
}

const QFont& getDefaultFont()
{
    static QFont s_font;
    return s_font;
}


//static char* s_pFreeMem (new char[1000000]);
static char* s_pFreeMem (new char[10000]);

// to test this use "ulimit -v 200000" and uncomment the allocations for char[1000000] below
//
//ttt2 if the GUI got started, this doesn't seem to get called, regardless of the size of s_pFreeMem
void newHandler()
{
    delete[] s_pFreeMem;
    //puts("inside new handler");
    cerr << "inside new handler" << endl;
    cerr.flush();
    throw bad_alloc();
    return;
}


class QMp3DiagsApplication : public QApplication
{
public:
    /*override*/ bool notify(QObject* pReceiver, QEvent *pEvent)
    {
        try
        {
            return QApplication::notify(pReceiver, pEvent);
        }
        catch (...)
        {
            CB_ASSERT (false);
        }
    }

    QMp3DiagsApplication(int& argc, char** argv) : QApplication(argc, argv) {}
};

#ifdef MSVC_QMAKE
void visStudioMessageOutput(QtMsgType, const char* szMsg)
{
    OutputDebugStringA("    "); // to stand out from the other messages that get printed
    OutputDebugStringA(szMsg);
    OutputDebugStringA("\r\n");
    //cerr << szMsg << endl;
    //QMessageBox::information(0, "Debug message", szMsg, QMessageBox::Ok);
}
#endif


namespace {

class OptionInfo
{
    const string m_strLongOpt;
    const string m_strShortOpt;
    const string m_strFullOpt;
    const string m_strDescr;

public:
    OptionInfo(const string& strLongOpt, const string& strShortOpt, const string& strDescr) :
            m_strLongOpt(strLongOpt),
            m_strShortOpt(strShortOpt),
            m_strFullOpt(strLongOpt + (strShortOpt.empty() ? "" : "," + strShortOpt)),
            m_strDescr(strDescr),
            m_szLongOpt(m_strLongOpt.c_str()),
            m_szShortOpt(m_strShortOpt.c_str()),
            m_szFullOpt(m_strFullOpt.c_str()),
            m_szDescr(m_strDescr.c_str())
            {}

    const char* const m_szLongOpt;
    const char* const m_szShortOpt;
    const char* const m_szFullOpt;
    const char* const m_szDescr;
};


OptionInfo s_helpOpt ("help", "h", "Show this help message");
OptionInfo s_uninstOpt ("uninstall", "u", "Uninstall (remove settings)");
OptionInfo s_severityOpt ("severity", "s", "Minimum severity to show (one of error, warning, support); default: warning");
OptionInfo s_inputFileOpt ("input-file", "", "Input file");
OptionInfo s_hiddenFolderSessOpt ("hidden-session", "f", "Creates a new session for the specified folder and stores it inside that folder. The session will be hidden when the program exits.");
OptionInfo s_loadedFolderSessOpt ("visible-session", "v", "Creates a new session for the specified folder and stores it inside that folder. The session will be visible in the session list after the program is restarted.");
OptionInfo s_tempSessOpt ("temp-session", "t", "Creates a temporary session for the specified folder, which will be deleted when the program exits");
OptionInfo s_transfListOpt ("transf-list", "l", "A number between 1 and 4. Applies the specified custom transformation list to the files and/or folders passed"); //ttt1 use CUSTOM_TRANSF_CNT instead of 4
OptionInfo s_sessionOpt ("session", "w", "Session file name");

OptionInfo s_overrideSess ("", "", ""); // ttt1 maybe implement - for s_folderSessOpt and s_tempSessOpt (and s_inputFileOpt) - session with settings to use instead of the template


// replaces the folders of a session file with the given new folder
void setFolder(const string& strSessionFile, const string& strFolder)
{
    SessionSettings stg (strSessionFile);
    vector<string> vstrIncl, vstrExcl;
    vstrIncl.push_back(strFolder);
    stg.saveDirs(vstrIncl, vstrExcl);
}


static const char* const TEMP_SESS ("MP3DiagsTempSession");

struct SessEraser
{
    static void eraseTempSess()
    {
        string strErr (eraseFiles(getSepTerminatedDir(convStr(QDir::tempPath())) + TEMP_SESS));
        if (!strErr.empty())
        {
            cerr << "Cannot remove file " << strErr << endl;
            exit(1);
        }
    }

    void hideFolderSession()
    {
        if (m_strSessionToHide.empty()) { return; }

        GlobalSettings st;
        vector<string> vstrSess;
        string strLastSession;
        bool bOpenLast;
        string strTempSessTempl;
        string strDirSessTempl;
        string strTranslation;
        st.loadSessions(vstrSess, strLastSession, bOpenLast, strTempSessTempl, strDirSessTempl, strTranslation);

        vector<string>::iterator it (find(vstrSess.begin(), vstrSess.end(), m_strSessionToHide));
        if (vstrSess.end() != it)
        {
            vstrSess.erase(it);
            if (strLastSession == m_strSessionToHide)
            {
                strLastSession = vstrSess.empty() ? "" : vstrSess.back();
            }
        }

        st.saveSessions(vstrSess, strLastSession, bOpenLast, strTempSessTempl, strDirSessTempl, strTranslation, GlobalSettings::IGNORE_EXTERNAL_CHANGES);
    }

    string m_strSessionToHide;

    ~SessEraser()
    {
        eraseTempSess();
        hideFolderSession();
    }
};

} // namespace


//ttt1 in 10.3 loading a session seemed to erase the templates, but couldn't reproduce later

string getActiveSession(const po::variables_map& options, int& nSessCnt, bool& bOpenLast, string& strTempSessTempl, string& strDirSessTempl)
{
    string strRes;
    vector<string> vstrSess;
    //string strLast;
    GlobalSettings st;
    string strTranslation;

    st.loadSessions(vstrSess, strRes, bOpenLast, strTempSessTempl, strDirSessTempl, strTranslation);
    nSessCnt = cSize(vstrSess);

    if (options.count(s_sessionOpt.m_szLongOpt) > 0)
    {
        strRes = options[s_sessionOpt.m_szLongOpt].as<string>();
    }

    return strRes;
}


// http://stackoverflow.com/questions/760323/why-does-my-qt4-5-app-open-a-console-window-under-windows - The option under Visual Studio for setting the subsystem is under Project Settings->Linker->System->SubSystem

int guiMain(const po::variables_map& options) {

    { // by default on Windows the selection is hard to see in the main window, because it's some gray;
        QPalette pal (QApplication::palette());
        pal.setColor(QPalette::Highlight, pal.color(QPalette::Active, QPalette::Highlight));
        pal.setColor(QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::HighlightedText));
        QApplication::setPalette(pal);
    }

    getDefaultFont(); // !!! to initialize the static var

    string strStartSession;
    string strLastSession;

    int nSessCnt;
    bool bOpenLast;
    string strTempSessTempl;
    string strDirSessTempl;
    //bool bIsTempSess (false);
    string strTempSession (getSepTerminatedDir(convStr(QDir::tempPath())) + TEMP_SESS + SessionEditorDlgImpl::SESS_EXT);
    string strFolderSess;
    bool bHideFolderSess (true);

    if (options.count(s_hiddenFolderSessOpt.m_szLongOpt) > 0)
    {
        strFolderSess = options[s_hiddenFolderSessOpt.m_szLongOpt].as<string>();
    }
    else if (options.count(s_loadedFolderSessOpt.m_szLongOpt) > 0)
    {
        strFolderSess = options[s_loadedFolderSessOpt.m_szLongOpt].as<string>();
        bHideFolderSess = false;
    }

    SessEraser sessEraser;

    strLastSession = getActiveSession(options, nSessCnt, bOpenLast, strTempSessTempl, strDirSessTempl);

    if (options.count(s_tempSessOpt.m_szLongOpt) > 0)
    {
        SessEraser::eraseTempSess();

        strStartSession = strTempSession;

        if (strTempSessTempl.empty())
        {
            strTempSessTempl = strLastSession;
        }

        if (!strTempSessTempl.empty())
        {
            try
            {
                copyFile2(strTempSessTempl, strStartSession);
            }
            catch (...)
            { // nothing //ttt2 do more
            }
        }

        string strProcDir (options[s_tempSessOpt.m_szLongOpt].as<string>());
        strProcDir = getNonSepTerminatedDir(convStr(QDir(fromNativeSeparators(convStr(strProcDir))).absolutePath()));
        setFolder(strStartSession, strProcDir);
        bOpenLast = true;
    }
    else if (!strFolderSess.empty())
    {
        string strProcDir = getNonSepTerminatedDir(convStr(QDir(fromNativeSeparators(convStr(strFolderSess))).absolutePath())); //ttt2 test on root

        if (!dirExists(strProcDir))
        {
            showMessage(0, QMessageBox::Critical, 0, 0, "Error", "Folder \"" + convStr(strProcDir) + "\" doesn't exist. The program will exit ...", "O&K");
            return 1;
        }

        string strDirName (convStr(QFileInfo(convStr(strProcDir)).fileName()));
        if (strDirName.empty()) // it's a whole disk drive on Windows, e.g. D:\\  ;
        {
            strDirName += strProcDir[0];
            strDirName += "-MP3Diags";
        }
        strStartSession = getSepTerminatedDir(strProcDir) + strDirName + SessionEditorDlgImpl::SESS_EXT;
        if (bHideFolderSess)
        {
            sessEraser.m_strSessionToHide = strStartSession;
        }

        if (!fileExists(strStartSession))
        {
            if (strDirSessTempl.empty())
            {
                strDirSessTempl = strLastSession;
            }

            if (!strDirSessTempl.empty())
            {
                try
                {
                    copyFile2(strDirSessTempl, strStartSession);
                    setFolder(strStartSession, strProcDir);
                }
                catch (...)
                { // nothing //ttt2 do more
                }
            }
        }

        ofstream out (strStartSession.c_str(), ios_base::app);
        if (!out)
        {
            showMessage(0, QMessageBox::Critical, 0, 0, "Error", "Cannot write to file \"" + convStr(strStartSession) + "\". The program will exit ...", "O&K");
            return 1;
        }
        bOpenLast = true;
    }
    else if (0 == nSessCnt)
    { // first run; create a new session and run it
        SessionEditorDlgImpl dlg (0, "", SessionEditorDlgImpl::FIRST_TIME, ""); // ttt0 detect system locale
        dlg.setWindowIcon(QIcon(":/images/logo.svg"));
        strStartSession = dlg.run();
        if (strStartSession.empty())
        {
            return 0;
        }

        if ("*" == strStartSession)
        {
            strStartSession.clear();
        }
        else
        {
            vector<string> vstrSess;
            //vstrSess.push_back(strStartSession);
            GlobalSettings st;
            st.saveSessions(vstrSess, strStartSession, dlg.shouldOpenLastSession(), "", "", dlg.getTranslation(), GlobalSettings::LOAD_EXTERNAL_CHANGES);
        }
        bOpenLast = true;
    }
    else
    {
        strStartSession = strLastSession;
    }

    bool bOpenSelDlg (strStartSession.empty() || !bOpenLast);

    try
    {
        for (;;)
        {
            {
                QFont fnt;
                string strNewFont (convStr(fnt.family()));
                int nNewSize (fnt.pointSize());
                fixAppFont(fnt, strNewFont, nNewSize);
            }

            if (bOpenSelDlg)
            {
                SessionsDlgImpl dlg (0);
                dlg.setWindowIcon(QIcon(":/images/logo.svg"));

                strStartSession = dlg.run();
                if (strStartSession.empty())
                {
                    return 0;
                }
            }
            bOpenSelDlg = true;

            CB_ASSERT (!strStartSession.empty());

            bool bDefaultForVisibleSessBtn (true);
            //if (strStartSession != strTempSession)
            {
                vector<string> vstrSess;
                bool bOpenLast;
                string s, s1, s2, s3;
                GlobalSettings st;
                st.loadSessions(vstrSess, s, bOpenLast, s1, s2, s3);
                st.saveSessions(vstrSess, strStartSession, bOpenLast, s1, s2, s3, GlobalSettings::LOAD_EXTERNAL_CHANGES);
                bDefaultForVisibleSessBtn = (cSize(vstrSess) != 1 || !strFolderSess.empty() || vstrSess.end() != find(vstrSess.begin(), vstrSess.end(), strTempSession));
            }

            { //ttt2 overkill - create a "CommonData" just to read the language setting
                SessionSettings settings (strStartSession);
                CommonData commonData(settings, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
                settings.loadMiscConfigSettings(&commonData, SessionSettings::DONT_INIT_GUI);
                TranslatorHandler::getGlobalTranslator().setTranslation(commonData.m_strTranslation); // !!! must be done here, before the MainFormDlgImpl constructor
            }

            MainFormDlgImpl mainDlg (strStartSession, bDefaultForVisibleSessBtn);
            mainDlg.setWindowIcon(QIcon(":/images/logo.svg"));

            if (bDefaultForVisibleSessBtn)
            {
                mainDlg.setWindowTitle(QString(getAppName()) + " - " + convStr(SessionEditorDlgImpl::getTitleName(strStartSession)));
            }
            else
            {
                mainDlg.setWindowTitle(QString(getAppName()));
            }

            if (MainFormDlgImpl::OPEN_SESS_DLG != mainDlg.run())
            {
                return 0;
            }
        }
    }
    catch (...) // ttt2 for now it doesn't catch many exceptions; it seems that nothing can be done if an exception leaves a slot / event handler, but maybe there are ways around
    {
        /*QMessageBox dlg (QMessageBox::Critical, "Error", "Caught generic exception. Exiting ...", QMessageBox::Close, 0, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);

        dlg.exec();
        qDebug("out - err");*/
        CB_ASSERT (false);
    }

    /*mainDlg.show();
    return app.exec();*/
}




namespace {

class CmdLineProcessor
{
    QualThresholds m_qualThresholds;
    int m_nCut; // for relative dirs

    const vector<string>& m_vstrNames;

    virtual bool processFile(const string& strFullName, Mp3Handler* mp3Handler) = 0;

    // returns "true" if there are no problems
    bool processFile(const string& strFullName)
    {
        if (strFullName.size() <= 4)
        {
            return true;
        }

        QString qs;
        qs = convStr(strFullName.substr(strFullName.size() - 4)).toLower();
        if (qs != ".mp3") // ttt1 unify with test in Mp3ProcThread::scan()
        {
            return true;
        }

        Mp3Handler* mp3Handler;
        try
        {
            mp3Handler = new Mp3Handler(strFullName, false, m_qualThresholds);
        }
        catch (Mp3Handler::FileNotFound)
        {
            cout << "File not found: " + toNativeSeparators(strFullName) << endl << endl;
            return false;
        }

        return processFile(strFullName, mp3Handler);
    }

    // returns "true" if there are no problems
    bool processFullName(const string& strFullName) //ttt1 make FileSearcher take wildcards; then this function could take files, directories, and wildcard names (which might match both files and directories); in particular, a FileSearcher might take the name of a specific file, and then it should list it
    {
        bool bRes (true);
        FileSearcher fs (strFullName);

        while (fs)
        {
            if (fs.isFile())
            {
                bRes = processFile(fs.getName()) && bRes;
            }
            else if (fs.isDir())
            {
                bRes = processFullName(fs.getName()) && bRes;
            }
            else
            {
                cout << "Skipping unknown name : \"" << toNativeSeparators(fs.getName()) << "\"" << endl << endl;
            }

            fs.findNext();
        }
        return bRes;
    }

    // returns "true" if there are no problems
    bool processName(const string& strName)
    {
        string strFullName (convStr(QDir(fromNativeSeparators(convStr(strName))).absolutePath())); //ttt2 test on root
        m_nCut = endsWith(strFullName, strName) ? cSize(strFullName) - cSize(strName) : 0;

        //cout << strName << "    #    " << strFullName << "    #    " << getNonSepTerminatedDir(strFullName) << endl;
        if (fileExists(strFullName))
        {
            return processFile(strFullName);
        }
        else if (dirExists(strFullName))
        {
            return processFullName(strFullName);
        }
        else
        {
            cout << "Name not found: \"" << toNativeSeparators(strFullName) << "\"" << endl << endl;
        }

        return false;
    }

protected:
    void setQualThresholds(const QualThresholds& qualThresholds) { m_qualThresholds = qualThresholds; }
    string getRelativeName(const string& strFullName)
    {
        return strFullName.substr(m_nCut);
    }

    CmdLineProcessor(const vector<string>& vstrNames) : m_vstrNames(vstrNames) {}
    virtual ~CmdLineProcessor() {}

public:
    // returns "true" if there are no problems
    bool run()
    {
        bool bAnyFileHasProblems (false);
        for (int i = 0, n = cSize(m_vstrNames); i < n; ++i)
        {
            const string& file (m_vstrNames[i]);

            bAnyFileHasProblems = processName(file) || bAnyFileHasProblems; //ttt1 make wildcards recognized on Windows (perhaps Linux too but Bash takes care of this; not sure about other shells)
        }

        return !bAnyFileHasProblems;
    }
};



class CmdLineAnalyzer : public CmdLineProcessor
{
    Note::Severity m_minLevel;

    /*override*/ bool processFile(const string& strFullName, Mp3Handler* pmp3Handler)
    {
        bool bThisFileHasProblems = false;
        const NoteColl& notes (pmp3Handler->getNotes());
        for (int i = 0, n = cSize(notes.getList()); i < n; ++i) // ttt2 poor performance
        {
            const Note* pNote (notes.getList()[i]);

            // category --include/--exclude options would be nice, too.
            bool bShowThisNote = (pNote->getSeverity() <= m_minLevel);
            if (!bShowThisNote)
            {
                continue;
            }

            if (!bThisFileHasProblems)
            {
                bThisFileHasProblems = true;
                cout << toNativeSeparators(getRelativeName(strFullName)) << endl;
            }

            cout << "- " << (Note::severityToString(pNote->getSeverity())) << ": " << pNote->getDescription() << endl;
        }

        if (bThisFileHasProblems)
        {
            cout << endl;
        }

        return !bThisFileHasProblems;
    }

public:
    CmdLineAnalyzer(Note::Severity minLevel, const QualThresholds& qualThresholds, const vector<string>& vstrNames) : CmdLineProcessor(vstrNames), m_minLevel(minLevel)
    {
        setQualThresholds(qualThresholds);
    }

};




class TransfListRunner : public CmdLineProcessor
{
    struct Mp3TransformerCli : public Mp3Transformer
    {
        Mp3TransformerCli(
            CommonData* pCommonData,
            const TransfConfig& transfConfig,
            const deque<const Mp3Handler*>& vpHndlr,
            vector<const Mp3Handler*>& vpDel,
            vector<const Mp3Handler*>& vpAdd,
            vector<Transformation*>& vpTransf) :

            Mp3Transformer(
                pCommonData,
                transfConfig,
                vpHndlr,
                vpDel,
                vpAdd,
                vpTransf,
                &cout)
        {
        }

        /*override*/ bool isAborted() { return false; }
        /*override*/ void checkPause() {}
        /*override*/ void emitStepChanged(const StrList&, int) {}
    };

    SessionSettings m_settings;
    CommonData m_commonData;
    TransfConfig m_transfConfig;

    deque<const Mp3Handler*> m_vpHndlr;
    vector<const Mp3Handler*> m_vpDel;
    vector<const Mp3Handler*> m_vpAdd;
    vector<Transformation*> m_vpTransf;
    Mp3TransformerCli m_mp3TransformerCli;

    void loadCustomTransf(int nTransfList) // ttt1 unify with MainFormDlgImpl::loadCustomTransf() - perhaps make it member of SessionSettings
    {
        char bfr [50];
        sprintf(bfr, "customTransf/set%04d", nTransfList);
        //vector<string> vstrNames (m_settings.loadCustomTransf(k));
        bool bErr;
        vector<string> vstrNames (m_settings.loadVector(bfr, bErr));
        vector<int> v;
        const vector<Transformation*>& u (m_commonData.getAllTransf());
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
                //QMessageBox::warning(this, "Error setting up custom transformations", "Couldn't find a transformation with the name \"" + convStr(strName) + "\". The program will proceed, but you should review the custom transformations lists.");
                cerr << "Error setting up custom transformations: " << "Couldn't find a transformation with the name \"" + strName + "\". The program will proceed, but you should review the custom transformations lists." << endl;
            }
        }

        if (v.empty())
        {
            vector<vector<int> > vv (CUSTOM_TRANSF_CNT);
            initDefaultCustomTransf(nTransfList, vv, &m_commonData);
            v = vv[nTransfList];
        }

        m_commonData.setCustomTransf(nTransfList, v);
    }

    /*override*/ bool processFile(const string& strFullName, Mp3Handler* pmp3Handler)
    {
        m_vpHndlr.clear();
        m_vpHndlr.push_back(pmp3Handler);
        m_vpDel.clear();
        m_vpAdd.clear();
        bool bRes (m_mp3TransformerCli.transform());
        if (!bRes)
        {
            string strErr (m_mp3TransformerCli.getError());
            if (strErr.empty())
            {
                strErr = "Unknown error while processing " + strFullName;
            }
            cerr << strErr << endl;
        }
        return bRes;
    }

public:
    TransfListRunner(const vector<string>& vstrNames, int nTransfList, const string& strSessFile) :
        CmdLineProcessor(vstrNames),
        m_settings(strSessFile),
        m_commonData(m_settings, 0, 0, 0, 0, 0, 0, 0, 0, 0, false),
        m_mp3TransformerCli(&m_commonData, m_transfConfig, m_vpHndlr, m_vpDel, m_vpAdd, m_vpTransf)
    {
        m_settings.loadMiscConfigSettings(&m_commonData, SessionSettings::DONT_INIT_GUI);
        m_settings.loadTransfConfig(m_transfConfig);
        for (int i = 0; i < CUSTOM_TRANSF_CNT; ++i)
        {
            loadCustomTransf(i);
        }

        m_vpTransf.clear();
        for (int i = 0, n = cSize(m_commonData.getCustomTransf()[nTransfList]); i < n; ++i)
        {
            m_vpTransf.push_back(m_commonData.getAllTransf()[m_commonData.getCustomTransf()[nTransfList][i]]);
        }

        setQualThresholds(m_commonData.getQualThresholds());
        m_commonData.m_strTransfLog = SessionEditorDlgImpl::getLogFileName(strSessFile);
    }
};



void noMessageOutput(QtMsgType, const char*) { }


int cmdlineMain(const po::variables_map& options)
{
    /*
    Notes about Windows, where it doesn't seem possible to output something to the console from a GUI app.
    - the chosen option is to use redirection, either to a file or through "more" or something similar; this is made more transparent by using MP3DiagsCLI.cmd to redirect to a temp file and then print it
    - other option would be to open Notepad and populate it using COM/OLE. Not sure how to do it.
    - other option would be to redirect output to a file and open that file in Notepad.
    - other option would be to open a Qt window that only has the output from the run and a close button.
    - other option would be to open a new console and output the results there

    see also:
        http://blogs.msdn.com/b/junfeng/archive/2004/02/06/68531.aspx
        http://www.codeproject.com/KB/cpp/EditBin.aspx
        http://www.halcyon.com/~ast/dload/guicon.htm
    */
    const vector<string> inputFiles = options[s_inputFileOpt.m_szLongOpt].as< vector<string> >();

    Note::Severity minLevel (Note::WARNING);
    if (options.count(s_severityOpt.m_szLongOpt) > 0)
    {
        try
        {
            minLevel = options[s_severityOpt.m_szLongOpt].as<Note::Severity>(); //ttt2 see how to use default params in cmdlineDesc.add_options()
        }
        catch (...)
        { // nothing
        }
    }

    // In cmdline mode, we want to make sure the user only sees our
    // carefully crafted messages, and no debug stuff from arbitrary
    // places in the program.
    qInstallMsgHandler(noMessageOutput);

    string strSessFile;

    {
        int nSessCnt;
        bool bOpenLast;
        string strTempSessTempl;
        string strDirSessTempl;
        strSessFile = getActiveSession(options, nSessCnt, bOpenLast, strTempSessTempl, strDirSessTempl);
    }

    if (options.count(s_transfListOpt.m_szLongOpt) > 0)
    {

        int nTransfList (options[s_transfListOpt.m_szLongOpt].as<int>() - 1); // [1..4] -> [0..3]
        if (nTransfList < 0 || nTransfList >= CUSTOM_TRANSF_CNT)
        {
            cerr << "Transformation list must be a number between 1 and " << CUSTOM_TRANSF_CNT << endl;
            return 1;
        }

        TransfListRunner transfListRunner (inputFiles, nTransfList, strSessFile);

        return transfListRunner.run() ? 0 : 1;
    }

    SessionSettings settings (strSessFile);
    CommonData commonData (settings, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);

    settings.loadMiscConfigSettings(&commonData, SessionSettings::DONT_INIT_GUI);
    //settings.loadTransfConfig(transfConfig);
    //commonData.m_strTransfLog = SessionEditorDlgImpl::getLogFileName(strSessFile);

    CmdLineAnalyzer cmdLineAnalyzer (minLevel, commonData.getQualThresholds(), inputFiles);

    return cmdLineAnalyzer.run() ? 0 : 1;
}

/*

ttt1 CLI-friendly function restructuring:
- errors and warnings should be returned in strings / lists and outputting from CLI tools done from a central place
*/


//static const char* CMD_HELP = "help"; static const char* CMD_HELP_SHORT = "h"; static const char* CMD_HELP_FULL = "help,h";

//#define GEN_CMD(NAME, LNG, SHRT) static const char* CMD_##NAME = #LNG; static const char* CMD_##NAME##_SHORT = #SHRT; static const char* CMD_##NAME##_FULL = "LNG##SHRT";

//GEN_CMD(HELP, help, h);


} // namespace


// To parse a Note::Severity value from the command line using boost::program_options.
static void validate(boost::any& v, vector<string> const& values, Note::Severity*, int) {
    po::validators::check_first_occurrence(v);
    const string& s = po::validators::get_single_string(values);
    if (s.compare("error") == 0) v = Note::ERR;
    else if (s.compare("warning") == 0) v = Note::WARNING;
    else if (s.compare("support") == 0) v = Note::SUPPORT;
    //else throw po::validation_error(po::validation_error::invalid_option_value);
    //else throw po::validation_error("invalid option value");
    else throw runtime_error("invalid option value");
}


int main(int argc, char *argv[])
{
    //ifstream_unicode in ("/home/ciobi/cpp/Mp3Utils/mp3diags/trunk/mp3diags/src/2/testä.txt");
    /*ifstream_unicode in ("/home/ciobi/cpp/Mp3Utils/mp3diags/trunk/mp3diags/src/2/test_.txt");
    string s;
    getline(in, s);
    cout << s;
    return 4;//*/

//char *argv[] = {"aa", "-s", "support", "pppqqq"}; argc = 4;
//char *argv[] = {"aa", "pppqqq"}; argc = 2;
//char *argv[] = {"aa", "/d/test_mp3/1/tmp2/c pic/vbri assertion.mp3"}; argc = 2;
//char *argv1[] = {"aa", "--directory", "/test_mp3/1/tmp2/c pic" }; argc = 3; argv = argv1;
//char *argv1[] = {"aa", "--temp-session", "/d/test_mp3/1/tmp2/c pic" }; argc = 3; argv = argv1;
//char *argv1[] = {"aa", "--input-file", "/test_mp3/1/tmp2/c pic" }; argc = 3; argv = argv1;
//char *argv1[] = {"aa", "--hidden-folder-session", "/d/test_mp3/1/tmp2/c pic" }; argc = 3; argv = argv1;
//char *argv1[] = {"aa", "--hidden-folder-session", "/d/test_mp3/1/tmp2/text frames" }; argc = 3; argv = argv1;
//char *argv1[] = {"aa", "--loaded-folder-session", "/d/test_mp3/1/tmp2/c pic" }; argc = 3; argv = argv1;
//char *argv1[] = {"aa", "--loaded-folder-session", "/d/test_mp3/1/tmp2/text frames" }; argc = 3; argv = argv1;
//char *argv1[] = {"aa", "--folder-session", "/usr" }; argc = 3; argv = argv1;
    //char *argv1[] = {"aa", "--hidden-session", "Q:\\" }; argc = 3; argv = argv1;

    //DEFINE_PROF_ROOT("mp3diags");
    //PROF("root");
/*
    //locale::global(locale(""));
    ostringstream o;
    o << 12345.78;
    cout << o.str() << endl;
    printf("%f\n", 12345.78);//*/

    void (*nh)() = set_new_handler(newHandler);
    if (0 != nh) { cerr << "previous new handler: " << (void*)nh << endl; }
    //for (int i = 0; i < 200; ++i) { new char[1000000]; }

#ifdef MSVC_QMAKE
    qInstallMsgHandler(visStudioMessageOutput);
    // see http://lists.trolltech.com/qt-interest/2006-10/msg00829.html
    //OutputDebugStringA("\n\ntest output\n\n\n"); // !!! this only works if actually debugging (started with F5);
#endif

    po::options_description genericDesc ("General options");
    genericDesc.add_options()
        (s_helpOpt.m_szFullOpt, s_helpOpt.m_szDescr)
        (s_uninstOpt.m_szFullOpt, s_uninstOpt.m_szDescr)
        (s_sessionOpt.m_szFullOpt, po::value<string>(), s_sessionOpt.m_szDescr)
    ;

    po::options_description cmdlineDesc ("Commandline mode");
    cmdlineDesc.add_options()
        //("input-file", po::value<vector<string> >(), "input file")
        //("severity,s", po::value<Note::Severity>()->default_value(Note::WARNING), "minimum severity to show (one of error, warning, support); default: warning") //ttt1 see if this can be made to work; it sort of does, but when invoked with "--help" it prints "arg (=1)" rather than "arg (=warning)"
        (s_severityOpt.m_szFullOpt, po::value<Note::Severity>(), s_severityOpt.m_szDescr)
        //("severity,s", "minimum severity to show (one of error, warning, support")
        (s_transfListOpt.m_szFullOpt, po::value<int>(), s_transfListOpt.m_szDescr)
    ;

    po::options_description folderSessDesc ("New, per-folder, session mode");
    folderSessDesc.add_options()
        (s_hiddenFolderSessOpt.m_szFullOpt, po::value<string>(), s_hiddenFolderSessOpt.m_szDescr)
        (s_loadedFolderSessOpt.m_szFullOpt, po::value<string>(), s_loadedFolderSessOpt.m_szDescr)
        (s_tempSessOpt.m_szFullOpt, po::value<string>(), s_tempSessOpt.m_szDescr)
    ;

    po::options_description hiddenDesc("Hidden options");
    hiddenDesc.add_options()
        (s_inputFileOpt.m_szFullOpt, po::value<vector<string> >(), s_inputFileOpt.m_szDescr)
    ;


    po::positional_options_description positionalDesc;
    positionalDesc.add(s_inputFileOpt.m_szLongOpt, -1);

    po::options_description fullDesc;
    fullDesc.add(genericDesc).add(cmdlineDesc).add(hiddenDesc).add(folderSessDesc);

    po::options_description visibleDesc;
    visibleDesc.add(genericDesc).add(cmdlineDesc).add(folderSessDesc);

    po::variables_map options;
    bool err (false);
    try
    {

        po::command_line_style::style_t style (po::command_line_style::style_t(
            po::command_line_style::unix_style
            // | po::command_line_style::case_insensitive
            // | po::command_line_style::allow_long_disguise
#ifdef WIN32
            | po::command_line_style::allow_slash_for_short
#endif
        ));

        //po::store(po::command_line_parser(argc, argv).options(fullDesc).positional(positionalDesc).run(), options);
        po::store(po::command_line_parser(argc, argv).style(style).options(fullDesc).positional(positionalDesc).run(), options);
        po::notify(options);
    }
    catch (const exception& ex)
    {
        cerr << ex.what() << endl;
        err = true;
    }
    catch (...)//const po::unknown_option&)
    {
        cerr << "unknown exception" << endl;
        err = true;
    }

    if (err || options.count(s_helpOpt.m_szLongOpt) > 0) //ttt1 options "u" and "s" are incompatible; "s" without a file is wrong as well; these should trigger the "usage" message, then exit as well;
    {
        cout << "Usage: " << argv[0] << " [OPTION]... [FILE]...\n";
        cout << visibleDesc << endl;
        return 1;
    }

    if (options.count(s_uninstOpt.m_szLongOpt) > 0)
    {
        QSettings s (getOrganization(), getSettingsAppName());
        s.clear(); //ttt2 see if this can actually remove everything, including the getOrganization() dir if it's empty;
        ShellIntegration::enableHiddenSession(false);
        ShellIntegration::enableVisibleSession(false);
        ShellIntegration::enableTempSession(false);
        return 0;
    }

    if (options.count(s_inputFileOpt.m_szLongOpt) > 0)
    {
        Q_INIT_RESOURCE(Mp3Diags); // base name of the ".qrc" file
        QCoreApplication app (argc, argv); // !!! without this Qt file functions don't work correctly, e.g. QFileInfo has problems with file names that contain accents
        //TranslatorHandler::getGlobalTranslator();
        return cmdlineMain(options);
    }
    else
    {
        //QTranslator translator;
        //translator.load("mp3diags_cs");

        Q_INIT_RESOURCE(Mp3Diags); // base name of the ".qrc" file
        QMp3DiagsApplication app (argc, argv);
        TranslatorHandler::getGlobalTranslator();

        /*qDebug("lang %s", TranslatorHandler::getGlobalTranslator().getLanguageInfo("ww/mp3diags_cs.qm").c_str());
        qDebug("lang %s", TranslatorHandler::getGlobalTranslator().getLanguageInfo("ww/mp3diags_en_US.qm").c_str());
        qDebug("lang %s", TranslatorHandler::getGlobalTranslator().getLanguageInfo("ww/mp3diags_ro.qm").c_str());
        qDebug("lang %s", TranslatorHandler::getGlobalTranslator().getLanguageInfo("mp3diags_ro.qm").c_str());
        qDebug("lang %s", TranslatorHandler::getGlobalTranslator().getLanguageInfo("mp3diags_ro_RO.qm").c_str());
        qDebug("lang %s", TranslatorHandler::getGlobalTranslator().getLanguageInfo("_mp3diags_ro_RO.qm").c_str());
        qDebug("lang %s", TranslatorHandler::getGlobalTranslator().getLanguageInfo("/qweqw/_mp3diags_ro_RO.qm").c_str());*/

        //app.installTranslator(&translator);

        return guiMain(options);
    }
}




//"undefined reference to `qInitResources_application()'"




//ttt2 perhaps sign package

/*
rpmlint:


MP3Diags.x86_64: W: no-changelogname-tag
MP3Diags.src: W: no-changelogname-tag
There is no %changelog tag in your spec file. To insert it, just insert a
'%changelog' in your spec file and rebuild it.

MP3Diags.src: W: source-or-patch-not-bzipped MP3Diags-0.99.0.1.tar.gz
A source archive or patch in your package is not bzipped (doesn't have the
.bz2 extension). Files bigger than 100k should be bzip2'ed in order to save
space. To bzip2 a patch, use bzip2. To bzip2 a source tarball, use bznew

MP3Diags.x86_64: E: summary-ended-with-dot (Badness: 89) Tool for finding and fixing problems in MP3 files. Includes a tagger.
MP3Diags.src: E: summary-ended-with-dot (Badness: 89) Tool for finding and fixing problems in MP3 files. Includes a tagger.
Summary ends with a dot.

----------------

???

rpmlint -v ../RPMS/x86_64/MP3Diags-0.99.0.1-1.x86_64.rpm
W: MP3Diags untranslated-desktop-file /usr/share/applications/MP3Diags.desktop
W: MP3Diags summary-ended-with-dot Tool for finding and fixing problems in MP3 files. Includes a tagger.
I: MP3Diags checking
E: MP3Diags tag-not-utf8 Summary
E: MP3Diags tag-not-utf8 %description
E: MP3Diags no-packager-tag
E: MP3Diags no-changelogname-tag
E: MP3Diags invalid-desktopfile /usr/share/applications/MP3Diags.desktop value "Audio;AudioVideo;AudioVideoEditing" for string list key "Categories" in group "Desktop Entry" does not have a semicolon (';') as trailing character
E: MP3Diags filename-not-utf8 /usr/share/icons/hicolor/48x48/apps/MP3Diags.png
E: MP3Diags filename-not-utf8 /usr/share/icons/hicolor/40x40/apps/MP3Diags.png
E: MP3Diags filename-not-utf8 /usr/share/icons/hicolor/36x36/apps/MP3Diags.png
E: MP3Diags filename-not-utf8 /usr/share/icons/hicolor/32x32/apps/MP3Diags.png
E: MP3Diags filename-not-utf8 /usr/share/icons/hicolor/24x24/apps/MP3Diags.png
E: MP3Diags filename-not-utf8 /usr/share/icons/hicolor/22x22/apps/MP3Diags.png
E: MP3Diags filename-not-utf8 /usr/share/icons/hicolor/16x16/apps/MP3Diags.png
E: MP3Diags filename-not-utf8 /usr/share/applications/MP3Diags.desktop
E: MP3Diags filename-not-utf8 /usr/bin/MP3Diags


local build on 10.3:

WARNING: Category "Audio" is unknown !
WARNING: it is ignored, until you registered a Category at adrian@suse.de .

*/

//ttt1 CLI-support: scan some files, create logs, apply some transforms, ...

//ttt1 explorer right-click; create a new session vs. add to existing one
//ttt0 perhaps look at building Boost serialization, so it can be statically linked

//ttt0 config screenshots need "shell" tab
//ttt0 "shell" tab has smaller font in screenshots

//ttt0 look at running multiple instances concurrently / exit when second starts
//use a .pid file in the session dir with its name based on the session name, so multiple instances programs can start for different sessions; if pid matches and it's mp3diags exit and bring the other program up
//ttt0 maybe: thread to write to tmp that crt proc is alive and what session it's using; then start new process / bring existing one on top, as needed

//ttt1 Windows changelog should use \n\r (probably by replacing "copy /y changelog.txt bin\\changelog.txt" in BuildMp3Diags.hta with something that reads the file line by line

//ttt0 does TIFF cover art work? On Windows?

//ttt2 non-utf8 file in /d/test_mp3/1/tmp2/crt_test/martin/dj not showing (reason: ListEnumerator::ListEnumerator calls QDir::entryInfoList(), which simply doesn't include the file, probably because its name is not UTF-8)

//ttt0 config restructuring: session data: general settings, gui settings, gui controls, files

/*

ttt0 move to some readme.txt

using translations:

MP3 Diags automatically loads translations. Translations are stored in .qm files. Several steps are needed to create them:
    1) make MP3 Diags aware of the new translation
    2) run lupdate to create a .ts file
    3) update the .ts file with the actual translation
    4) run lcreate to create a .qm file

1) add an entry in the TRANSLATIONS section of src/src.pro

2) to create the .ts files run:
lupdate src/src.pro

3) open Qt Linguist and load the .ts file in src/translations
translate, save

4) to create the .qm files run:
lrelease src/src.pro

After it is done, the .ts file should be added to the project's source control.

The regular build (in Install.sh or BuildBz2.sh) also creates the .qm files.

Testing: the program looks for .qm files in 2 places:
    - the executable's folder
    - folder "../share/mp3diags/translations" (or "../share/mp3diags-unstable/translations") relative to the executable's folder

*/

//ttt0 translation build on Wnd
