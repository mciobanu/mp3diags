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


#include  <QMessageBox>
#include  <QFileDialog>
#include  <QTimer>
#include  <QHeaderView>
#include  <QSettings>

#include  "SessionEditorDlgImpl.h"

#include  "CheckedDir.h"
#include  "Helpers.h"
#include  "Transformation.h"
#include  "StoredSettings.h"
#include  "OsFile.h"
#include  "Translation.h"
#include  "CommonData.h"
#include  "Widgets.h"

using namespace std;
//using namespace pearl;


void SessionEditorDlgImpl::commonConstr() // common code for both constructors
{
    setupUi(this);

    m_pDirModel = new CheckedDirModel(this, CheckedDirModel::USER_CHECKABLE);

    m_pDirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::Drives);
    m_pDirModel->setSorting(QDir::IgnoreCase);
    m_pDirectoriesT->setModel(m_pDirModel);
    m_pDirectoriesT->expand(m_pDirModel->index("/"));
    m_pDirectoriesT->header()->hide();
    m_pDirectoriesT->header()->setStretchLastSection(false); m_pDirectoriesT->header()->setResizeMode(0, QHeaderView::ResizeToContents);

    m_bOpenLastSession = true;

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }

    QPalette grayPalette (m_pBackupE->palette());
    grayPalette.setColor(QPalette::Base, grayPalette.color(QPalette::Disabled, QPalette::Window));
    m_pBackupE->setPalette(grayPalette);
    m_pFileNameE->setPalette(grayPalette);

    { // language
        int nCrt (0);
        const vector<string>& vstrTranslations (TranslatorHandler::getGlobalTranslator().getTranslations());
        string strTmpTranslation (m_strTranslation); // !!! needed because on_m_pTranslationCbB_currentIndexChanged() will get triggered and change m_strTranslation
        for (int i = 0; i < cSize(vstrTranslations); ++i)
        {
            m_pTranslationCbB->addItem(convStr(TranslatorHandler::getLanguageInfo(vstrTranslations[i])));
            if (strTmpTranslation == vstrTranslations[i])
            {
                nCrt = i;
            }
        }
        m_strTranslation = strTmpTranslation;
        m_pTranslationCbB->setCurrentIndex(nCrt);
    }
}


// used for creating a new session;
SessionEditorDlgImpl::SessionEditorDlgImpl(QWidget* pParent, const string& strDir, bool bFirstTime, const string& strTranslation) : QDialog(pParent, getDialogWndFlags()), Ui::SessionEditorDlg(), m_strDir(strDir), m_bNew(true), m_strTranslation(strTranslation)
{
    commonConstr();

    bool bAutoFileName (false);
    {
#ifndef WIN32
        QString qs (QDir::homePath() + "/Documents"); // OK on openSUSE, not sure how standardized it is //ttt0 this is localized, so not OK; look at Qt
#else
        QSettings settings (QSettings::UserScope, "Microsoft", "Windows");
        settings.beginGroup("CurrentVersion/Explorer/Shell Folders");
        QString qs (fromNativeSeparators(settings.value("Personal").toString()));
#endif
        if (QFileInfo(qs).isDir())
        {
            qs += "/MP3Diags";
            qs += SESS_EXT;
            if (!QDir().exists(qs))
            {
                m_pFileNameE->setText(toNativeSeparators(qs));
                bAutoFileName = true;
            }
        }
    }

    //setWindowTitle("MP3 Diags - Create a new session or load an existing one");
    setWindowTitle();
    m_pDontCreateBackupRB->setChecked(true);
    m_pScanAtStartupCkB->setChecked(true);

    m_pFileNameE->setToolTip(bAutoFileName ?
        tr("This is the name of the \"settings file\"\n\n"
        "It is supposed to be a file that doesn't already exist. You don't need to set it up. MP3 Diags\n"
        "will store its settings in this file.\n\n"
        "The name was generated automatically. If you want to choose a different name, simply click on\n"
        "the button at the right to change it.", "this is a multiline tooltip") :

        tr("Here you need to specify the name of a \"settings file\"\n\n"
        "This is supposed to be a file that doesn't already exist. You don't need to set it up. MP3 Diags\n"
        "will store its settings in this file.\n\n"
        "To change it, simply click on the button at the right to choose the name of the settings file.", "this is a multiline tooltip"));

    if (!bFirstTime)
    {
        m_pOpenLastCkB->hide();
        m_pOpenSessionsB->hide();
    }
}


