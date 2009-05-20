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


#include  <algorithm>

#include  <QApplication>
#include  <QSettings>
#include  <QFileInfo>

#include  "MainFormDlgImpl.h"
#include  "SessionEditorDlgImpl.h"
#include  "SessionsDlgImpl.h"
#include  "Helpers.h"
#include  "StoredSettings.h"

using namespace std;

static const char* APP_NAME ("Mp3Diags");
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
                vstrSess.push_back(vstrSess2[i]); // ttt1 perhaps add a return value to the function, so the caller would know to update the UI; however, what is done here is more important, because it prevents data loss
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


int main(int argc, char *argv[])
{
/*      QCoreApplication app(argc, argv);
      qDebug("Hello from Qt 4!");
      return 0;*/

    Q_INIT_RESOURCE(Mp3Diags); // base name of the ".qrc" file
    QApplication app(argc, argv);

    if (argc > 1)
    {
        if (2 == argc && (0 == strcmp("/u", argv[1]) || 0 == strcmp("-u", argv[1])))
        {
            QSettings s (ORGANIZATION, APP_NAME);
            s.clear(); //ttt2 see if this can actually remove everything, including the ORGANIZATION dir if it's empty;
            return 0;
        }

        printf("\nUsage:\n%s [-u]\n\n", argv[0]);
        return 1;
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

    for (;;)
    {
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

        MainFormDlgImpl mainDlg (0, strStartSession);

        mainDlg.setWindowIcon(QIcon(":/images/logo.svg"));
        mainDlg.setWindowTitle("MP3 Diags");
        {
            vector<string> vstrSess;
            bool bOpenLast;
            string s;
            GlobalSettings st;
            st.loadSessions(vstrSess, s, bOpenLast);
            st.saveSessions(vstrSess, strStartSession, bOpenLast);
            nSessCnt = cSize(vstrSess);
        }
        if (MainFormDlgImpl::OPEN_SESS_DLG != mainDlg.run()) { return 0; }
    }
    /*mainDlg.show();
    return app.exec();*/
}


//"undefined reference to `qInitResources_application()'"


//ttt0 sign package

/*
ttt0 rpmlint:


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




*/

