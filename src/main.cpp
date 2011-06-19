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

//#include  "Profiler.h"

using namespace std;

namespace po = boost::program_options;

static const char* APP_NAME ("Mp3Diags-unstable");
static const char* ORGANIZATION ("Ciobi");

GlobalSettings::GlobalSettings()
{
    m_pSettings = new QSettings (ORGANIZATION, APP_NAME);
}


GlobalSettings::~GlobalSettings()
{
    delete m_pSettings;
}


void GlobalSettings::saveSessions(const vector<string>& vstrSess1, const string& strLast, bool bOpenLast)
{
    vector<string> vstrSess (vstrSess1);

    { // add sessions that might have been added by another instance of the program
        vector<string> vstrSess2;
        string strLast2;
        bool bOpenLast2;
        loadSessions(vstrSess2, strLast2, bOpenLast2);
        for (int i = 0, n = cSize(vstrSess2); i < n; ++i)
        {
            if (vstrSess.end() == find(vstrSess.begin(), vstrSess.end(), vstrSess2[i]))
            {
                vstrSess.push_back(vstrSess2[i]); // ttt2 perhaps add a return value to the function, so the caller would know to update the UI; however, what is done here is more important, because it prevents data loss
            }
        }
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
}


void GlobalSettings::loadSessions(vector<string>& vstrSess, string& strLast, bool& bOpenLast) const
{
    vstrSess.clear();
    int n (m_pSettings->value("main/sessions/count", 0).toInt());
    strLast = convStr(m_pSettings->value("main/lastSession").toString());
    bOpenLast = m_pSettings->value("main/openLast", true).toBool();
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
        strLast.clear();
    }
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





// http://stackoverflow.com/questions/760323/why-does-my-qt4-5-app-open-a-console-window-under-windows - The option under Visual Studio for setting the subsystem is under Project Settings->Linker->System->SubSystem

int guiMain(int argc, char *argv[]) {
    Q_INIT_RESOURCE(Mp3Diags); // base name of the ".qrc" file
    QMp3DiagsApplication app(argc, argv);

    { // by default on Windows the selection is hard to see in the main window, because it's some gray;
        QPalette pal (QApplication::palette());
        pal.setColor(QPalette::Highlight, pal.color(QPalette::Active, QPalette::Highlight));
        pal.setColor(QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::HighlightedText));
        QApplication::setPalette(pal);
    }

    getDefaultFont(); // !!! to initialize the static var

    string strStartSession;
    int nSessCnt;
    bool bOpenLast;

    {
        vector<string> vstrSess;
        //string strLast;
        GlobalSettings st;
        st.loadSessions(vstrSess, strStartSession, bOpenLast);
        nSessCnt = cSize(vstrSess);
    }

    if (0 == nSessCnt)
    { // first run; create a new session and run it
        SessionEditorDlgImpl dlg (0, "", SessionEditorDlgImpl::FIRST_TIME);
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
            vstrSess.push_back(strStartSession);
            GlobalSettings st;
            st.saveSessions(vstrSess, strStartSession, dlg.shouldOpenLastSession());
        }
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

            {
                vector<string> vstrSess;
                bool bOpenLast;
                string s;
                GlobalSettings st;
                st.loadSessions(vstrSess, s, bOpenLast);
                st.saveSessions(vstrSess, strStartSession, bOpenLast);
                nSessCnt = cSize(vstrSess);
            }
            MainFormDlgImpl mainDlg (strStartSession, 1 == nSessCnt);
            mainDlg.setWindowIcon(QIcon(":/images/logo.svg"));

            if (1 == nSessCnt)
            {
                mainDlg.setWindowTitle("MP3 Diags" + QString(APP_BRANCH));
            }
            else
            {
                string::size_type n (strStartSession.rfind(getPathSep()));
                string s (strStartSession.substr(n + 1, strStartSession.size() - n - 3 - 2));
                mainDlg.setWindowTitle("MP3 Diags"  + QString(APP_BRANCH) + " - " + convStr(s));
            }

            if (MainFormDlgImpl::OPEN_SESS_DLG != mainDlg.run()) { return 0; }
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


class CmdLineAnalyzer
{
    Note::Severity m_minLevel;

    const QualThresholds& m_qualThresholds;
    int m_nCut; // for relative dirs

    // returns "true" if there are no problems
    bool processFile(const string& strFullName)
    {
        Mp3Handler* mp3;
        try
        {
            mp3 = new Mp3Handler(strFullName, false, m_qualThresholds);
        }
        catch (Mp3Handler::FileNotFound)
        {
            cout << "File not found: " + toNativeSeparators(strFullName) << endl << endl;
            return false;
        }

        bool thisFileHasProblems = false;
        const NoteColl& notes (mp3->getNotes());
        for (int i = 0, n = cSize(notes.getList()); i < n; ++i) // ttt2 poor performance
        {
            const Note* pNote (notes.getList()[i]);

            // category --include/--exclude options would be nice, too.
            bool showThisNote = (pNote->getSeverity() <= m_minLevel);
            if (!showThisNote)
            {
                continue;
            }

            if (!thisFileHasProblems)
            {
                thisFileHasProblems = true;
                cout << toNativeSeparators(strFullName.substr(m_nCut)) << endl; // ttt1 see if using short names is preferable
            }

            cout << "- " << (Note::severityToString(pNote->getSeverity())) << ": " << pNote->getDescription() << endl;
        }

        if (thisFileHasProblems)
        {
            cout << endl;
        }

        return !thisFileHasProblems;
    }

    // returns "true" if there are no problems
    bool processDir(const string& strFullName)
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
                bRes = processDir(fs.getName()) && bRes;
            }
            else
            {
                cout << "Skipping unknown name : \"" << toNativeSeparators(fs.getName()) << "\"" << endl << endl;
            }

            fs.findNext();
        }
        return bRes;
    }

