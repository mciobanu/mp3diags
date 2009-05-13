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


#include  <QTimer>
#include  <QPainter>
#include  <QHeaderView>

#include  "FileRenamerDlgImpl.h"

#include  "Helpers.h"
#include  "ColumnResizer.h"
#include  "CommonData.h"
#include  "DataStream.h"
#include  "RenamerPatternsDlgImpl.h"
#include  "OsFile.h"
#include  "ThreadRunnerDlgImpl.h"
#include  "StoredSettings.h"
#include  "Id3V230Stream.h"
#include  "Widgets.h"


using namespace std;
using namespace pearl;

using namespace FileRenamer;



CurrentAlbumModel::CurrentAlbumModel(CommonData* pCommonData, FileRenamerDlgImpl* pFileRenamerDlgImpl) : m_pFileRenamerDlgImpl(pFileRenamerDlgImpl), m_pCommonData(pCommonData), m_pRenamer(0)
{
}

CurrentAlbumModel::~CurrentAlbumModel()
{
    delete m_pRenamer;
}

void CurrentAlbumModel::setRenamer(const Renamer* p)
{
    delete m_pRenamer;
    m_pRenamer = p;
    emitLayoutChanged();
}

/*override*/ int CurrentAlbumModel::rowCount(const QModelIndex&) const
{
    return cSize(m_pCommonData->getCrtAlbum());
}


/*override*/ int CurrentAlbumModel::columnCount(const QModelIndex&) const
{
    return TagReader::LIST_END + 2 - 1;
}


/*override*/ QVariant CurrentAlbumModel::data(const QModelIndex& index, int nRole) const
{
    if (!index.isValid()) { return QVariant(); }
    int i (index.row());
    int j (index.column());

    if (Qt::DisplayRole != nRole && Qt::ToolTipRole != nRole) { return QVariant(); }
    QString s;

    const Mp3Handler* p (m_pCommonData->getCrtAlbum().at(i));
    const Id3V2StreamBase* pId3V2 (p->getId3V2Stream());

    if (0 == j)
    {
        s = convStr(p->getShortName());
    }
    else if (0 == pId3V2)
    {
        s = "N/A";
    }
    else if (1 == j)
    {
        //s = convStr(p->getName());
        if (0 == m_pRenamer)
        {
            s = "N/A";
        }
        else
        {
            s = convStr(m_pRenamer->getNewName(p));
        }
    }
    else
    {
        j -= 2;
        if (j >= TagReader::POS_OF_FEATURE[TagReader::IMAGE])
        {
            j += 1;
        }
        s = convStr(pId3V2->getValue((TagReader::Feature)TagReader::FEATURE_ON_POS[j]));
    }

    if (Qt::DisplayRole == nRole)
    {
        return s;
    }

    // so it's Qt::ToolTipRole

    QFontMetrics fm (m_pFileRenamerDlgImpl->m_pCurrentAlbumG->fontMetrics()); // !!! no need to use "QApplication::fontMetrics()"
    int nWidth (fm.width(s));
//qDebug("tooltip for %s, width %d
    int nMargin (QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1); // this "1" should probably be another "metric" constant
    if (nWidth + 2*nMargin + 1 <= m_pFileRenamerDlgImpl->m_pCurrentAlbumG->horizontalHeader()->sectionSize(index.column())) // ttt2 not sure this "nMargin" is correct
    {
        //return QVariant();
        return ""; // !!! with "return QVariant()" the previous tooltip remains until the cursor moves over another cell that has a tooltip
    }

    return s;
}



/*override*/ QVariant CurrentAlbumModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /*= Qt::DisplayRole*/) const
{
    if (nRole != Qt::DisplayRole) { return QVariant(); }

    if (Qt::Horizontal == eOrientation)
    {
        if (0 == nSection) { return "File name"; }
        else if (1 == nSection) { return "New file name"; }
        else
        {
            nSection -= 2;
            if (nSection >= TagReader::POS_OF_FEATURE[TagReader::IMAGE])
            {
                nSection += 1;
            }
            return TagReader::getLabel(TagReader::FEATURE_ON_POS[nSection]);
        }
    }

    return nSection + 1;
}


//======================================================================================================================
//======================================================================================================================


