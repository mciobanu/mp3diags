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
#include  <sstream>

#include  <QFileDialog>
#include  <QKeyEvent>
#include  <QStackedLayout>
#include  <QHeaderView>
#include  <QTimer>
#include  <QDesktopWidget>
#include  <QToolTip>

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
#include  "NormalizeDlgImpl.h"
#include  "DebugDlgImpl.h"
#include  "TagEditorDlgImpl.h"
#include  "Mp3TransformThread.h"
#include  "FileRenamerDlgImpl.h"
#include  "ScanDlgImpl.h"
#include  "SessionEditorDlgImpl.h"


using namespace std;
using namespace pearl;


//ttt2 try to switch from QDialog to QWidget, to see if min/max in gnome show up; or add Qt::Dialog flag (didn't seem to work, though)

MainFormDlgImpl* getGlobalDlg();  //ttt1 remove

void trace(const string& s)
{
    MainFormDlgImpl* p (getGlobalDlg());
    //p->m_pContentM->append(convStr(s));
    //p->m_pCommonData->m_qstrContent += convStr(s);
    //p->m_pCommonData->m_qstrContent += "\n";
    if (0 != p)
    {
        p->m_pCommonData->trace(s);
    }
}

static QString s_strAssertTitle ("Assertion failure");
static QString s_strAssertMsg;
static bool s_bMainAssertOut;

/*static QString replaceDblQuotes(const QString& s)
{
    QString s1 (s);
    for (;;)
    {
        int k (s1.indexOf('\"'));
        if (-1 == k) { break; }
        s1.replace(k, 1, "&quot;");
    }

    for (;;)
    {
        int k (s1.indexOf('#')); //ttt1 this is not right, but everything after # gets truncated, even if "&num;" is used; see if # can be included
        if (-1 == k) { break; }
        s1.replace(k, 1, " ");
    }

    return s1;
}*/




static void showAssertDlg(QWidget* pParent)
{
    //QMessageBox dlg (QMessageBox::Critical, s_strAssertTitle, s_strAssertMsg, QMessageBox::Close, 0, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint); // ttt1 this might fail / crash, as it may be called from a secondary thread

//getDialogWndFlags
    QDialog dlg (pParent, Qt::Dialog | getNoResizeWndFlags() | Qt::WindowStaysOnTopHint); // Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint |
    //QDialog dlg (pParent, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);

    dlg.setWindowTitle(s_strAssertTitle);
    dlg.setWindowIcon(QIcon(":/images/logo.svg"));
    QVBoxLayout* pLayout (new QVBoxLayout(&dlg));
    //delete dlg.layout();
    //dlg.setLayout(pLayout);
    /*QLabel* pTitle (new QLabel(&dlg));
    pTitle->setText(s_strAssertTitle);
    pLayout->addWidget(pTitle);*/

    QTextBrowser* pContent (new QTextBrowser(&dlg));

    QString qstrVer (getSystemInfo());


    pContent->setOpenExternalLinks(true);
    //QString s ("<p/>Please notify <a href=\"mailto:ciobi@inbox.com?subject=000 MP3 Diags assertion failure&body=" + replaceDblQuotes(Qt::escape(s_strAssertMsg + " " + qstrVer)) + "\">ciobi@inbox.com</a> about this. (If your email client is properly configured, it's enough to click on the account name and then send.) <p/>Alternatively, you can report the bug at the <a href=\"http://sourceforge.net/forum/forum.php?forum_id=947207\">MP3 Diags Help Forum</a> (<a href=\"http://sourceforge.net/forum/forum.php?forum_id=947207\">http://sourceforge.net/forum/forum.php?forum_id=947207</a>)");

    QString s ("<p/>Please report this issue on the <a href=\"http://sourceforge.net/apps/mantisbt/mp3diags/\">MP3 Diags Issue Tracker</a> (<a href=\"http://sourceforge.net/apps/mantisbt/mp3diags/\">http://sourceforge.net/apps/mantisbt/mp3diags/</a>). Make sure to include the data below, as well as any other detail that seems relevant (what might have caused the failure, steps to reproduce it, ...)<p/><p/><hr/><p/>");

//qDebug("%s", s.toUtf8().data());
    pContent->setHtml(Qt::escape(s_strAssertMsg) + s + Qt::escape(s_strAssertMsg) + "<p/>" + qstrVer);
    pLayout->addWidget(pContent);

    QHBoxLayout btnLayout;
    btnLayout.addStretch(0);
    QPushButton* pBtn (new QPushButton("Exit", &dlg));
    btnLayout.addWidget(pBtn);
    QObject::connect(pBtn, SIGNAL(clicked()), &dlg, SLOT(accept()));

    pLayout->addLayout(&btnLayout);

    dlg.resize(750, 300);

    dlg.exec();
}

