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
}


// used for creating a new session;
SessionEditorDlgImpl::SessionEditorDlgImpl(QWidget* pParent, const string& strDir, bool bFirstTime) : QDialog(pParent, getDialogWndFlags()), Ui::SessionEditorDlg(), m_strDir(strDir), m_bNew(true)
{
    commonConstr();

    bool bAutoFileName (false);
    {
#ifndef WIN32
        QString qs (QDir::homePath() + "/Documents"); // OK on openSUSE, not sure how standardized it is
#else
        QSettings settings (QSettings::UserScope, "Microsoft", "Windows");
        settings.beginGroup("CurrentVersion/Explorer/Shell Folders");
        QString qs (fromNativeSeparators(settings.value("Personal").toString()));
#endif
        if (QFileInfo(qs).isDir())
        {
            qs += "/MP3Diags.ini";
            if (!QDir().exists(qs))
            {
                m_pFileNameE->setText(toNativeSeparators(qs));
                bAutoFileName = true;
            }
        }
    }

    //setWindowTitle("MP3 Diags - Create a new session or load an existing one");
    setWindowTitle("MP3 Diags - Create new session");
    m_pDontCreateBackupRB->setChecked(true);
    m_pScanAtStartupCkB->setChecked(true);

    m_pFileNameE->setToolTip(bAutoFileName ?
        "This is the name of the \"settings file\"\n\n"
        "It is supposed to be a file that doesn't already exist. You don't need to set it up. MP3 Diags\n"
        "will store its settings in this file.\n\n"
        "The name was generated automatically. If you want to choose a different name, simply click on\n"
        "the button at the right to change it." :

        "Here you need to specify the name of a \"settings file\"\n\n"
        "This is supposed to be a file that doesn't already exist. You don't need to set it up. MP3 Diags\n"
        "will store its settings in this file.\n\n"
        "To change it, simply click on the button at the right to choose the name of the settings file.");

    if (!bFirstTime)
    {
        m_pOpenLastCkB->hide();
        m_pOpenSessionsB->hide();
    }
}


// used for editing an existing session;
SessionEditorDlgImpl::SessionEditorDlgImpl(QWidget* pParent, const string& strIniFile) : QDialog(pParent, getDialogWndFlags()), Ui::SessionEditorDlg(), m_bNew(false)
{
    commonConstr();

    setWindowTitle("MP3 Diags - Edit session");
    m_pFileNameE->setReadOnly(true);
    m_pFileNameE->setText(toNativeSeparators(convStr(strIniFile)));
    m_pFileNameB->hide();
    m_pOpenLastCkB->hide();
    //m_pLoadB->hide();
    m_pOpenSessionsB->hide();

    m_strIniFile = strIniFile;
    CB_ASSERT (!m_strIniFile.empty());

    SessionSettings st (m_strIniFile);
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




// returns the name of an INI file for OK and an empty string for Cancel; returns "*" to just go to the sessions dialog;
string SessionEditorDlgImpl::run()
{
    GlobalSettings gs;
    int nWidth, nHeight;
    gs.loadSessionEdtSize(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 400) { resize(nWidth, nHeight); }

    if (QDialog::Accepted != exec()) { return ""; }

    gs.saveSessionEdtSize(width(), height());

    return m_strIniFile;
}



void SessionEditorDlgImpl::on_m_pOkB_clicked()
{
    m_bOpenLastSession = m_pOpenLastCkB->isChecked();
    vector<string> vstrCheckedDirs;
    vector<string> vstrUncheckedDirs;

    if (m_bNew)
    {
        QString qstrFile (fromNativeSeparators(m_pFileNameE->text()));
        if (!qstrFile.isEmpty() && !qstrFile.endsWith(".ini")) { qstrFile += ".ini"; m_pFileNameE->setText(toNativeSeparators(qstrFile)); }
        //m_strIniFile = convStr(QFileInfo(qstrFile).canonicalFilePath());
        m_strIniFile = convStr(qstrFile);
        if (m_strIniFile.empty())
        {
            QMessageBox::critical(this, "Error", "You need to specify the name of the settings file.\n\nThis is supposed to be a file that doesn't already exist. You don't need to set it up, but just to pick a name for it. MP3 Diags will store its settings in this file.");
            on_m_pFileNameB_clicked();
            return;
        }
    }

    vstrCheckedDirs = m_pDirModel->getCheckedDirs();

    if (vstrCheckedDirs.empty())
    {
        QMessageBox::critical(this, "Error", "You need to select at least a directory to be included in the session.");
        return;
    }

    if (m_pCreateBackupRB->isChecked())
    {
        QString s (fromNativeSeparators(m_pBackupE->text()));
        if (s.isEmpty() || !QFileInfo(s).isDir())
        {
            QMessageBox::critical(this, "Error", "If you want to create backups, you must select an existing directory to store them.");
            return;
        }
    }

    vstrUncheckedDirs = m_pDirModel->getUncheckedDirs();

    {
        if (m_bNew)
        {
            removeSession(m_strIniFile);
        }

        SessionSettings st (m_strIniFile);

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

        if (!st.sync())
        {
            QMessageBox::critical(this, "Error", "Failed to write to file " + m_pFileNameE->text());
            if (m_bNew)
            {
                removeSession(m_strIniFile);
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
    QFileDialog dlg (this, "Select folder", s, "All files (*)");
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
    QFileDialog dlg (this, "Enter configuration file", s, "INI files (*.ini)");
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    //dlg.setFileMode(QFileDialog::Directory);
    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    s = fileNames.first();

    if (!s.endsWith(".ini")) { s += ".ini"; }

    m_pFileNameE->setText(toNativeSeparators(s));
}


/*static*/ string SessionEditorDlgImpl::getDataFileName(const string& strIniName)
{
    CB_ASSERT (endsWith(strIniName, ".ini"));
    return strIniName.substr(0, strIniName.size() - 4) + ".dat";
}

/*static*/ string SessionEditorDlgImpl::getLogFileName(const string& strIniName)
{
    CB_ASSERT (endsWith(strIniName, ".ini"));
    return strIniName.substr(0, strIniName.size() - 4) + ".transf_log.txt";
}


/*static*/ void SessionEditorDlgImpl::removeSession(const string& strIniName) // removes INI, DAT, and LOG
{
    QFile::remove(convStr(strIniName)); // ttt2 perhaps check ...
    QFile::remove(convStr(getDataFileName(strIniName)));
    QFile::remove(convStr(getLogFileName(strIniName)));
}


void SessionEditorDlgImpl::on_m_pOpenSessionsB_clicked()
{
    m_strIniFile = "*";
    accept();
}


void SessionEditorDlgImpl::onHelp()
{
    openHelp("110_first_run.html"); //ttt2 not quite right, since there are 2 "modes"
}


//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================

//ttt2 see about dereferenced symlinks