CurrentAlbumDelegate::CurrentAlbumDelegate(QWidget* pParent, CommonData* pCommonData) : QItemDelegate(pParent), m_pCommonData(pCommonData)
{
}


/*override*/ void CurrentAlbumDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    pPainter->save();

    const Mp3Handler* p (m_pCommonData->getCrtAlbum().at(index.row()));
    const Id3V2StreamBase* pId3V2 (p->getId3V2Stream());

    if (0 == pId3V2)
    {
        pPainter->fillRect(option.rect, QBrush(ERROR_COLOR()));
    }

    QItemDelegate::paint(pPainter, option, index);

    pPainter->restore();
}



//======================================================================================================================
//======================================================================================================================
//======================================================================================================================



FileRenamerDlgImpl::FileRenamerDlgImpl(QWidget* pParent, CommonData* pCommonData) : QDialog(pParent, 0), Ui::FileRenamerDlg(), m_pCommonData(pCommonData)
{
    setupUi(this);

    resizeIcons();

    m_pCurrentAlbumModel = new CurrentAlbumModel(m_pCommonData, this);

    {
        m_pCurrentAlbumG->verticalHeader()->setResizeMode(QHeaderView::Interactive);
        m_pCurrentAlbumG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT + 1);
        m_pCurrentAlbumG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT + 1);//*/

        m_pCurrentAlbumG->setModel(m_pCurrentAlbumModel);

        CurrentAlbumDelegate* pDel (new CurrentAlbumDelegate(this, m_pCommonData));
        m_pCurrentAlbumG->setItemDelegate(pDel);
    }

    m_pButtonGroup = new QButtonGroup(this);

    loadPatterns();
    updateButtons();

    {
        int nWidth, nHeight;
        bool bKeepOriginal;
        m_pCommonData->m_settings.loadRenamerSettings(nWidth, nHeight, m_nSaButton, m_nVaButton, bKeepOriginal);
        if (nWidth > 400 && nHeight > 400)
        {
            resize(nWidth, nHeight);
        }
        else
        {
            defaultResize(*this);
        }

        if (m_nVaButton >= cSize(m_vstrPatterns) || m_nVaButton <= 0) { m_nVaButton = 0; }
        if (m_nSaButton >= cSize(m_vstrPatterns) || m_nSaButton <= 0) { m_nSaButton = 0; }
        m_pKeepOriginalCkB->setChecked(bKeepOriginal);
    }


    { m_pModifRenameB = new ModifInfoToolButton(m_pRenameB); connect(m_pModifRenameB, SIGNAL(clicked()), this, SLOT(on_m_pRenameB_clicked())); m_pRenameB = m_pModifRenameB; }

    QTimer::singleShot(1, this, SLOT(onShow())); // just calls reloadTable(); !!! needed to properly resize the table columns; album and file tables have very small widths until they are actually shown, so calling resizeTagEditor() earlier is pointless; calling update() on various layouts seems pointless as well; (see also DoubleList::resizeEvent() )
}


FileRenamerDlgImpl::~FileRenamerDlgImpl()
{
    m_pCommonData->m_settings.saveRenamerSettings(width(), height(), m_nSaButton, m_nVaButton, m_pKeepOriginalCkB->isChecked());
}


string FileRenamerDlgImpl::run()
{
    exec();
    int k (m_pCurrentAlbumG->currentIndex().row());
    const deque<const Mp3Handler*>& vpCrtAlbum (m_pCommonData->getCrtAlbum());
    if (k < 0 || k >= cSize(vpCrtAlbum)) { return ""; }
    return vpCrtAlbum[k]->getName();
}



void FileRenamerDlgImpl::on_m_pNextB_clicked()
{
    if (m_pCommonData->nextAlbum())
    {
        reloadTable();
    }
}

void FileRenamerDlgImpl::on_m_pPrevB_clicked()
{
    if (m_pCommonData->prevAlbum())
    {
        reloadTable();
    }
}


void FileRenamerDlgImpl::on_m_pEditPatternsB_clicked()
{
    RenamerPatternsDlgImpl dlg (this, m_pCommonData->m_settings);
    if (dlg.run(m_vstrPatterns))
    {
        m_nVaButton = m_nSaButton = 0;
        savePatterns();
        updateButtons();
        selectPattern();
    }
}