void logAssert(const char* szFile, int nLine, const char* szCond)
{
    qDebug("Assertion failure in file %s, line %d: %s", szFile, nLine, szCond);
    //QMessageBox::critical(0, "Assertion failure", QString("Assertion failure in file %1, line %2: %3").arg(szFile).arg(nLine).arg(szCond), QMessageBox::Close);
    /*QMessageBox dlg (QMessageBox::Critical, "Assertion failure", QString("Assertion failure in file %1, line %2: %3").arg(szFile).arg(nLine).arg(szCond), QMessageBox::Close, getThreadLocalDlgList().getDlg(), Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);*/

    s_strAssertMsg = QString("Assertion failure in file %1, line %2: %3").arg(szFile).arg(nLine).arg(szCond);

    MainFormDlgImpl* p (getGlobalDlg());

    if (0 != p)
    {
        s_bMainAssertOut = false;
        //QTimer::singleShot(1, p, SLOT(onShowAssert())); //ttt1 see why this doesn't work
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
        /*QMessageBox dlg (QMessageBox::Critical, s_strAssertTitle, s_strAssertMsg, QMessageBox::Close, 0, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint); // ttt1 this might fail / crash, as it may be called from a secondary thread

        dlg.exec();*/
        showAssertDlg(0);
    }
}

void MainFormDlgImpl::onShowAssert()
{
    /*QMessageBox dlg (QMessageBox::Critical, s_strAssertTitle, s_strAssertMsg, QMessageBox::Close, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);

    dlg.exec();*/
    showAssertDlg(this);

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
    bool bRes (true); //ttt1 ? use
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
                s = "An unknown note was found in the configuration. This note wasn't found:\n\n" + convStr(vstrNotFoundNotes[0]);
            }
            else
            {
                s = "An unknown note was found in the configuration. These notes weren't found:\n\n";
                for (int i = 0; i < n; ++i)
                {
                    s += convStr(vstrNotFoundNotes[i]);
                    if (i < n - 1)
                    {
                        s += "\n";
                    }
                }
            }

            QMessageBox::warning(this, "Error setting up the \"ignored notes\" list", s + "\n\nYou may want to check again the list and add any notes that you want to ignore."); //ttt2 use MP3 Diags icon
        }
    }
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


/*

Signals:

currentFileChanged() - sent by m_pCommonData->m_pFilesModel and received by both MainFormDlgImpl (to create a new m_pTagDetailsW) and CommonData (to update current notes and streams, which calls m_pStreamsModel->emitLayoutChanged() and m_pNotesModel->emitLayoutChanged())

filterChanged() - sent by m_pCommonData->m_filter and received by m_pCommonData; it updates m_pCommonData->m_vpFltHandlers and calls m_pCommonData->updateWidgets(); updateWidgets() calls m_pFilesModel->selectRow(), which triggers currentFileChanged(), which causes the note and stream grids to be updated


// ttt1 finish documenting the signal flow (mainly changing of "current" for both main window and tag editor); then check that it is properly implemented; pay attention to not calling signal handlers directly unless there's a very good reason to do so


*/


static MainFormDlgImpl* s_pGlobalDlg (0); //ttt2 review this

MainFormDlgImpl* getGlobalDlg()
{
    return s_pGlobalDlg;
}

