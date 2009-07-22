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

#include  <QSettings>
#include  <QHeaderView>
#include  <QTimer>
#include  <QFileDialog>
#include  <QMessageBox>

#include  "SessionsDlgImpl.h"

#include  "Helpers.h"
#include  "StoredSettings.h"
#include  "CheckedDir.h"
#include  "SessionEditorDlgImpl.h"
#include  "OsFile.h"
#include  "Widgets.h"

using namespace std;

//========================================================================================================================
//========================================================================================================================
//========================================================================================================================


SessionsModel::SessionsModel(std::vector<std::string>& vstrSessions) : m_vstrSessions(vstrSessions)
{
}


/*override*/ int SessionsModel::rowCount(const QModelIndex&) const { return cSize(m_vstrSessions); }

/*override*/ QVariant SessionsModel::data(const QModelIndex& index, int nRole /*= Qt::DisplayRole*/) const
{
    if (!index.isValid()) { return QVariant(); }
    int i (index.row());
    //int j (index.column());

    // ttt2 perhaps Qt::ToolTipRole
    if (Qt::DisplayRole != nRole) { return QVariant(); }
    return toNativeSeparators(convStr(m_vstrSessions.at(i)));
}


/*override*/ QVariant SessionsModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /*= Qt::DisplayRole*/) const
{
    if (nRole != Qt::DisplayRole) { return QVariant(); }

    if (Qt::Horizontal == eOrientation)
    {
        return "File name";
    }

    return nSection + 1;
}


//========================================================================================================================
//========================================================================================================================
//========================================================================================================================



extern int CELL_HEIGHT;
const QFont& getDefaultFont();

SessionsDlgImpl::SessionsDlgImpl(QWidget* pParent) : QDialog(pParent, getMainWndFlags()), Ui::SessionsDlg(), m_sessionsModel(m_vstrSessions)
{
    {
        QApplication::setFont(getDefaultFont());
        CELL_HEIGHT = QApplication::fontMetrics().height() + 3; //ttt1 hard-coded
    }

    setupUi(this);

    vector<string> vstrSess;
    string strLast;

    //m_pSettings = SessionSettings::getGlobalSettings();
    GlobalSettings st;
    bool bOpenLast;
    st.loadSessions(m_vstrSessions, strLast, bOpenLast);
    m_pOpenLastCkB->setChecked(bOpenLast);

    m_pSessionsG->verticalHeader()->setResizeMode(QHeaderView::Interactive);
    m_pSessionsG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT + 1);
    m_pSessionsG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT + 1);//*/
    m_pSessionsG->setModel(&m_sessionsModel);
    m_pSessionsG->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    m_pSessionsG->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    m_pSessionsG->horizontalHeader()->hide();

    m_pCheckedDirModel = new CheckedDirModel(this, CheckedDirModel::NOT_USER_CHECKABLE);
    m_pDirectoriesT->header()->hide();
    m_pDirectoriesT->setModel(m_pCheckedDirModel);
    m_pCheckedDirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::Drives);
    m_pCheckedDirModel->setSorting(QDir::IgnoreCase);
    m_pDirectoriesT->header()->setStretchLastSection(false); m_pDirectoriesT->header()->setResizeMode(0, QHeaderView::ResizeToContents);

    QPalette grayPalette (m_pDirectoriesT->palette());
    grayPalette.setColor(QPalette::Base, grayPalette.color(QPalette::Disabled, QPalette::Window));
    m_pDirectoriesT->setPalette(grayPalette);

    //m_sessionsModel.emitLayoutChanged();

    connect(m_pSessionsG->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onCrtSessChanged()));
    connect(m_pSessionsG, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onSessDoubleClicked(const QModelIndex&)));
    //m_pSessionsG->setCurrentIndex(m_sessionsModel.index(nLast, 0));
    if (strLast.empty() && !m_vstrSessions.empty())
    {
        strLast = m_vstrSessions.back();
    }

    if (!strLast.empty())
    {
        selectSession(strLast);
    }

    {
        int nWidth, nHeight;
        st.loadSessionsDlgSize(nWidth, nHeight);
        if (nWidth > 400 && nHeight > 400) { resize(nWidth, nHeight); }
    }

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }

    QTimer::singleShot(1, this, SLOT(onShow()));
}