void FileRenamerDlgImpl::updateButtons()
{
    m_nBtnId = 0;
    //createButtons(m_pSaButtonsW);
    //createButtons(m_pVaButtonsW);
    createButtons(m_pButtonsW);
}


void FileRenamerDlgImpl::createButtons(QWidget* pWidget)
{
    QBoxLayout* pLayout (dynamic_cast<QBoxLayout*>(pWidget->layout()));
    CB_ASSERT (0 != pLayout);
    /*int nPos (pLayout->indexOf(pOldBtn));
    pLayout->insertWidget(nPos, this);*/

    QObjectList l (pWidget->children());
    //qDebug("cnt: %d", l.size());

    for (int i = 1, n = l.size(); i < n; ++i)
    {
        delete l[i];
    }

    for (int i = 0, n = cSize(m_vstrPatterns); i < n; ++i)
    {
        QToolButton* p (new QToolButton(pWidget));
        p->setText(convStr(m_vstrPatterns[i]));
        p->setCheckable(true);
        m_pButtonGroup->addButton(p, m_nBtnId++);
        //p->setAutoExclusive(true);
        connect(p, SIGNAL(clicked()), this, SLOT(onPatternClicked()));
        pLayout->insertWidget(i, p);
    }
}


void FileRenamerDlgImpl::onPatternClicked()
{
    int nId (m_pButtonGroup->checkedId());
    int n (cSize(m_vstrPatterns));
    //qDebug("id=%d", nId);
    //CB_ASSERT (nId >= 0 && nId < 2*n);
    CB_ASSERT (nId >= 0 && nId < n);
    if (nId >= n) { nId -= n; }

    if (isSingleArtist())
    {
        m_nSaButton = nId;
    }
    else
    {
        m_nVaButton = nId;
    }
    m_pCurrentAlbumModel->setRenamer(new Renamer(m_vstrPatterns[nId]));
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================



namespace {

struct RenameThread : public PausableThread
{
    const deque<const Mp3Handler*>& m_vpHndl;
    bool m_bKeepOrig;
    const Renamer* m_pRenamer;
    QWidget* m_pParent;
    CommonData* m_pCommonData;
    vector<const Mp3Handler*>& m_vpDel;
    vector<const Mp3Handler*>& m_vpAdd;

    RenameThread(const deque<const Mp3Handler*>& vpHndl, bool bKeepOrig, const Renamer* pRenamer, QWidget* pParent, CommonData* pCommonData, vector<const Mp3Handler*>& vpDel, vector<const Mp3Handler*>& vpAdd) :
            m_vpHndl(vpHndl),
            m_bKeepOrig(bKeepOrig),
            m_pRenamer(pRenamer),
            m_pParent(pParent),
            m_pCommonData(pCommonData),
            m_vpDel(vpDel),
            m_vpAdd(vpAdd)
    {
    }

    /*override*/ void run()
    {
        CompleteNotif notif(this);

        notif.setSuccess(proc());
    }

    bool proc();
};


bool RenameThread::proc()
{
    QString qstrErr;

    for (int i = 0, n = cSize(m_vpHndl); i < n; ++i)
    {
        if (isAborted()) { return false; }
        checkPause();

        QString qstrName (convStr(m_vpHndl[i]->getName()));
        StrList l;
        l.push_back(qstrName);
        emit stepChanged(l);

        string strDest (m_pRenamer->getNewName(m_vpHndl[i]));
        try
        {
            if (m_bKeepOrig)
            {
                copyFile2(m_vpHndl[i]->getName(), strDest);
            }
            else
            {
                renameFile(m_vpHndl[i]->getName(), strDest);
                m_vpDel.push_back(m_vpHndl[i]);
            }

            if (m_pCommonData->m_dirTreeEnum.isIncluded(strDest))
            {
                try
                {
                    m_vpAdd.push_back(new Mp3Handler(strDest, m_pCommonData->m_bUseAllNotes, m_pCommonData->getQualThresholds()));
                }
                catch (const Mp3Handler::FileNotFound&)
                {
                }
            }
        }
        catch (const FoundDir&)
        {
            qstrErr = "Source or destination is a directory";
        }
        catch (const CannotCopyFile&)
        {
            qstrErr = "Error during copying";
        }
        catch (const CannotRenameFile&)
        {
            qstrErr = "Error during renaming";
        }
        catch (const AlreadyExists&)
        {
            qstrErr = "Destination already exists";
        }
        catch (const NameNotFound&)
        {
            qstrErr = "Source not found";
        }
        catch (...)
        {
            qstrErr = "Unknown error";
        }

        if (!qstrErr.isEmpty())
        {
            QMessageBox::critical(m_pParent, "Error", qstrErr);
            return false;
        }
    }

    return true;
}

} // namespace




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


void FileRenamerDlgImpl::on_m_pRenameB_clicked()
{
    if (m_vstrPatterns.empty())
    {
        QMessageBox::critical(this, "No patterns exist", "You must create at least a pattern before you can start renaming files.");
        return;
    }

    const Renamer* pRenamer (m_pCurrentAlbumModel->getRenamer());
    CB_ASSERT (0 != pRenamer);

    bool bAll (0 == (Qt::ShiftModifier & m_pModifRenameB->getModifiers()));
    bool bKeepOrig (m_pKeepOriginalCkB->isChecked());

    const deque<const Mp3Handler*>& vpAllHndl (m_pCommonData->getCrtAlbum());
    deque<const Mp3Handler*> vpHndl;
    if (bAll)
    {
        vpHndl = vpAllHndl;
    }
    else
    {
        QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());

        QModelIndexList listSel (pSelModel->selectedIndexes());
        set<int> snSongs;
        for (QModelIndexList::iterator it = listSel.begin(), end = listSel.end(); it != end; ++it)
        {
            QModelIndex ndx (*it);
            int nSong (ndx.row());
            snSongs.insert(nSong);
        }

        for (set<int>::iterator it = snSongs.begin(), end = snSongs.end(); it != end; ++it)
        {
            vpHndl.push_back(vpAllHndl[*it]);
        }
    }

    if (QMessageBox::Yes != QMessageBox::question(this, "Confirmation", QString(bKeepOrig ? "Copy" : "Rename") + (bAll ? " all the" : " selected") + " files?", QMessageBox::Yes | QMessageBox::Cancel))
    {
        return;
    }

    {
        set<string> s;
        for (int i = 0, n = cSize(vpHndl); i < n; ++i)
        {
            string strDest (pRenamer->getNewName(vpHndl[i]));
            if (s.count(strDest) > 0)
            {
                QMessageBox::critical(this, "Error", "Operation aborted because it would create 2 copies of a file called \"" + convStr(strDest) + "\"");
                return;
            }

            if (fileExists(strDest))
            {
                QMessageBox::critical(this, "Error", "Operation aborted because a file called \"" + convStr(strDest) + "\" already exists.");
                return;
            }

            s.insert(strDest);
        }
    }

    vector<const Mp3Handler*> vpDel, vpAdd;

    {
        RenameThread* pThread (new RenameThread(vpHndl, bKeepOrig, pRenamer, this, m_pCommonData, vpDel, vpAdd));

        ThreadRunnerDlgImpl dlg (this, 0, pThread, ThreadRunnerDlgImpl::SHOW_COUNTER, ThreadRunnerDlgImpl::TRUNCATE_BEGIN);
        dlg.setWindowTitle(QString(bKeepOrig ? "Copying" : "Renaming") + (bAll ? " all the" : " selected") + " files in the current album");
        dlg.exec();
    }

    m_pCommonData->mergeHandlerChanges(vpAdd, vpDel, CommonData::SEL | CommonData::CURRENT);

    if (!bKeepOrig)
    {
        reloadTable();
    }
}