static PausableThread* s_pSerThread;
PausableThread* getSerThread() //ttt1 global function
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
        CompleteNotif notif(this);

        bool bAborted (!load());

        notif.setSuccess(!bAborted);
    }

    bool load()
    {
        m_strErr = m_pCommonData->load(SessionEditorDlgImpl::getDataFileName(m_strSession));
        m_pCommonData->m_strTransfLog = SessionEditorDlgImpl::getLogFileName(m_strSession);
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
        CompleteNotif notif(this);

        bool bAborted (!load());

        notif.setSuccess(!bAborted);
    }

    bool load()
    {
        m_strErr = m_pCommonData->save(SessionEditorDlgImpl::getDataFileName(m_strSession));
        return true;
    }
};

/*
// a subset of m_vpExisting  gets copied to m_vpDel; so if m_vpExisting is empty, m_vpDel will be empty too;
bool SerLoadThread::scan()
{
    //cout << "################### procRec(" << strDir << ")\n";
    //FileSearcher fs ((strDir + "/ *").c_str());
    m_fileEnum.reset();

    for (;;)
    {
        string strName (m_fileEnum.next());
        if (strName.empty()) { return true; }
        if (endsWith(strName, ".mp3") || endsWith(strName, ".MP3"))
        {
            if (isAborted()) { return false; }
            checkPause();

            StrList l;
            l.push_back(convStr(strName));
            emit stepChanged(l);
            if (!m_bForce)
            {
                deque<const Mp3Handler*>::iterator it (lower_bound(m_vpExisting.begin(), m_vpExisting.end(), strName, CmpMp3HandlerPtrByName()));
                if (m_vpExisting.end() != it && (*it)->getName() == strName && !(*it)->needsReload())
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
            catch (const Mp3Handler::FileNotFound&) //ttt1 see if it should catch more
            {
            }
        }
    }
}*/

} // namespace



MainFormDlgImpl::MainFormDlgImpl(const string& strSession, bool bUniqueSession) : QDialog(0, getMainWndFlags()), m_settings(strSession), m_nLastKey(0)/*, m_settings("Ciobi", "Mp3Diags_v01")*/ /*, m_nPrevTabIndex(-1), m_bTagEdtWasEntered(false)*/, m_strSession(strSession), m_bShowMaximized(false), m_nScanWidth(0)
{
//int x (2); CB_ASSERT(x > 4);
//CB_ASSERT("345" == "ab");
//CB_ASSERT(false);
    s_pGlobalDlg = this;
    setupUi(this);

    {
        /*KbdNotifTableView* pStreamsG (new KbdNotifTableView(m_pStreamsG));
        connect(pStreamsG, SIGNAL(keyPressed(int)), this, SLOT(onStreamsGKeyPressed(int)));

        m_pStreamsG = pStreamsG;*/
        m_pStreamsG->installEventFilter(this);
    }

    m_pCommonData = new CommonData(m_settings, m_pFilesG, m_pNotesG, m_pStreamsG, m_pUniqueNotesG, /*m_pCurrentFileG, m_pCurrentAlbumG,*/ /*m_pLogG,*/ /*m_pAssignedB,*/ m_pNoteFilterB, m_pDirFilterB, m_pModeAllB, m_pModeAlbumB, m_pModeSongB, bUniqueSession);

    m_settings.loadMiscConfigSettings(m_pCommonData);
    m_pCommonData->m_bScanAtStartup = m_settings.loadScanAtStartup();

    {
        m_pCommonData->m_pFilesModel = new FilesModel(m_pCommonData);
        m_pFilesG->setModel(m_pCommonData->m_pFilesModel);

        FilesGDelegate* pDel (new FilesGDelegate(m_pCommonData, m_pFilesG));
        m_pFilesG->setItemDelegate(pDel);

        m_pFilesG->setHorizontalHeader(new FileHeaderView(m_pCommonData, Qt::Horizontal, m_pFilesG));

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
        m_settings.loadTransfConfig(m_transfConfig);

        bool bAllTransfEmpty (true);
        for (int i = 0; i < CUSTOM_TRANSF_CNT; ++i)
        {
            loadCustomTransf(i);
            bAllTransfEmpty = bAllTransfEmpty && m_pCommonData->getCustomTransf()[i].empty();
        }

        if (bAllTransfEmpty)
        {
            vector<vector<int> > vv (CUSTOM_TRANSF_CNT);
            for (int i = 0; i < CUSTOM_TRANSF_CNT; ++i)
            {
                initDefaultCustomTransf(i, vv, m_pCommonData);
            }
            m_pCommonData->setCustomTransf(vv);
        }
    }
//ttt1 perhaps have "experimental" transforms, different color (or just have the names begin with "experimental")
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

    //delete m_pSpacing01W;
/*
    {
        string strErr;
        SerLoadThread* p (new SerLoadThread(m_pCommonData, strSession, strErr));

        ThreadRunnerDlgImpl dlg (p, ThreadRunnerDlgImpl::SHOW_COUNTER, this);
        CB_ASSERT (m_nScanWidth > 400);
        dlg.resize(m_nScanWidth, dlg.height());
        dlg.setWindowIcon(QIcon(":/images/logo.svg"));
        //dlg.setWindowTitle(bForce ? "Scanning MP3 files" : "x" : reloading / all / list / "Reloading all MP3 files" : "Reloading selected MP3 files"); //ttt1
        dlg.setWindowTitle("Loading data");
        s_pSerThread = p;
        dlg.exec();
        s_pSerThread = 0;
        m_nScanWidth = dlg.width();

        if (!strErr.empty())
        {
            QMessageBox::critical(this, "Error", "An error occured while loading the MP3 information. You will have to scan your files again.\n\n" + convStr(strErr));
        }
    }

    if (m_pCommonData->m_bScanAtStartup)
    {
        CommonData::ViewMode eMode (m_pCommonData->getViewMode());
        m_pCommonData->setViewMode(CommonData::ALL, m_pCommonData->getCrtMp3Handler());
        reload(RELOAD_ALL, DONT_FORCE);
        m_pCommonData->setViewMode(eMode, m_pCommonData->getCrtMp3Handler());
    }
*/
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
    openHelp("130_main_window.html");
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

        if (r.width() - nWidth < nApprox && r.height() - nHeight - nTitleHeight < nApprox) //ttt1 no idea how this works on Vista (+Aero)
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

    if (!m_pCommonData->m_bShowDebug)
    {
        m_pDebugB->hide();
    }

    if (!m_pCommonData->m_bShowSessions)
    {
        m_pSessionsB->hide();
    }

    resizeIcons();
}