SessionsDlgImpl::~SessionsDlgImpl()
{
    GlobalSettings st;
    st.saveSessionsDlgSize(width(), height());

    {
        vector<string> vstrSess;
        string strLast;

        //m_pSettings = SessionSettings::getGlobalSettings();
        //GlobalSettings st;
        vector<string> v;
        bool bOpenLast;
        st.loadSessions(v, strLast, bOpenLast);
        st.saveSessions(m_vstrSessions, getCrtSession(), m_pOpenLastCkB->isChecked());
    }
}

// ttt1 generic inconsistency in what is saved depending on the user clicking the "x" button, pressing ESC, clicking other button ...


void SessionsDlgImpl::onShow()
{
    m_pCheckedDirModel->expandNodes(m_pDirectoriesT);
}


string SessionsDlgImpl::run()
{
    if (QDialog::Accepted != exec()) { return ""; }

    return getCrtSession();
}

void GlobalSettings::saveSessionsDlgSize(int nWidth, int nHeight)
{
    m_pSettings->setValue("sessions/width", nWidth);
    m_pSettings->setValue("sessions/height", nHeight);
}

void GlobalSettings::loadSessionsDlgSize(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("sessions/width").toInt();
    nHeight = m_pSettings->value("sessions/height").toInt();
}


void GlobalSettings::saveSessionEdtSize(int nWidth, int nHeight)
{
    m_pSettings->setValue("sessionEditor/width", nWidth);
    m_pSettings->setValue("sessionEditor/height", nHeight);
}

void GlobalSettings::loadSessionEdtSize(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("sessionEditor/width").toInt();
    nHeight = m_pSettings->value("sessionEditor/height").toInt();
}



void SessionsDlgImpl::onCrtSessChanged()
{
    int i (m_pSessionsG->selectionModel()->currentIndex().row());
    int n (cSize(m_vstrSessions));
    if (i < 0 || i >= n)
    {
        return;
    }

    SessionSettings st (m_vstrSessions[i]);
    vector<string> vstrCheckedDirs, vstrUncheckedDirs;

    bool bOk (st.loadDirs(vstrCheckedDirs, vstrUncheckedDirs));
    if (!bOk)
    { //ttt1 some warning
    }

    m_pCheckedDirModel->setDirs(vstrCheckedDirs, vstrUncheckedDirs, m_pDirectoriesT);
}


void SessionsDlgImpl::selectSession(const string& strLast)
{
    vector<string>::iterator it (std::find(m_vstrSessions.begin(), m_vstrSessions.end(), strLast));
    CB_ASSERT (m_vstrSessions.end() != it);
    int k (it - m_vstrSessions.begin());
    m_sessionsModel.emitLayoutChanged();
    m_pSessionsG->setCurrentIndex(m_sessionsModel.index(k, 0));
    onCrtSessChanged();
}




string SessionsDlgImpl::getCrtSession() const
{
    int i (m_pSessionsG->currentIndex().row());
    if (i < 0 || i >= cSize(m_vstrSessions)) { return ""; }
    return m_vstrSessions[i];
}



string SessionsDlgImpl::getCrtSessionDir() const
{
    string s (getCrtSession());
    if (!s.empty())
    {
        s = getParent(s);
    }
    return s;
}

void SessionsDlgImpl::addSession(const std::string& strSession)
{
    int n (cSize(m_vstrSessions));
    for (int i = 0; i < n; ++i)
    {
        if (m_vstrSessions[i] == strSession) //ttt2 ignore case on windows
        {
            m_vstrSessions.erase(m_vstrSessions.begin() + i);
            --n;
            break;
        }
    }
    m_vstrSessions.push_back(strSession);

    selectSession(strSession);

    GlobalSettings st;
    st.saveSessions(m_vstrSessions, strSession, m_pOpenLastCkB->isChecked());
}


void SessionsDlgImpl::on_m_pNewB_clicked()
{
    string strSession;
    {
        SessionEditorDlgImpl dlg (this, getCrtSessionDir(), SessionEditorDlgImpl::NOT_FIRST_TIME);
        strSession = dlg.run();
        if (strSession.empty())
        {
            return;
        }
    }

    addSession(strSession);
}


void SessionsDlgImpl::on_m_pEditB_clicked()
{
    string strSession (getCrtSession());
    if (strSession.empty())
    {
        return;
    }

    SessionEditorDlgImpl dlg (this, strSession);
    dlg.run();

    //m_sessionsModel.emitLayoutChanged();
    onCrtSessChanged();
}