void FileRenamerDlgImpl::reloadTable()
{
    {
        bool bVa (false);
        bool bErr (false); // it's error if any file lacks ID3V2
        const deque<const Mp3Handler*>& vpHndl (m_pCommonData->getCrtAlbum());
        if (vpHndl.empty())
        {
            accept();
            return;
        }

        int n (cSize(vpHndl));
        CB_ASSERT (n > 0);
        string strArtist ("\1");
        for (int i = 0; i < n; ++i)
        {
            const Mp3Handler* pHndl (vpHndl[i]);
            const Id3V2StreamBase* pId3V2 (pHndl->getId3V2Stream());
            if (0 == pId3V2)
            {
                bErr = true;
            }
            else
            {
                if ("\1" == strArtist)
                {
                    strArtist = pId3V2->getArtist();
                }
                else
                {
                    if (strArtist != pId3V2->getArtist())
                    {
                        bVa = true;
                    }
                }
            }
        }

        if (bErr)
        {
            m_eState = "\1" == strArtist ? ERR : bVa ? VARIOUS_ERR : SINGLE_ERR;
        }
        else
        {
            m_eState = bVa ? VARIOUS : SINGLE;
        }
    }

    selectPattern();

    m_pCurrentAlbumModel->emitLayoutChanged();
    const Mp3Handler* p (m_pCommonData->getCrtAlbum().at(0));
    m_pCrtDirTagEdtE->setText(convStr(p->getDir()));
    //m_pCrtDirTagEdtE->hide();
    //setWindowTitle("MP3 Diags - " + convStr(p->getDir()) + " - File renamer");

    QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());
    pSelModel->clear();
    pSelModel->setCurrentIndex(m_pCurrentAlbumModel->index(0, 0), QItemSelectionModel::SelectCurrent);

    resizeUi();
}