void MainFormDlgImpl::onShow()
{

    {
        string strErr;
        SerLoadThread* p (new SerLoadThread(m_pCommonData, m_strSession, strErr));

        ThreadRunnerDlgImpl dlg (this, getNoResizeWndFlags(), p, ThreadRunnerDlgImpl::SHOW_COUNTER, ThreadRunnerDlgImpl::TRUNCATE_BEGIN, ThreadRunnerDlgImpl::HIDE_PAUSE_ABORT);
        CB_ASSERT (m_nScanWidth > 400);
        dlg.resize(m_nScanWidth, dlg.height());
        dlg.setWindowIcon(QIcon(":/images/logo.svg"));
        //dlg.setWindowTitle(bForce ? "Scanning MP3 files" : "x" : reloading / all / list / "Reloading all MP3 files" : "Reloading selected MP3 files"); //ttt1
        dlg.setWindowTitle("Loading data");
        s_pSerThread = p;
        dlg.exec();
        s_pSerThread = 0;
        m_nScanWidth = dlg.width();
        m_pCommonData->setCrtAtStartup();

        if (!strErr.empty())
        {
            QMessageBox::critical(this, "Error", "An error occured while loading the MP3 information. You will have to scan your files again.\n\n" + convStr(strErr));
        }
    }

    if (m_pCommonData->m_bScanAtStartup)
    {
        fullReload();
    }//*/
//qDebug("pppp");
    resizeEvent(0);
    //if (m_pCommonData->getViewHandlers().empty())

    // !!! without these the the file grid may look bad if it has a horizontal scrollbar and one of the last files is current
    string strCrt (m_pCommonData->getCrtName());
    m_pFilesG->setCurrentIndex(m_pFilesG->model()->index(0, 0));
    m_pCommonData->updateWidgets(strCrt);
}