public:

    CmdLineAnalyzer(Note::Severity minLevel, const QualThresholds& qualThresholds) : m_minLevel(minLevel), m_qualThresholds(qualThresholds) {}

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
            return processDir(strFullName);
        }
        else
        {
            cout << "Name not found: \"" << toNativeSeparators(strFullName) << "\"" << endl << endl;
        }

        return false;
    }

    // returns "true" if there are no problems
    bool processNames(const vector<string>& vstrNames)
    {
        bool anyFileHasProblems (false);
        for (int i = 0, n = cSize(vstrNames); i < n; ++i)
        {
            const string& file (vstrNames[i]);

            anyFileHasProblems = processName(file) || anyFileHasProblems; //ttt2 make wildcards recognized on Windows (perhaps Linux too but Bash takes care of this; not sure about other shells)
        }

        return !anyFileHasProblems;
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
    const vector<string> inputFiles = options["input-file"].as< vector<string> >();

    Note::Severity minLevel (Note::WARNING);
    try
    {
        minLevel = options["severity"].as<Note::Severity>(); //ttt1 see how to use default params in cmdlineDesc.add_options()
    }
    catch (...)
    { // nothing
    }

    // In cmdline mode, we want to make sure the user only sees our
    // carefully crafted messages, and no debug stuff from arbitrary
    // places in the program.
    qInstallMsgHandler(noMessageOutput);


    // ttt2 For now, we always use the default quality thresholds; however,
    // it certainly would make sense to load those from the last session,
    // or make it possible to specify them on the command line.
    CmdLineAnalyzer cmdLineAnalyzer (minLevel, QualThresholds::getDefaultQualThresholds());

    return cmdLineAnalyzer.processNames(inputFiles) ? 0 : 1;
}

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
//char *argv[] = {"aa", "-s", "support", "pppqqq"}; argc = 4;
//char *argv[] = {"aa", "pppqqq"}; argc = 2;
//char *argv[] = {"aa", "/d/test_mp3/1/tmp2/c pic/vbri assertion.mp3"}; argc = 2;
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
        ("help,h", "produce help message")
        ("uninstall,u", "uninstall (remove settings)")
    ;

    po::options_description cmdlineDesc ("Commandline mode");
    cmdlineDesc.add_options()
        //("input-file", po::value<vector<string> >(), "input file")
        //("severity,s", po::value<Note::Severity>()->default_value(Note::WARNING), "minimum severity to show (one of error, warning, support); default: warning") //ttt1 see if this can be made to work; it sort of does, but when invoked with "--help" it prints "arg (=1)" rather than "arg (=warning)"
        ("severity,s", po::value<Note::Severity>(), "minimum severity to show (one of error, warning, support); default: warning")
        //("severity,s", "minimum severity to show (one of error, warning, support")
    ;

    po::options_description hiddenDesc("Hidden options");
    hiddenDesc.add_options()
        ("input-file", po::value<vector<string> >(), "input file")
    ;


    po::positional_options_description positionalDesc;
    positionalDesc.add("input-file", -1);

    po::options_description fullDesc;
    fullDesc.add(genericDesc).add(cmdlineDesc).add(hiddenDesc);

    po::options_description visibleDesc;
    visibleDesc.add(genericDesc).add(cmdlineDesc);

    po::variables_map vm;
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

        //po::store(po::command_line_parser(argc, argv).options(fullDesc).positional(positionalDesc).run(), vm);
        po::store(po::command_line_parser(argc, argv).style(style).options(fullDesc).positional(positionalDesc).run(), vm);
        po::notify(vm);
    }
    catch (...)//const po::unknown_option&)
    {
        err = true;
    }

    if (err || vm.count("help") > 0) //ttt1 options "u" and "s" are incompatible; "s" without a file is wrong as well; these should trigger the "usage" message, then exit as well;
    {
        cout << "Usage: " << argv[0] << " [OPTION]... [FILE]...\n";
        cout << visibleDesc << endl;
        return 1;
    }

    if (vm.count("u") > 0)
    {
        QSettings s (ORGANIZATION, APP_NAME);
        s.clear(); //ttt2 see if this can actually remove everything, including the ORGANIZATION dir if it's empty;
        return 0;
    }

    if (vm.count("input-file") > 0)
    {
        Q_INIT_RESOURCE(Mp3Diags); // base name of the ".qrc" file
        QCoreApplication app(argc, argv); // !!! without this Qt file functions don't work correctly, e.g. QFileInfo has problems with file names that contain accents
        return cmdlineMain(vm);
    }
    else
    {
        return guiMain(argc, argv);
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
//ttt1 ID3V1 remover
//ttt0 make AdjustMt.sh work