void FileRenamerDlgImpl::resizeUi()
{
    SimpleQTableViewWidthInterface intf (*m_pCurrentAlbumG);
    ColumnResizer rsz (intf, 100, ColumnResizer::FILL, ColumnResizer::CONSISTENT_RESULTS);
}

/*override*/ void FileRenamerDlgImpl::resizeEvent(QResizeEvent* /*pEvent*/)
{
    resizeUi();
}


// selects the appropriate pattern for a new album, based on whether it's VA or SA; sets m_nCrtPattern and checks the right button; assumes m_eState is properly set up;
void FileRenamerDlgImpl::selectPattern()
{
    if (!m_vstrPatterns.empty())
    {
        if (isSingleArtist())
        {
            m_pButtonGroup->button(m_nSaButton)->setChecked(true);
            m_pCurrentAlbumModel->setRenamer(new Renamer(m_vstrPatterns[m_nSaButton]));
        }
        else
        {
            m_pButtonGroup->button(m_nVaButton /*+ cSize(m_vstrPatterns)*/)->setChecked(true);
            m_pCurrentAlbumModel->setRenamer(new Renamer(m_vstrPatterns[m_nVaButton]));
        }
    }
    else
    {
        m_pCurrentAlbumModel->setRenamer(0);
    }

    resizeUi();

    m_pAlbumTypeL->setText(isSingleArtist() ? "Single artist" : "Various artists"); //ttt1 see if "single" is the best word
}




void FileRenamerDlgImpl::loadPatterns()
{ // pattern readers
    bool bErr (false);
    //vector<string> v (m_pCommonData->m_settings.loadRenamerPatterns(bErr));
    vector<string> v (m_pCommonData->m_settings.loadVector("fileRenamer/patterns", bErr));

    m_vstrPatterns.clear();
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        string strPatt (v[i]);
        try
        {
            Renamer r (strPatt);
            m_vstrPatterns.push_back(strPatt);
        }
        catch (const Renamer::InvalidPattern&)
        {
            bErr = true;
        }
    }

    /*if (m_vstrPatterns.empty()) // ttt1 because there is no default, the user is forced to add a pattern first; see if a meaningful default can be used
    { // use default (only if the user didn't remove all patterns on purpose)
        string s ("/tmp/%a/%b[ %y]/%r%n %t"); //ttt1 change from tmp to root/out; //ttt1 perhaps ask the user //ttt1 OS specific
        Renamer r (s); // may throw after porting, but that's OK, because it signals a fix is needed
        m_vstrPatterns.push_back(s);
    }*/

    if (bErr)
    {
        QMessageBox::warning(this, "Error setting up patterns", "An invalid value was found in the configuration file. You'll have to set up the patterns manually.");
    }
}


void FileRenamerDlgImpl::savePatterns()
{
    //m_pCommonData->m_settings.saveRenamerPatterns(m_vstrPatterns);
    m_pCommonData->m_settings.saveVector("fileRenamer/patterns", m_vstrPatterns);
}


//ttt1 perhaps have a "reload" button