void MainFormDlgImpl::fullReload()
{
    CommonData::ViewMode eMode (m_pCommonData->getViewMode());
    m_pCommonData->setViewMode(CommonData::ALL, m_pCommonData->getCrtMp3Handler());
    m_pCommonData->m_filter.disableAll();
    reload(IGNORE_SEL, DONT_FORCE);
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
    //dlg.setWindowTitle(bForce ? "Scanning MP3 files" : "x" : reloading / all / list / "Reloading all MP3 files" : "Reloading selected MP3 files"); //ttt1
    dlg.setWindowTitle("Saving data");
    s_pSerThread = p;
    dlg.exec();
    s_pSerThread = 0;
    m_nScanWidth = dlg.width();

    if (!strErr.empty())
    {
        QMessageBox::critical(this, "Error", "An error occured while saving the MP3 information. You will have to scan your files again.\n\n" + convStr(strErr));
    }
}



MainFormDlgImpl::~MainFormDlgImpl()
{
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

    //QMessageBox dlg (this); dlg.show();
    //CursorOverrider crs;// (Qt::ArrowCursor);


    delete m_pCommonData;
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
    pLayout->setSpacing(6);
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

    m_pCrtDirE->setText(toNativeSeparators(convStr(m_pCommonData->getViewHandlers()[nCrtFile]->getDir())));

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
        CompleteNotif notif(this);

        bool bAborted (!scan());

        if (!bAborted)
        {
            sort(m_vpKeep.begin(), m_vpKeep.end(), CmpMp3HandlerPtrByName());
            set_difference(m_vpExisting.begin(), m_vpExisting.end(), m_vpKeep.begin(), m_vpKeep.end(), back_inserter(m_vpDel), CmpMp3HandlerPtrByName());
        }

        notif.setSuccess(!bAborted);
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
        if (endsWith(strName, ".mp3") || endsWith(strName, ".MP3"))
        {
            if (isAborted()) { return false; }
            checkPause();

            StrList l;
            l.push_back(toNativeSeparators(convStr(strName)));
            emit stepChanged(l);
            if (!m_bForce)
            {
                deque<const Mp3Handler*>::iterator it (lower_bound(m_vpExisting.begin(), m_vpExisting.end(), strName, CmpMp3HandlerPtrByName()));
                if (m_vpExisting.end() != it && (*it)->getName() == strName && !(*it)->needsReload())
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
            catch (const Mp3Handler::FileNotFound&) //ttt1 see if it should catch more
            {
            }
        }
    }
}

} // namespace



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
        //dlg.setWindowTitle(bForce ? "Scanning MP3 files" : "x" : reloading / all / list / "Reloading all MP3 files" : "Reloading selected MP3 files"); //ttt1
        dlg.setWindowTitle("Scanning MP3 files");
        dlg.exec(); //ttt2 perhaps see if it ended with ok/reject and clear all on reject
        m_nScanWidth = dlg.width();
        vpAdd = p->m_vpAdd;
        vpDel = p->m_vpDel;
    }

    m_pCommonData->mergeHandlerChanges(vpAdd, vpDel, nKeepWhenUpdate);
}




//ttt1 disable trace to see impact on memory
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
            QMessageBox::warning(this, "Error setting up custom transformations", "Couldn't find a transformation with the name \"" + convStr(strName) + "\". The program will proceed, but you should review the custom transformations lists.");
        }
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
    vector<string> vstrNames (m_settings.loadVector("visibleTransf", bErr));
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
            QMessageBox::warning(this, "Error setting up visible transformations", "Couldn't find a transformation with the name \"" + convStr(strName) + "\". The program will proceed, but you should review the visible transformations list.");
        }
    }

    m_pCommonData->setVisibleTransf(v);
}


//ttt2 keyboard shortcuts: next, prev, ... ;
//ttt3 set focus on some edit box when changing tabs;


//ttt1 perhaps add "whatsThis" texts
//ttt1 perhaps add "what's this" tips



