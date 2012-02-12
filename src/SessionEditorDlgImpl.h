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


#ifndef SessionEditorDlgImplH
#define SessionEditorDlgImplH

#include  <string>

#include  <QDialog>

#include  "ui_SessionEditor.h"


class CheckedDirModel;
class QSettings;

class SessionEditorDlgImpl : public QDialog, private Ui::SessionEditorDlg
{
    Q_OBJECT

    CheckedDirModel* m_pDirModel;

    const std::string m_strDir;

    std::string m_strSessFile;
    bool m_bNew;
    std::string m_strTranslation;

    bool m_bOpenLastSession; // meaningful only if bFirstTime was true on the constructor;
    void commonConstr(); // common code for both constructors
    void setWindowTitle();


public:
    // if bFirstTime is false it doesn't show the "Open last session" checkbox;
    // strDir is used as a start directory by on_m_pFileNameB_clicked;
    enum { NOT_FIRST_TIME, FIRST_TIME };
    SessionEditorDlgImpl(QWidget* pParent, const std::string& strDir, bool bFirstTime, const std::string& strTranslation); // used for creating a new session;
    SessionEditorDlgImpl(QWidget* pParent, const std::string& strSessFile); // used for editing an existing session;
    ~SessionEditorDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    //std::vector<std::string> m_vstrCheckedDirs;
    //std::vector<std::string> m_vstrUncheckedDirs; // !!! not needed; retrieved from the config file
    bool shouldOpenLastSession() const { return m_bOpenLastSession; } // meaningful only if bFirstTime was true on the constructor;

    // returns the name of an INI file for OK and an empty string for Cancel; returns "*" to just go to the sessions dialog;
    std::string run();
    const std::string& getTranslation() const { return m_strTranslation; }

    static std::string getDataFileName(const std::string& strSessFile);
    static std::string getLogFileName(const std::string& strSessFile);
    static std::string getBaseName(const std::string& strSessFile); // removes extension
    static std::string getTitleName(const std::string& strSessFile); // removes path and extension
    static void removeSession(const std::string& strSessFile); // removes all files associated with a session: .ini, .mp3ds, .log, .dat, _trace.txt, _step1.txt, _step2.txt
    static const char* const SESS_EXT;
    static int SESS_EXT_LEN;

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pOkB_clicked();
    void on_m_pCancelB_clicked();
    void on_m_pBackupB_clicked();
    void on_m_pFileNameB_clicked();
    //void on_m_pLoadB_clicked();
    void on_m_pOpenSessionsB_clicked();
    void on_m_pTranslationCbB_currentIndexChanged(int);

    void onShow();

    void onHelp();
};



#endif