void FileRenamerDlgImpl::resizeIcons()
{
    vector<QToolButton*> v;
    v.push_back(m_pPrevB);
    v.push_back(m_pNextB);
    v.push_back(m_pEditPatternsB);
    v.push_back(m_pRenameB);

    int k (m_pCommonData->m_nMainWndIconSize);
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        QToolButton* p (v[i]);
        p->setMaximumSize(k, k);
        p->setMinimumSize(k, k);
        p->setIconSize(QSize(k - 4, k - 4));
    }
}




//======================================================================================================================
//======================================================================================================================
//======================================================================================================================

namespace FileRenamer {

struct PatternBase
{
    virtual ~PatternBase() {}
    virtual string getVal(const Mp3Handler*) const = 0;
};

struct StaticPattern : public PatternBase
{
    StaticPattern(string strVal) : m_strVal(strVal) {}
    /*override*/ string getVal(const Mp3Handler*) const { return m_strVal; }
private:
    string m_strVal;
};


struct FieldPattern : public PatternBase
{
    FieldPattern(TagReader::Feature eFeature) : m_eFeature(eFeature) {}
    /*override*/ string getVal(const Mp3Handler* pHndl) const
    {
        const Id3V2StreamBase* p (pHndl->getId3V2Stream());
        if (0 == p) { return ""; }
        return p->getValue(m_eFeature);
    }
private:
    TagReader::Feature m_eFeature;
};

struct YearPattern : public PatternBase
{
    /*override*/ string getVal(const Mp3Handler* pHndl) const
    {
        const Id3V2StreamBase* p (pHndl->getId3V2Stream());
        if (0 == p) { return ""; }
        return p->getTime().getYear();
    }
};

struct TrackNoPattern : public PatternBase
{
    /*override*/ string getVal(const Mp3Handler* pHndl) const
    {
        const Id3V2StreamBase* p (pHndl->getId3V2Stream());
        if (0 == p) { return ""; }
        string s (p->getTrackNumber());
        int n (atoi(s.c_str()));
        if (n <= 0) { return ""; }
        char a [20];
        sprintf(a, "%02d", n);
        return a;
    }
};



struct RatingPattern : public PatternBase
{
    /*override*/ string getVal(const Mp3Handler* pHndl) const
    {
        const Id3V2StreamBase* p (pHndl->getId3V2Stream());
        if (0 == p) { return ""; }
        double r (p->getRating());
        if (r < 0) { return ""; }
// from TrackTextReader::TrackTextReader
//                                   a    b    c    d    e    f    g    h    i    j    k    l    m    n    o    p   q   r    s    t    u    v    w    x    y    z
//static double s_ratingMap [] = { 5.0, 4.5, 4.0, 3.5, 3.0, 2.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 1.5, 1.0, 1.0, 1.0, -1, -1, 1.0, 1.0, 1.0, 1.0, 1.0, 0.5, 0.5, 0.0 };
// note that if this changes it should be synchronized with TrackTextReader
        if (r >= 4.9) { return "a"; }
        if (r >= 4.4) { return "b"; }
        if (r >= 3.9) { return "c"; }
        if (r >= 3.4) { return "d"; }
        if (r >= 2.9) { return "e"; }
        if (r >= 2.4) { return "f"; }
        if (r >= 1.9) { return "g"; }
        if (r >= 1.4) { return "m"; }
        if (r >= 0.9) { return "n"; }
        if (r >= 0.4) { return "x"; }
        return "z";
    }
};

struct SequencePattern : public PatternBase
{
    /*override*/ ~SequencePattern() { clearPtrContainer(m_vpPatterns); }
    /*override*/ string getVal(const Mp3Handler* pHndl) const { return getVal(pHndl, ACCEPT_EMPTY); }
    string getNonNullVal(const Mp3Handler* pHndl) const { return getVal(pHndl, DONT_ACCEPT_EMPTY); } // returns an empty string if any of its components are empty
    void addPattern(const PatternBase* p) { m_vpPatterns.push_back(p); }
private:
    enum { DONT_ACCEPT_EMPTY, ACCEPT_EMPTY };
    vector<const PatternBase*> m_vpPatterns;
    string getVal(const Mp3Handler* pHndl, bool bAcceptEmpty) const
    {
        string strRes;
        for (int i = 0, n = cSize(m_vpPatterns); i < n; ++i)
        {
            string s (m_vpPatterns[i]->getVal(pHndl));
            if (s.empty() && !bAcceptEmpty) { return s; }
            strRes += s;
        }
        return strRes;
    }
};

struct OptionalPattern : public PatternBase
{
    /*override*/ ~OptionalPattern() { delete m_pSequencePattern; }
    OptionalPattern(SequencePattern* pSequencePattern) : m_pSequencePattern(pSequencePattern) {}
    /*override*/ string getVal(const Mp3Handler* pHndl) const { return m_pSequencePattern->getNonNullVal(pHndl); }
private:
    SequencePattern* m_pSequencePattern;
};

} // namespace FileRenamer