void MainFormDlgImpl::on_m_pTransformB_clicked() //ttt2 an alternative is to use QToolButton::setMenu(); see if that is really simpler
{
    ModifInfoMenu menu;
    vector<QAction*> vpAct;

    const vector<Transformation*>& vpTransf (m_pCommonData->getAllTransf());
    const vector<int>& vnVisualNdx (m_pCommonData->getVisibleTransf());

    for (int i = 0, n = cSize(vnVisualNdx); i < n; ++i)
    {
        Transformation* pTransf (vpTransf.at(vnVisualNdx[i]));
        QAction* pAct (new QAction(pTransf->getActionName(), &menu));
        pAct->setToolTip(makeMultiline(pTransf->getDescription()));

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
        transform(v, 0 == (Qt::ShiftModifier & menu.getModifiers()));
    }
}


void MainFormDlgImpl::onMenuHovered(QAction* pAction)
{
    QToolTip::showText(QCursor::pos(), "");
    QToolTip::showText(QCursor::pos(), pAction->toolTip());
    // see http://www.mail-archive.com/pyqt@riverbankcomputing.com/msg17214.html and http://www.mail-archive.com/pyqt@riverbankcomputing.com/msg17245.html ; apparently there's some inconsistency in when the menus are shown
}


void MainFormDlgImpl::on_m_pNormalizeB_clicked()
{
    bool bSel (0 != (Qt::ShiftModifier & m_pModifNormalizeB->getModifiers()));
    const deque<const Mp3Handler*>& vpHndlr (bSel ? m_pCommonData->getSelHandlers() : m_pCommonData->getViewHandlers());

    QStringList l;
    for (int i = 0, n = cSize(vpHndlr); i < n; ++i)
    {
        l << vpHndlr[i]->getUiName();
    }
    if (l.isEmpty())
    {
        QMessageBox::critical(this, "Error", "There are no files to normalize.");
        return;
    }

    int nIssueCount (0);
    QString qstrWarn;
    if (bSel && cSize(m_pCommonData->getViewHandlers()) != cSize(m_pCommonData->getSelHandlers()))
    {
        ++nIssueCount;
        qstrWarn += "\n- you are requesting to normalize only some of the files";
    }

    if (CommonData::FOLDER != m_pCommonData->getViewMode())
    {
        ++nIssueCount;
        qstrWarn += "\n- the \"Album\" mode is not selected";
    }

    if (m_pCommonData->m_filter.isNoteEnabled() || m_pCommonData->m_filter.isDirEnabled())
    {
        ++nIssueCount;
        if (m_pCommonData->m_filter.isNoteEnabled() && m_pCommonData->m_filter.isDirEnabled())
        {
            qstrWarn += "\n- filters are applied";
        }
        else
        {
            qstrWarn += "\n- a filter is applied";
        }
    }

    if (cSize(vpHndlr) > 50)
    {
        ++nIssueCount;
        qstrWarn += "\n- the normalization will process more than 50 files, which is more than what an album usually has";
    }

    if (0 != nIssueCount)
    {
        QString s ("Normalization should process one whole album at a time, so it should only be run in \"Album\" mode, when no filters are active, and it should be applied to all the files in that album. But in the current case");
        if (1 == nIssueCount)
        {
            qstrWarn.remove(0, 2);
            s += qstrWarn;
            s += ".";
        }
        else
        {
            s += " there are some issues:\n" + qstrWarn;
        }

        int k (showMessage(this, QMessageBox::Warning, 1, 1, "Warning", s + "\n\nNormalize anyway?", "Normalize", "Cancel"));

        if (k != 0)
        {
            return;
        }
    }

    if (0 == nIssueCount)
    {
        if (0 != showMessage(this, QMessageBox::Question, 1, 1, "Confirm", "Normalize all the files in the current album? (Note that normalization is done \"in place\", by an external program, so it doesn't care about the transformation settings for original and modified files.)", "Normalize", "Cancel"))
        {
            return;
        }
    }

    NormalizeDlgImpl dlg (this, m_pCommonData->m_bKeepNormWndOpen, m_settings, m_pCommonData);
    dlg.normalize(convStr(m_pCommonData->m_strNormalizeCmd), l);

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
    vector<Transformation*> v;
    for (int i = 0, n = cSize(m_pCommonData->getCustomTransf()[k]); i < n; ++i)
    {
        v.push_back(m_pCommonData->getAllTransf()[m_pCommonData->getCustomTransf()[k][i]]);
    }
    transform(v, 0 == (Qt::ShiftModifier & m_vpTransfButtons[k]->getModifiers()));
}