void SessionsDlgImpl::removeCrtSession()
{
    int k (m_pSessionsG->currentIndex().row());
    int n (cSize(m_vstrSessions));
    if (k < 0 || k >= n) { return; }
    m_vstrSessions.erase(m_vstrSessions.begin() + k);

    //m_sessionsModel.emitLayoutChanged();

    if (k == n - 1)
    {
        --k;
    }

    string strCrtSess;
    if (k >= 0)
    {
        strCrtSess = m_vstrSessions[k];
        selectSession(strCrtSess);
    }
    else
    {
        vector<string> v;
        m_pCheckedDirModel->setDirs(v, v, m_pDirectoriesT); // to uncheck all dirs
    }

    GlobalSettings st;
    st.saveSessions(m_vstrSessions, strCrtSess, m_pOpenLastCkB->isChecked());
}


void SessionsDlgImpl::on_m_pEraseB_clicked()
{
    if (m_vstrSessions.empty()) { return; }
    if (0 != showMessage(this, QMessageBox::Question, 1, 1, "Confirm", "Do you really want to erase the current session?", "Erase", "Cancel")) { return; }
    string s (getCrtSession());
    removeCrtSession();

    try
    {
        deleteFile(s);
        deleteFile(SessionEditorDlgImpl::getDataFileName(s));
        if (fileExists(SessionEditorDlgImpl::getLogFileName(s)))
        {
            deleteFile(SessionEditorDlgImpl::getLogFileName(s));
        }
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...) //ttt1 use specific exceptions
    {
        QMessageBox::critical(this, "Error", "Failed to remove the data files associated with this session"); // maybe the files were already deleted ...
        return;
    }
}


//ttt1 perhaps: when choosing dirs show them in the title bar or a label
void SessionsDlgImpl::on_m_pSaveAsB_clicked()
{
    if (m_vstrSessions.empty()) { return; }
    //string s (getCrtSession());

    QFileDialog dlg (this, "Save session as ...", convStr(getCrtSessionDir()), "INI files (*.ini)");
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    string s (convStr(fileNames.first()));
    if (!endsWith(s, ".ini")) { s += ".ini"; }

    string strCrt (getCrtSession());
    if (s == strCrt) { return; }

    try
    {
        copyFile2(strCrt, s);
        copyFile2(SessionEditorDlgImpl::getDataFileName(strCrt), SessionEditorDlgImpl::getDataFileName(s));
        if (fileExists(SessionEditorDlgImpl::getLogFileName(strCrt)))
        {
            copyFile2(SessionEditorDlgImpl::getLogFileName(strCrt), SessionEditorDlgImpl::getLogFileName(s));
        }
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...)
    { //ttt1 show errors
    }
    addSession(s);
}


void SessionsDlgImpl::on_m_pHideB_clicked()
{
    if (m_vstrSessions.empty()) { return; }
    if (0 != showMessage(this, QMessageBox::Question, 1, 1, "Confirm", "Do you really want to hide the current session?", "Hide", "Cancel")) { return; }
    removeCrtSession();
}


void SessionsDlgImpl::on_m_pLoadB_clicked()
{
    QFileDialog dlg (this, "Choose a session file", convStr(getCrtSessionDir()), "INI files (*.ini)");
    dlg.setAcceptMode(QFileDialog::AcceptOpen);

    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    string s (convStr(fileNames.first()));
    CB_ASSERT (endsWith(s, ".ini"));

    for (int i = 0, n = cSize(m_vstrSessions); i < n; ++i)
    {
        if (m_vstrSessions[i] == s)
        {
            QMessageBox::critical(this, "Error", "The session named \"" + convStr(s) + "\" is already part of the session list");
            return;
        }
    }

    addSession(s);
}


void SessionsDlgImpl::on_m_pOpenB_clicked()
{
    if (getCrtSession().empty())
    {
        QMessageBox::critical(this, "Error", "The session list is empty. You must create a new session or load an existing one.");
        return;
    }
    accept();
}


void SessionsDlgImpl::on_m_pCancelB_clicked()
{
    reject();
}


void SessionsDlgImpl::onSessDoubleClicked(const QModelIndex& index)
{
    selectSession(m_vstrSessions.at(index.row()));
    on_m_pOpenB_clicked();
}

void SessionsDlgImpl::onHelp()
{
    openHelp("310_advanced.html");
}

