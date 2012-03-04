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
#include  "Translation.h"

using namespace std;

//========================================================================================================================
//========================================================================================================================
//========================================================================================================================


SessionsModel::SessionsModel(std::vector<std::string>& vstrSessions) : m_vstrSessions(vstrSessions)
{
}


/*override*/ int SessionsModel::rowCount(const QModelIndex&) const { return cSize(m_vstrSessions); }

/*override*/ QVariant SessionsModel::data(const QModelIndex& index, int nRole /* = Qt::DisplayRole*/) const
{
LAST_STEP("SessionsModel::data()");
    if (!index.isValid()) { return QVariant(); }
    int i (index.row());
    //int j (index.column());

    // ttt2 perhaps Qt::ToolTipRole
    if (Qt::DisplayRole != nRole) { return QVariant(); }
    return toNativeSeparators(convStr(m_vstrSessions.at(i)));
}


/*override*/ QVariant SessionsModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /* = Qt::DisplayRole*/) const
{
    if (nRole != Qt::DisplayRole) { return QVariant(); }

    if (Qt::Horizontal == eOrientation)
    {
        return tr("File name");
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
        CELL_HEIGHT = QApplication::fontMetrics().height() + 3; //ttt2 hard-coded
    }

    setupUi(this);

    //vector<string> vstrSess;
    string strLast;

    //m_pSettings = SessionSettings::getGlobalSettings();
    GlobalSettings st;
    bool bOpenLast;

    st.loadSessions(m_vstrSessions, strLast, bOpenLast, m_strTempSessTempl, m_strDirSessTempl, m_strTranslation);
    m_pOpenLastCkB->setChecked(bOpenLast);

    m_pSessionsG->verticalHeader()->setResizeMode(QHeaderView::Interactive);
    m_pSessionsG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT + 1); // ttt0 is this initialized before creating sessions? should it be?
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

    loadTemplates();

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


    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }

    QTimer::singleShot(1, this, SLOT(onShow()));
}

SessionsDlgImpl::~SessionsDlgImpl()
{
    GlobalSettings st;
    st.saveSessionsDlgSize(width(), height());

    {
        /*string strLast;

        //m_pSettings = SessionSettings::getGlobalSettings();
        //GlobalSettings st;
        vector<string> v;
        bool bOpenLast;

        string strTempSessTempl;
        string strDirSessTempl;
        string strTranslation;
        st.loadSessions(v, strLast, bOpenLast, strTempSessTempl, strDirSessTempl, strTranslation); //ttt0 what's the point of this? no vars are used*/
        saveTemplates();
        st.saveSessions(m_vstrSessions, getCrtSession(), m_pOpenLastCkB->isChecked(), m_strTempSessTempl, m_strDirSessTempl, m_strTranslation, GlobalSettings::LOAD_EXTERNAL_CHANGES);
    }
}

// ttt2 generic inconsistency in what is saved depending on the user clicking the "x" button, pressing ESC, clicking other button ...


// sets up the combo boxes with temp/folder session templates based on m_vstrSessions, m_strTempSessTempl, and m_strDirSessTempl
void SessionsDlgImpl::loadTemplates()
{
    m_pTempSessionCbB->clear(); m_pTempSessionCbB->addItem(tr("<last session>"));
    m_pDirSessionCbB->clear(); m_pDirSessionCbB->addItem(tr("<last session>"));

    for (int i = 0; i < cSize(m_vstrSessions); ++i)
    {
        m_pTempSessionCbB->addItem(toNativeSeparators(convStr(m_vstrSessions[i])));
        if (m_strTempSessTempl == m_vstrSessions[i])
        {
            m_pTempSessionCbB->setCurrentIndex(m_pTempSessionCbB->count() - 1);
        }

        m_pDirSessionCbB->addItem(toNativeSeparators(convStr(m_vstrSessions[i])));
        if (m_strDirSessTempl == m_vstrSessions[i])
        {
            m_pDirSessionCbB->setCurrentIndex(m_pDirSessionCbB->count() - 1);
        }
    }
}


// sets c and m_strDirSessTempl based on the current items in the combo boxes
void SessionsDlgImpl::saveTemplates()
{
    m_strTempSessTempl.clear();
    if (m_pTempSessionCbB->currentIndex() > 0)
    {
        m_strTempSessTempl = fromNativeSeparators(convStr(m_pTempSessionCbB->currentText()));
    }

    m_strDirSessTempl.clear();
    if (m_pDirSessionCbB->currentIndex() > 0)
    {
        m_strDirSessTempl = fromNativeSeparators(convStr(m_pDirSessionCbB->currentText()));
    }
}


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
    { //ttt2 some warning
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
    loadTemplates();
    saveTemplates();
    st.saveSessions(m_vstrSessions, strSession, m_pOpenLastCkB->isChecked(), m_strTempSessTempl, m_strDirSessTempl, m_strTranslation, GlobalSettings::LOAD_EXTERNAL_CHANGES);
}