// The file list is updated in the sense that if a file was changed or removed, this is reflected in the UI. However, new files are not seen. For one thing, rebuilding a 10000-file list takes a lot of time. OTOH perhaps just the new files could be added. Also, perhaps the user could be asked about updating the list.
//ttt1 review
void MainFormDlgImpl::transform(std::vector<Transformation*>& vpTransf, bool bAll)
{
    if (m_pCommonData->getViewHandlers().empty())
    {
        QMessageBox::warning(this, "Warning", "The file list is empty, therefore no transformations can be applied.\n\nExiting ...");
        return;
    }

    QString qstrListInfo;
    if (bAll)
    {
        char bfr [10];

        int nCnt (cSize(m_pCommonData->getViewHandlers()));
        if (nCnt < 10)
        {
            strcpy(bfr, "the");
        }
        else
        {
            sprintf(bfr, "%d", nCnt);
        }
        qstrListInfo = QString("all %1 files shown in the file list").arg(bfr);
    }
    else
    {
        int nCnt (cSize(m_pCommonData->getSelHandlers()));
        if (0 == nCnt)
        {
            QMessageBox::warning(this, "Warning", "No file is selected, therefore no transformations can be applied.\n\nExiting ...");
            return;
        }
        else if (1 == nCnt)
        {
            qstrListInfo = "\"" + convStr(m_pCommonData->getSelHandlers()[0]->getShortName()) + "\"";
        }
        else
        { //ttt2 actually this is never reached, because a single file can be selected at a time; ttt1 perhaps allow multiple selection but show the corresponding streams and notes for the "current" file only; might use different background colors for "current" and "selected"
            qstrListInfo = "\"" + convStr(m_pCommonData->getSelHandlers()[0]->getShortName()) + "\" and the other the selected file(s)";
        }
    }

    QString qstrConf;
    if (vpTransf.empty())
    {
        if (0 == m_transfConfig.m_optionsWrp.m_opt.m_nUnprocOrigChange)
        {
            QMessageBox::warning(this, "Warning", "The transformation list is empty.\n\nBased on the configuration, it is possible for changes to the files in the list to be performed, even in this case (the files may still be moved, renamed or erased). However, the current settings are to leave the original files unchanged, so currently there's no point in applying an empty transformation list.\n\nExiting ...");
            return;
        }
        qstrConf = "Apply an empty transformation list to all the files shown in the file list? (Note that even if no transformations are performed, the files may still be moved, renamed or erased, based on the current settings.)";
    }
    else if (1 == cSize(vpTransf))
    {
        qstrConf = (convStr(string("Apply transformation \"") + vpTransf[0]->getActionName() + "\" to ")) + qstrListInfo + "?";
    }
    else
    {
        qstrConf = "Apply the following transformations to " + qstrListInfo + "?";
        for (int i = 0, n = cSize(vpTransf); i < n; ++i)
        {
            qstrConf += "\n      ";
            qstrConf += vpTransf[i]->getActionName();
        }
    }

    qstrConf += "\n\nActions to be taken:";
    {
        const char* aOrig[] = { "don't change", "erase", "move", "move", "rename", "move if destination doesn't exist" };
        if (!vpTransf.empty())
        {
            qstrConf += "\n- original file that has been transformed: ";
            qstrConf += aOrig[m_transfConfig.m_optionsWrp.m_opt.m_nProcOrigChange];
        }

        {
            qstrConf += "\n- original file that has not been transformed: ";
            qstrConf += aOrig[m_transfConfig.m_optionsWrp.m_opt.m_nUnprocOrigChange];
        }
    }

    QMessageBox::StandardButton res (QMessageBox::question(this, "Confirmation", qstrConf, QMessageBox::Yes | QMessageBox::No));
    if (QMessageBox::Yes != res) { return; }

    ::transform(bAll ? m_pCommonData->getViewHandlers() : m_pCommonData->getSelHandlers(), vpTransf, "Applying transformations to MP3 files", this, m_pCommonData, m_transfConfig);
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
    QString s1;
    s1.sprintf("Apply custom transformation list #%d\n", k + 1);
    QString s2;
    for (int i = 0, n = cSize(m_pCommonData->getCustomTransf()[k]); i < n; ++i)
    {
        s2 += "    ";
        s2 += m_pCommonData->getAllTransf()[m_pCommonData->getCustomTransf()[k][i]]->getActionName();
        if (i < n - 1) { s2 += "\n"; }
    }
    if (s2.isEmpty()) { s2 = "   <empty list>\n\n(you can edit the list in the Settings dialog)"; }
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
//CB_ASSERT("345" == "ab");
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
    if (m_pCommonData->getViewHandlers().empty())
    {
        QMessageBox::critical(this, "Error", "The file list is empty. You need to populate it before opening the tag editor.");
        return;
    }

    m_pCommonData->setSongInCrtAlbum();

    TagEditorDlgImpl dlg (this, m_pCommonData, m_transfConfig);

    //if (QDialog::Accepted == dlg.exec())
    string strCrt (dlg.run());

    updateUi(strCrt); // needed because the tag editor might have called the config and changed things; it would be nicer to send a signal when config changes, but IIRC in Qt 4.3.1 resizing things in a dialog that opened another one doesn't work very well; (see also TagEditorDlgImpl::on_m_pQueryDiscogsB_clicked())

    if (m_pCommonData->useFastSave())
    {
        fullReload();
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
        QMessageBox::critical(this, "Error", "The file list is empty. You need to populate it before opening the file rename tool.");
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

    {
        saveVisibleTransf();
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
    /*override*/ const char* getDescription() const { return "Removes selected streams."; }

    static const char* getClassName() { return "Remove selected stream(s)"; }

    set<DataStream*> m_spToRemove;
};




//void MainFormDlgImpl::onStreamsGKeyPressed(int nKey)
/*override*/ bool MainFormDlgImpl::eventFilter(QObject* pObj, QEvent* pEvent)
{
//qDebug("type %d", pEvent->type());
    QKeyEvent* pKeyEvent (dynamic_cast<QKeyEvent*>(pEvent));
    int nKey (0 == pKeyEvent ? 0 : pKeyEvent->key());
    if (m_pStreamsG != pObj || 0 == pKeyEvent || Qt::Key_Delete != nKey || QEvent::ShortcutOverride != pKeyEvent->type()) { return QDialog::eventFilter(pObj, pEvent); }

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
    transform(v, SELECTED); //ttt2 currently (2009.04.20) this works OK, but there's this issue: we don't want the second param to be SELECTED, but "CURRENT", because it only applies to the streams of the current file; however, when selecting streams, all songs except for the current one get deselected; even if this changes, and more files could be selected, it will still work OK, except that the confirmation message will ask about deleting streams from several files, while it only cares about one;
    return true;
}


void MainFormDlgImpl::on_m_pViewFileInfoB_clicked()
{
    m_pLowerHalfLayout->setCurrentWidget(m_pFileInfoTab);
    m_pViewFileInfoB->setChecked(true); //ttt1 use autoExclusive instead
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
            emit stepChanged(l);
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

//ttt1 ? option to discard errors in unknown streams: probably not; at any rate, that's the only chance to learn that there was an error there (instead of a really unknown stream)

//ttt1 how to process: run an automated check, then filter the files to only show those with issues; the user chooses how to deal with those issues; then the changes are applied

//ttt1 persistent note filter


// ttt2 make sure that everything that doesn't take a "parent" on the constructor gets deleted;

//ttt1 configurable icon size

/*

Development machine:

-style=Plastique
-style=Cleanlooks
-style=CDE
-style=Motif
-style=Oxygen

?? Windows, Windows Vista, Plastik




*/

//ttt1 build: not sure is possible, but have the .pro file include another file, which is generated by a "pre-config" script, which figures out if the lib is installed

//ttt1 perhaps a "consider unknown" function for streams; then it would be possible for a truncated id3v1 tag that seems ok to be marked as unknown, thus allowing the following tag's start to be detected (see "c09 Mark Knopfler - The Long Road.mp3")

//ttt1 perhaps implement a playlist generator

//ttt1 a "reload" that only looks for new / removed files

//ttt1 handle symbolic links to ancestors