// used for editing an existing session;
SessionEditorDlgImpl::SessionEditorDlgImpl(QWidget* pParent, const string& strSessFile) : QDialog(pParent, getDialogWndFlags()), Ui::SessionEditorDlg(), m_bNew(false)
{
    SessionSettings st (strSessFile);
    {
        CommonData commonData(st, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
        st.loadMiscConfigSettings(&commonData, SessionSettings::DONT_INIT_GUI);
        m_strTranslation = commonData.m_strTranslation;
    }

    commonConstr();

    setWindowTitle();
    m_pFileNameE->setReadOnly(true);
    m_pFileNameE->setText(toNativeSeparators(convStr(strSessFile)));
    m_pFileNameB->hide();
    m_pOpenLastCkB->hide();
    //m_pLoadB->hide();
    m_pOpenSessionsB->hide();

    m_strSessFile = strSessFile;
    CB_ASSERT (!m_strSessFile.empty());

    //SessionSettings st (m_strSessFile);
    vector<string> vstrCheckedDirs, vstrUncheckedDirs;
    st.loadDirs(vstrCheckedDirs, vstrUncheckedDirs);

    m_pScanAtStartupCkB->setChecked(st.loadScanAtStartup());

    TransfConfig tc;
    st.loadTransfConfig(tc);
    if (TransfConfig::Options::PO_MOVE_OR_ERASE == tc.m_options.m_eProcOrigChange)
    {
        m_pCreateBackupRB->setChecked(true);
    }
    else
    {
        m_pDontCreateBackupRB->setChecked(true);
    }
    m_pBackupE->setText(toNativeSeparators(convStr(tc.getProcOrigDir())));

    m_pDirModel->setDirs(vstrCheckedDirs, vstrUncheckedDirs, m_pDirectoriesT);

    QTimer::singleShot(1, this, SLOT(onShow()));
}


SessionEditorDlgImpl::~SessionEditorDlgImpl()
{
    delete m_pDirModel;
}

void SessionEditorDlgImpl::onShow()
{
    m_pDirModel->expandNodes(m_pDirectoriesT);
}


void SessionEditorDlgImpl::setWindowTitle()
{
    QDialog::setWindowTitle(m_bNew ? tr("MP3 Diags - Create new session") : tr("MP3 Diags - Edit session"));
}

// returns the name of an INI file for OK and an empty string for Cancel; returns "*" to just go to the sessions dialog;
string SessionEditorDlgImpl::run()
{
    GlobalSettings gs;
    int nWidth, nHeight;
    gs.loadSessionEdtSize(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 400) { resize(nWidth, nHeight); }

    if (QDialog::Accepted != exec()) { return ""; }

    gs.saveSessionEdtSize(width(), height());
    m_strTranslation = TranslatorHandler::getGlobalTranslator().getTranslations()[m_pTranslationCbB->currentIndex()];

    return m_strSessFile;
}


void SessionEditorDlgImpl::on_m_pOkB_clicked()
{
    m_bOpenLastSession = m_pOpenLastCkB->isChecked();
    vector<string> vstrCheckedDirs;
    vector<string> vstrUncheckedDirs;

    if (m_bNew)
    {
        QString qstrFile (fromNativeSeparators(m_pFileNameE->text()));
        if (!qstrFile.isEmpty() && !qstrFile.endsWith(SESS_EXT)) { qstrFile += SESS_EXT; m_pFileNameE->setText(toNativeSeparators(qstrFile)); }
        //m_strSessFile = convStr(QFileInfo(qstrFile).canonicalFilePath());
        m_strSessFile = convStr(qstrFile);
        if (m_strSessFile.empty())
        {
            showCritical(this, tr("Error"), tr("You need to specify the name of the settings file.\n\nThis is supposed to be a file that doesn't already exist. You don't need to set it up, but just to pick a name for it. MP3 Diags will store its settings in this file."));
            on_m_pFileNameB_clicked();
            return;
        }
    }

    vstrCheckedDirs = m_pDirModel->getCheckedDirs();

    if (vstrCheckedDirs.empty())
    {
        showCritical(this, tr("Error"), tr("You need to select at least a directory to be included in the session."));
        return;
    }

    if (m_pCreateBackupRB->isChecked())
    {
        QString s (fromNativeSeparators(m_pBackupE->text()));
        if (s.isEmpty() || !QFileInfo(s).isDir())
        {
            showCritical(this, tr("Error"), tr("If you want to create backups, you must select an existing directory to store them."));
            return;
        }
    }

    vstrUncheckedDirs = m_pDirModel->getUncheckedDirs();

    {
        if (m_bNew)
        {
            removeSession(m_strSessFile);
        }

        SessionSettings st (m_strSessFile);

        TransfConfig tc;
        if (!m_bNew)
        {
            st.loadTransfConfig(tc);
        }

        if (m_pDontCreateBackupRB->isChecked())
        {
            tc.m_options.m_eProcOrigChange = TransfConfig::Options::PO_ERASE; //ttt2 inconsistency with how config handles this; perhaps just hide the backup settings for existing sessions
        }
        else
        {
            tc.m_options.m_eProcOrigChange = TransfConfig::Options::PO_MOVE_OR_ERASE;
            tc.setProcOrigDir(fromNativeSeparators(convStr(m_pBackupE->text())));
        }

        st.saveTransfConfig(tc);

        st.saveDirs(vstrCheckedDirs, vstrUncheckedDirs);

        st.saveScanAtStartup(m_pScanAtStartupCkB->isChecked());

        {
            CommonData commonData(st, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
            st.loadMiscConfigSettings(&commonData, SessionSettings::DONT_INIT_GUI);
            commonData.m_strTranslation = m_strTranslation;
            st.saveMiscConfigSettings(&commonData);
        }


        if (!st.sync())
        {
            showCritical(this, tr("Error"), tr("Failed to write to file %1").arg(m_pFileNameE->text()));
            if (m_bNew)
            {
                removeSession(m_strSessFile);
            }
            return;
        }
    }

    accept();
}


void SessionEditorDlgImpl::on_m_pCancelB_clicked()
{
    reject();
}

void SessionEditorDlgImpl::on_m_pBackupB_clicked()
{
    QString s (fromNativeSeparators(m_pBackupE->text()));
    if (s.isEmpty())
    {
        s = QDir::homePath();
    }
    QFileDialog dlg (this, tr("Select folder"), s, tr("All files (*)"));
    //dlg.setAcceptMode(QFileDialog::AcceptSave);

    dlg.setFileMode(QFileDialog::Directory);
    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    s = fileNames.first();
    QFileInfo f (s);
    if (!f.isDir())
    {
        return;
    }

    m_pBackupE->setText(toNativeSeparators(s));

    m_pCreateBackupRB->setChecked(true);
}


void SessionEditorDlgImpl::on_m_pFileNameB_clicked()
{
    QString s (fromNativeSeparators(m_pFileNameE->text()));
    if (s.isEmpty())
    {
        if (m_strDir.empty())
        {
            s = QDir::homePath();
        }
        else
        {
            s = convStr(m_strDir);
        }
    }
    QFileDialog dlg (this, tr("Enter configuration file"), s, tr("MP3 Diags session files (*%1)").arg(SESS_EXT));
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    //dlg.setFileMode(QFileDialog::Directory);
    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    s = fileNames.first();

    if (!s.endsWith(SESS_EXT)) { s += SESS_EXT; }

    m_pFileNameE->setText(toNativeSeparators(s));
}


/*static*/ string SessionEditorDlgImpl::getDataFileName(const string& strSessFile)
{
    CB_ASSERT (endsWith(strSessFile, SESS_EXT));
    return strSessFile.substr(0, strSessFile.size() - SESS_EXT_LEN) + ".dat";
}

/*static*/ string SessionEditorDlgImpl::getLogFileName(const string& strSessFile)
{
    CB_ASSERT (endsWith(strSessFile, SESS_EXT));
    return strSessFile.substr(0, strSessFile.size() - SESS_EXT_LEN) + ".transf_log.txt";
}

/*static*/ string SessionEditorDlgImpl::getBaseName(const string& strSessFile)
{
    CB_ASSERT (endsWith(strSessFile, SESS_EXT));
    return strSessFile.substr(0, strSessFile.size() - SESS_EXT_LEN);
}

/*static*/ string SessionEditorDlgImpl::getTitleName(const string& strSessFile)
{
    CB_ASSERT (endsWith(strSessFile, SESS_EXT));

    string::size_type n (strSessFile.rfind(getPathSep()));
    return strSessFile.substr(n + 1, strSessFile.size() - n - SESS_EXT_LEN - 1);
}


 // removes all files associated with a session: .ini, .mp3ds, .log, .dat, _trace.txt, _step1.txt, _step2.txt
/*static*/ void SessionEditorDlgImpl::removeSession(const string& strSessFile)
{
    eraseFiles(strSessFile.substr(0, strSessFile.size() - SESS_EXT_LEN));
}


/*static*/ const char* const SessionEditorDlgImpl::SESS_EXT (".ini"); //ttt0 perhaps switch to .mp3ds (keep in mind that there may be many folders with .ini in them now); the point is to be able to double-click on a .mp3ds file
/*static*/ int SessionEditorDlgImpl::SESS_EXT_LEN (strlen(SessionEditorDlgImpl::SESS_EXT));


void SessionEditorDlgImpl::on_m_pOpenSessionsB_clicked()
{
    m_strSessFile = "*";
    accept();
}


void SessionEditorDlgImpl::on_m_pTranslationCbB_currentIndexChanged(int)
{
    const vector<string>& vstrTranslations (TranslatorHandler::getGlobalTranslator().getTranslations());
    m_strTranslation = vstrTranslations[m_pTranslationCbB->currentIndex()];

    TranslatorHandler::getGlobalTranslator().setTranslation(m_strTranslation);
    retranslateUi(this);
    setWindowTitle();
}


void SessionEditorDlgImpl::onHelp()
{
    openHelp("110_first_run.html"); //ttt2 not quite right, since there are 2 "modes"
}


//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================

//ttt2 see about dereferenced symlinks