void SessionsDlgImpl::on_m_pNewB_clicked()
{
    string strSession;
    {
        SessionEditorDlgImpl dlg (this, getCrtSessionDir(), SessionEditorDlgImpl::NOT_FIRST_TIME, m_strTranslation);
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
    loadTemplates();
    saveTemplates();
    st.saveSessions(m_vstrSessions, strCrtSess, m_pOpenLastCkB->isChecked(), m_strTempSessTempl, m_strDirSessTempl, m_strTranslation, GlobalSettings::IGNORE_EXTERNAL_CHANGES);
}


void SessionsDlgImpl::on_m_pEraseB_clicked()
{
    if (m_vstrSessions.empty()) { return; }
    if (0 != showMessage(this, QMessageBox::Question, 1, 1, tr("Confirm"), tr("Do you really want to erase the current session?"), tr("Erase"), tr("Cancel"))) { return; }
    string s (getCrtSession());
    removeCrtSession();

    try
    {
        //ttt0 use eraseFiles()
        deleteFile(s);
        deleteFile(SessionEditorDlgImpl::getDataFileName(s));
        if (fileExists(SessionEditorDlgImpl::getLogFileName(s)))
        {
            deleteFile(SessionEditorDlgImpl::getLogFileName(s));
        }
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...) //ttt2 use specific exceptions
    {
        showCritical(this, tr("Error"), tr("Failed to remove the data files associated with this session")); // maybe the files were already deleted ...
        return;
    }
}


//ttt2 perhaps: when choosing dirs show them in the title bar or a label
void SessionsDlgImpl::on_m_pSaveAsB_clicked()
{
    if (m_vstrSessions.empty()) { return; }
    //string s (getCrtSession());

    QFileDialog dlg (this, tr("Save session as ..."), convStr(getCrtSessionDir()), SessionEditorDlgImpl::tr("MP3 Diags session files (*%1)").arg(SessionEditorDlgImpl::SESS_EXT));
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    string s (convStr(fileNames.first()));
    if (!endsWith(s, SessionEditorDlgImpl::SESS_EXT)) { s += SessionEditorDlgImpl::SESS_EXT; }

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
    { //ttt2 show errors
    }
    addSession(s);
}


void SessionsDlgImpl::on_m_pHideB_clicked()
{
    if (m_vstrSessions.empty()) { return; }
    if (0 != showMessage(this, QMessageBox::Question, 1, 1, tr("Confirm"), tr("Do you really want to hide the current session?"), tr("Hide"), tr("Cancel"))) { return; }
    removeCrtSession();
}


void SessionsDlgImpl::on_m_pLoadB_clicked()
{
    QFileDialog dlg (this, tr("Choose a session file"), convStr(getCrtSessionDir()), SessionEditorDlgImpl::tr("MP3 Diags session files (*%1)").arg(SessionEditorDlgImpl::SESS_EXT)); //ttt0 add ".ini", for import from older versions
    dlg.setAcceptMode(QFileDialog::AcceptOpen);

    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    string s (convStr(fileNames.first()));
    CB_ASSERT (endsWith(s, SessionEditorDlgImpl::SESS_EXT));

    for (int i = 0, n = cSize(m_vstrSessions); i < n; ++i)
    {
        if (m_vstrSessions[i] == s)
        {
            showCritical(this, tr("Error"), tr("The session named \"%1\" is already part of the session list").arg(toNativeSeparators(convStr(s))));
            return;
        }
    }

    addSession(s);
}


void SessionsDlgImpl::on_m_pOpenB_clicked()
{
    if (getCrtSession().empty())
    {
        showCritical(this, tr("Error"), tr("The session list is empty. You must create a new session or load an existing one."));
        return;
    }
    accept();
}


void SessionsDlgImpl::on_m_pCloseB_clicked() // ttt0 redo screenshots Cancel->Close
{
    reject();
}


void SessionsDlgImpl::on_m_pTranslationCbB_currentIndexChanged(int)
{
    const vector<string>& vstrTranslations (TranslatorHandler::getGlobalTranslator().getTranslations());
    m_strTranslation = vstrTranslations[m_pTranslationCbB->currentIndex()];

    TranslatorHandler::getGlobalTranslator().setTranslation(m_strTranslation);
    retranslateUi(this);
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