//using namespace RenamerPatterns;


Renamer::Renamer(const std::string& strPattern) : m_strPattern(strPattern), m_pRoot(new SequencePattern())
{
    auto_ptr<SequencePattern> ap (m_pRoot);
    const char* p (strPattern.c_str());
    SequencePattern* pSeq (m_pRoot); // this is either m_pRoot or a new sequence, for optional elements
    if (0 == *p) { throw InvalidPattern("A pattern cannot be empty"); }
    if (getPathSep() != *p) { throw InvalidPattern("A pattern must begin with '/'"); } // ttt1 Linux-specific

    auto_ptr<SequencePattern> optAp;

    string strStatic;
    bool bTitleFound (false);
    //const char* q (p);
    for (; *p != 0; ++p)
    {
        char c (*p);
        switch (c)
        {
        case '%':
            {
                ++p;
                char c1 (*p);
                switch (c1)
                {
                case 'n':
                case 'a':
                case 't':
                case 'b':
                case 'y':
                case 'g':
                case 'c':
                case 'r':
                    if (!strStatic.empty()) { pSeq->addPattern(new StaticPattern(strStatic)); strStatic.clear(); }
                }

                switch (c1)
                {
                case 'n': pSeq->addPattern(new TrackNoPattern()); break;
                case 'a': pSeq->addPattern(new FieldPattern(TagReader::ARTIST)); break;
                case 't': pSeq->addPattern(new FieldPattern(TagReader::TITLE)); bTitleFound = true; break;
                case 'b': pSeq->addPattern(new FieldPattern(TagReader::ALBUM)); break;
                case 'y': pSeq->addPattern(new YearPattern()); break;
                case 'g': pSeq->addPattern(new FieldPattern(TagReader::GENRE)); break;
                case 'c': pSeq->addPattern(new FieldPattern(TagReader::COMPOSER)); break;
                case 'r': pSeq->addPattern(new RatingPattern()); break;

                case '%':
                case '[':
                case ']':
                //case '/': // ttt linux-specific // actually there's no need for '/' to be a special character here
                    strStatic += c; break;

                default: throw InvalidPattern("Invalid pattern"); // ttt1 details
                }
            }
            break;

        case '[':
            {
                if (!strStatic.empty()) { pSeq->addPattern(new StaticPattern(strStatic)); strStatic.clear(); }
                if (pSeq != m_pRoot) { throw InvalidPattern("Nested optional elements are not allowed"); } //ttt2 column
                pSeq = new SequencePattern();
                optAp.reset(pSeq);
                break;
            }

        case ']':
            {
                if (!strStatic.empty()) { pSeq->addPattern(new StaticPattern(strStatic)); strStatic.clear(); }
                if (pSeq == m_pRoot) { throw InvalidPattern("Trying to close and optional element although none is open"); } //ttt2 column
                m_pRoot->addPattern(new OptionalPattern(pSeq));
                pSeq = m_pRoot;
                optAp.release();
                break;
            }

        default:
            strStatic += c;
        }
    }

    if (!strStatic.empty()) { pSeq->addPattern(new StaticPattern(strStatic)); strStatic.clear(); }

    if (pSeq != m_pRoot) { throw InvalidPattern("Optional element must be closed"); } //ttt2 column

    if (!bTitleFound) { throw InvalidPattern("Title entry (%t) must be present"); }

    ap.release();
}

Renamer::~Renamer()
{
    delete m_pRoot;
}


string Renamer::getNewName(const Mp3Handler* pHndl) const
{
    return m_pRoot->getVal(pHndl) + ".mp3";
}

//ttt1 a "pop/rock" genre creates unexpected folder
//ttt1 timer in normalizer
//ttt1 look at normalized loudness in tracks, maybe warn
