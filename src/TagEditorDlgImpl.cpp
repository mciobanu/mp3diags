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

#include  <QScrollArea>
#include  <QScrollBar>
#include  <QPainter>
#include  <QTimer>
#include  <QKeyEvent>
#include  <QHeaderView>

#include  "TagEditorDlgImpl.h"

#include  "Helpers.h"
#include  "TagWriter.h"
#include  "DataStream.h"
#include  "CommonData.h"
#include  "ConfigDlgImpl.h"
#include  "ColumnResizer.h"
#include  "TagEdtPatternsDlgImpl.h"
#include  "DiscogsDownloader.h"
#include  "MusicBrainzDownloader.h"
#include  "ImageInfoPanelWdgImpl.h"
#include  "PaletteDlgImpl.h"
#include  "Id3V230Stream.h"
#include  "Id3V240Stream.h"
#include  "MpegStream.h"
#include  "Widgets.h"
#include  "Transformation.h"
#include  "Mp3TransformThread.h"
#include  "StoredSettings.h"
#include  "Mp3Manip.h"
#include  "OsFile.h"
#include  "Version.h"


using namespace std;
using namespace pearl;

using namespace TagEditor;
using namespace Version;

//======================================================================================================================
//======================================================================================================================
//======================================================================================================================


CurrentAlbumModel::CurrentAlbumModel(TagEditorDlgImpl* pTagEditorDlgImpl) : m_pTagEditorDlgImpl(pTagEditorDlgImpl), m_pTagWriter(pTagEditorDlgImpl->getTagWriter()), m_pCommonData(pTagEditorDlgImpl->getCommonData())
{
}



/*override*/ int CurrentAlbumModel::rowCount(const QModelIndex&) const
{
//qDebug("rowCount ret %d", cSize(m_pTagWriter->m_vpMp3HandlerTagData));
    return cSize(m_pTagWriter->m_vpMp3HandlerTagData);
}


/*override*/ int CurrentAlbumModel::columnCount(const QModelIndex&) const
{
    return TagReader::LIST_END + 1;
}


/*override*/ QVariant CurrentAlbumModel::data(const QModelIndex& index, int nRole) const
{
LAST_STEP("CurrentAlbumModel::data()");
    if (!index.isValid()) { return QVariant(); }
    int i (index.row());
    int j (index.column());
//qDebug("ndx %d %d", i, j);

    if (Qt::DisplayRole != nRole && Qt::ToolTipRole != nRole && Qt::EditRole != nRole) { return QVariant(); }
    QString s;

    if (m_pTagEditorDlgImpl->isSaving()) { return "N/A"; }
    if (m_pTagEditorDlgImpl->isNavigating()) { return ""; }

    if (0 == j)
    {
        const Mp3Handler* p (m_pTagWriter->m_vpMp3HandlerTagData[i]->getMp3Handler());
        s = convStr(p->getShortName());
    }
    else
    {
        /*const Mp3HandlerTagData& d (*m_pTagWriter->m_vpMp3HandlerTagData[i]);
        s = convStr(d.getData(TagReader::FEATURE_ON_POS[j - 1]));*/
        s = convStr(m_pTagWriter->getData(i, TagReader::FEATURE_ON_POS[j - 1]));
    }

    if (Qt::DisplayRole == nRole || Qt::EditRole == nRole)
    {
        return s;
    }

    QFontMetrics fm (m_pTagEditorDlgImpl->m_pCurrentAlbumG->fontMetrics()); // !!! no need to use "QApplication::fontMetrics()"
    int nWidth (fm.width(s));

    int nMargin (QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1); // this "1" should probably be another "metric" constant
    if (nWidth + 2*nMargin + 1 <= m_pTagEditorDlgImpl->m_pCurrentAlbumG->horizontalHeader()->sectionSize(j)) // ttt2 not sure this "nMargin" is correct
    {
        //return QVariant();
        return ""; // !!! with "return QVariant()" the previous tooltip remains until the cursor moves over another cell that has a tooltip
    }

    return s;
}

//ttt2 perhaps: "right click on crt file header moves to last; left click moves to first"; dragging the columns seems ok, though

/*override*/ QVariant CurrentAlbumModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /*= Qt::DisplayRole*/) const
{
LAST_STEP("CurrentAlbumModel::headerData");
    if (nRole != Qt::DisplayRole) { return QVariant(); }

    if (Qt::Horizontal == eOrientation)
    {
        if (0 == nSection) { return "File name"; }
        return TagReader::getLabel(TagReader::FEATURE_ON_POS[nSection - 1]);
    }

    return nSection + 1;
}


/*override*/ Qt::ItemFlags CurrentAlbumModel::flags(const QModelIndex& /*index*/) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}



/*override*/ bool CurrentAlbumModel::setData(const QModelIndex& index, const QVariant& value, int nRole /*= Qt::EditRole*/)
{
    if (Qt::EditRole != nRole) { return false; }

    Mp3HandlerTagData* pData (m_pTagWriter->m_vpMp3HandlerTagData[index.row()]);
    try
    {
        pData->setData(TagReader::FEATURE_ON_POS[index.column() - 1], convStr(value.toString()));
        m_pTagEditorDlgImpl->updateAssigned();
        return true;
    }
    catch (const Mp3HandlerTagData::InvalidValue&)
    {
        QMessageBox::critical(m_pTagEditorDlgImpl, "Error", "The data contained errors and couldn't be saved"); //ttt2 put focus on album table
        return false; // ttt2 if it gets here the data is lost; perhaps CurrentAlbumDelegate should be modified more extensively, to not close the editor on Enter if this returns false;
    }
}


//======================================================================================================================
//======================================================================================================================


CurrentFileModel::CurrentFileModel(const TagEditorDlgImpl* pTagEditorDlgImpl) : m_pTagEditorDlgImpl(pTagEditorDlgImpl), m_pTagWriter(pTagEditorDlgImpl->getTagWriter()), m_pCommonData(pTagEditorDlgImpl->getCommonData())
{
}



/*override*/ int CurrentFileModel::rowCount(const QModelIndex&) const
{
    return TagReader::LIST_END;
}


/*override*/ int CurrentFileModel::columnCount(const QModelIndex&) const
{
    return cSize(m_pTagWriter->m_vTagReaderInfo);
}


/*override*/ QVariant CurrentFileModel::data(const QModelIndex& index, int nRole) const
{
LAST_STEP("CurrentFileModel::data()");
    if (!index.isValid() || 0 == m_pTagWriter->getCurrentHndl()) { return QVariant(); }
    int i (index.row());
    int j (index.column());

    if (Qt::DisplayRole != nRole && Qt::ToolTipRole != nRole) { return QVariant(); }

    if (m_pTagEditorDlgImpl->isSaving()) { return "N/A"; }
    if (m_pTagEditorDlgImpl->isNavigating()) { return ""; }

    if (0 == m_pTagWriter->getCurrentHndl()) { return QVariant(); } // may happen in transient states (prev/next album)

    const Mp3HandlerTagData& d (*m_pTagWriter->getCrtMp3HandlerTagData());
    string s1 (d.getData(TagReader::FEATURE_ON_POS[i], j));
    QString s (convStr(s1));

    if (Qt::DisplayRole == nRole)
    {
        return s;
    }

    QFontMetrics fm (m_pTagEditorDlgImpl->m_pCurrentFileG->fontMetrics()); // !!! no need to use "QApplication::fontMetrics()"
    int nWidth (fm.width(s));

    int nMargin (QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1); // this "1" should probably be another "metric" constant
    if (nWidth + 2*nMargin + 1 <= m_pTagEditorDlgImpl->m_pCurrentFileG->horizontalHeader()->sectionSize(j)) // ttt2 not sure this "nMargin" is correct
    {
        //return QVariant();
        return ""; // !!! with "return QVariant()" the previous tooltip remains until the cursor moves over another cell that has a tooltip
    }

    return s;

}



/*override*/ QVariant CurrentFileModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /*= Qt::DisplayRole*/) const
{
LAST_STEP("CurrentFileModel::headerData");
    //if (nRole == Qt::SizeHintRole) { return QSize(CELL_WIDTH - 1, CELL_HEIGHT); }

    if (nRole != Qt::DisplayRole) { return QVariant(); }

    if (Qt::Horizontal == eOrientation)
    {
        if (nSection >= cSize(m_pTagWriter->m_vTagReaderInfo)) { return QVariant(); }
        QString sSuff;

        const TagReaderInfo& inf (m_pTagWriter->m_vTagReaderInfo.at(nSection));

        if (
                0 != m_pTagWriter->getCrtMp3HandlerTagData() &&
                (
                    TrackTextReader::getClassDisplayName() == inf.m_strName ||
                    WebReader::getClassDisplayName() == inf.m_strName
                )
            )
        {
            const TagReader* p (m_pTagWriter->getCrtMp3HandlerTagData()->getMatchingReader(nSection));
            if (0 != p)
            {
                if (TrackTextReader::getClassDisplayName() == inf.m_strName)
                {
                    const TrackTextReader* pRd (dynamic_cast<const TrackTextReader*>(p));
                    CB_ASSERT (0 != pRd);
                    sSuff = QString(" ") + pRd->getType();
                }
                else if (WebReader::getClassDisplayName() == inf.m_strName)
                {
                    const WebReader* pRd (dynamic_cast<const WebReader*>(p));
                    CB_ASSERT (0 != pRd);
                    sSuff = " (" + convStr(pRd->getType()) + ")";
                }
            }
        }

        QString s;

        if (inf.m_bAlone)
        {
            s = convStr(inf.m_strName) + sSuff;
        }
        else
        {
            s = QString("%1 %2").arg(convStr(inf.m_strName)).arg(inf.m_nPos + 1) + sSuff;
        }
        return s;
    }

    return TagReader::getLabel(TagReader::FEATURE_ON_POS[nSection]);
}


//======================================================================================================================
//======================================================================================================================
//======================================================================================================================


TagEditorDlgImpl::TagEditorDlgImpl(QWidget* pParent, CommonData* pCommonData, TransfConfig& transfConfig, bool& bDataSaved) : QDialog(pParent, getDialogWndFlags()), Ui::TagEditorDlg(), m_pCommonData(pCommonData), m_bSectionMovedLock(false), m_transfConfig(transfConfig), m_bIsFastSaving(false), m_bIsSaving(false), m_bIsNavigating(false), m_bDataSaved(bDataSaved), m_bWaitingAlbumResize(false), m_bWaitingFileResize(false), m_eArtistsCase(TC_NONE), m_eOthersCase(TC_NONE)
{
    setupUi(this);

    m_bDataSaved = false;

    setupVarArtistsBtn();

    m_pAssgnBtnWrp = new AssgnBtnWrp (m_pToggleAssignedB);

    {
        m_pTagWriter = new TagWriter(m_pCommonData, this, m_bIsFastSaving, m_eArtistsCase, m_eOthersCase);
        loadTagWriterInf();
        connect(m_pTagWriter, SIGNAL(albumChanged()), this, SLOT(onAlbumChanged()));
        connect(m_pTagWriter, SIGNAL(fileChanged()), this, SLOT(onFileChanged()));
        connect(m_pTagWriter, SIGNAL(imagesChanged()), this, SLOT(onImagesChanged()));
        connect(m_pTagWriter, SIGNAL(requestSave()), this, SLOT(on_m_pSaveB_clicked()));
        connect(m_pTagWriter, SIGNAL(varArtistsUpdated(bool)), this, SLOT(onVarArtistsUpdated(bool)));
    }

    m_pCurrentAlbumModel = new CurrentAlbumModel(this);
    m_pCurrentFileModel = new CurrentFileModel(this);

    {
        m_pCurrentAlbumG->verticalHeader()->setResizeMode(QHeaderView::Interactive);
        m_pCurrentAlbumG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        m_pCurrentAlbumG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);//*/

        m_pCurrentAlbumG->setModel(m_pCurrentAlbumModel);

        m_pAlbumDel = new CurrentAlbumDelegate(m_pCurrentAlbumG, this);
        m_pCurrentAlbumG->setItemDelegate(m_pAlbumDel);


        //connect(m_pCurrentAlbumG, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onAlbSelChanged())); // ttt2 see if both this and next are needed (next seems enough)
        connect(m_pCurrentAlbumG->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onAlbSelChanged()));
        connect(m_pCurrentAlbumG->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex &)), this, SLOT(onAlbCrtChanged()));
    }


    {
        m_pCurrentFileG->setVerticalHeader(new NoCropHeaderView(m_pCurrentFileG));
        m_pCurrentFileG->verticalHeader()->setResizeMode(QHeaderView::Interactive);
        m_pCurrentFileG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        m_pCurrentFileG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);//*/

        m_pCurrentFileG->setModel(m_pCurrentFileModel);
        CurrentFileDelegate* pDel (new CurrentFileDelegate(m_pCurrentFileG, m_pCommonData));

        connect(m_pCurrentFileG->horizontalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(onFileSelSectionMoved(int, int, int)));

        m_pCurrentFileG->setItemDelegate(pDel);

        m_pCurrentFileG->horizontalHeader()->setMovable(true);
    }


    {
        int nWidth, nHeight;
        int nArtistsCase, nOthersCase;
        m_pCommonData->m_settings.loadTagEdtSettings(nWidth, nHeight, nArtistsCase, nOthersCase);
        m_eArtistsCase = TextCaseOptions(nArtistsCase);
        m_eOthersCase = TextCaseOptions(nOthersCase);
        if (nWidth > 400 && nHeight > 400)
        {
            resize(nWidth, nHeight);
        }
        else
        {
            defaultResize(*this);
        }
    }


    {
        delete m_pRemovable1L; // m_pRemovableL's only job is to make m_pImagesW visible in QtDesigner

        QWidget* pWidget (new QWidget (this));
        pWidget->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

        QHBoxLayout* pScrlLayout (new QHBoxLayout (pWidget));
        pWidget->setLayout(pScrlLayout);

        pWidget->layout()->setContentsMargins(0, 0, 0, 4 + 1); // "+1" added to look better, but don't know how to calculate

        m_pImgScrollArea = new QScrollArea (m_pImagesW);

        m_pImagesW->layout()->addWidget(m_pImgScrollArea);

        m_pImgScrollArea->setWidget(pWidget);
        m_pImgScrollArea->setFrameShape(QFrame::NoFrame);
    }

    {
        delete m_pRemovable2L; // m_pRemovable2L's only job is to make m_pPatternsW visible in QtDesigner
        createPatternButtons();
    }

    //layout()->update();
    /*layout()->update();
    widget_4->layout()->update();
    widget->layout()->update();
    layout()->update();
    widget_4->layout()->update();
    widget->layout()->update();*/

    m_pTagWriter->reloadAll("", TagWriter::CLEAR_DATA, TagWriter::CLEAR_ASSGN);
    selectMainCrt();

    resizeIcons();

    m_pCurrentAlbumG->installEventFilter(this);
    installEventFilter(this);

    {
        int nFileHght ((CELL_HEIGHT)*10 + m_pCurrentFileG->horizontalHeader()->height() + 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, m_pCurrentFileG));
        m_pCurrentFileG->setMaximumHeight(nFileHght);
        m_pCurrentFileG->setMinimumHeight(nFileHght);
    }

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }

    QTimer::singleShot(1, this, SLOT(onShow())); // just calls resizeTagEditor(); !!! needed to properly resize the table columns; album and file tables have very small widths until they are actually shown, so calling resizeTagEditor() earlier is pointless; calling update() on various layouts seems pointless as well; (see also DoubleList::resizeEvent() )
}


void TagEditorDlgImpl::createPatternButtons()
{
    QBoxLayout* pLayout (dynamic_cast<QBoxLayout*>(m_pPatternsW->layout()));
    CB_ASSERT (0 != pLayout);

    QObjectList l (m_pPatternsW->children());

    for (int i = 0, n = l.size(); i < n; ++i)
    {
        if (l[i] != pLayout)
        {
            delete l[i];
        }
    }

    const set<int>& snActivePatterns (m_pTagWriter->getActivePatterns());

    m_vpPattButtons.clear();

    const vector<string>& vstrPatterns (m_pTagWriter->getPatterns());
    int n (cSize(vstrPatterns));
    for (int i = 0; i < n; ++i)
    {
        QToolButton* p (new QToolButton(m_pPatternsW));
        p->setText(toNativeSeparators(convStr(vstrPatterns[i])));
        p->setCheckable(true);
        m_vpPattButtons.push_back(p);
        if (snActivePatterns.count(i) > 0)
        {
            p->setChecked(true);
        }
        connect(p, SIGNAL(clicked()), this, SLOT(onPatternClicked()));
        pLayout->insertWidget(i, p);
    }

    pLayout->insertStretch(n);

    m_pPatternsW->setVisible(!m_vpPattButtons.empty());
}


void TagEditorDlgImpl::onPatternClicked()
{
    set<int> s;
    for (int i = 0; i < cSize(m_vpPattButtons); ++i)
    {
        if (m_vpPattButtons[i]->isChecked())
        {
            s.insert(i);
        }
    }

    m_pTagWriter->setActivePatterns(s);
}


TagEditorDlgImpl::~TagEditorDlgImpl()
{
    saveTagWriterInf();

    m_pCommonData->m_settings.saveTagEdtSettings(width(), height(), int(m_eArtistsCase), int(m_eOthersCase));
    delete m_pCurrentAlbumModel;
    delete m_pCurrentFileModel;
    delete m_pTagWriter;
    delete m_pAssgnBtnWrp;
}


string TagEditorDlgImpl::run()
{
    TRACER("TagEditorDlgImpl::run()");
    exec();
    return m_pTagWriter->getCurrentName();
}


void TagEditorDlgImpl::setupVarArtistsBtn()
{
    m_pVarArtistsB->setEnabled(m_pCommonData->m_bItunesVarArtists || m_pCommonData->m_bWmpVarArtists);
    m_pVarArtistsB->setToolTip(m_pVarArtistsB->isEnabled() ?
            "Toggle \"Various Artists\"" :

            "To enable \"Various Artists\" you need to open the\n"
            "configuration dialog, go to the \"Others\" tab and\n"
            "check the corresponding checkbox(es)");
}



void TagEditorDlgImpl::resizeTagEditor()
{
    if (!m_bWaitingAlbumResize)
    {
        m_bWaitingAlbumResize = true;
        QTimer::singleShot(1, this, SLOT(onResizeTagEditorDelayed()));
    }
}


void TagEditorDlgImpl::onResizeTagEditorDelayed() // needed because it's pointless to call this while navigating, when the text for all cells is returned as ""
{
    m_bWaitingAlbumResize = false;
    QWidget* pParent (m_pCurrentAlbumG->parentWidget());

    //cout << "\n========================\n" << endl; listWidget(pParent);

    m_pImgScrollArea->widget()->adjustSize();
    //m_pImgScrollArea->adjustSize();
    int nHeight (m_pImgScrollArea->widget()->height() /*+ 10*/);
    if (m_pImgScrollArea->widget()->width() > m_pImgScrollArea->width())
    {
        int nScrollBarWidth (QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent));
        /*if (0 != QApplication::style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents)) // see if this is needed for Oxygen
        {
            nRes -= 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, &m_tbl); //ttt2 Qt 4.4 (and below) - specific; in 4.5 there's a QStyle::PM_ScrollView_ScrollBarSpacing
        }*/

        nHeight += nScrollBarWidth;
    }
    m_pImagesW->setMaximumSize(100000, nHeight); // ttt2 see about adding space for horizontal scrollbar


    pParent->layout()->activate(); // resize m_pCurrentAlbumG based on the image list being visible or not
    m_pCurrentAlbumModel->emitLayoutChanged(); // !!! force recalculation of the vertical scrollbar's visibility

//m_pCurrentAlbumG->horizontalHeader()->hideSection(1);
    SimpleQTableViewWidthInterface intf1 (*m_pCurrentAlbumG);
/*intf1.setMinWidth(2, 600);
intf1.setMinWidth(0, 50);
intf1.setMinWidth(1, 50);*/


/*intf1.setFixedWidth(0, 50);
intf1.setFixedWidth(1, 50);
intf1.setFixedWidth(2, 50);
intf1.setFixedWidth(3, 50);
intf1.setFixedWidth(4, 50);
intf1.setFixedWidth(5, 50);*/
    ColumnResizer rsz1 (intf1, 100, ColumnResizer::FILL, ColumnResizer::CONSISTENT_RESULTS);
    resizeFile();
}


void TagEditorDlgImpl::resizeFile() // resizes the "current file" grid; called by resizeTagEditor() and by onFileChanged();
{
    if (!m_bWaitingFileResize)
    {
        m_bWaitingFileResize = true;
        QTimer::singleShot(1, this, SLOT(onResizeFileDelayed()));
    }
}

void TagEditorDlgImpl::onResizeFileDelayed() // resizes the "current file" grid; called by resizeTagEditor() and by onFileChanged();
{
    m_bWaitingFileResize = false;
    SimpleQTableViewWidthInterface intf2 (*m_pCurrentFileG);
    ColumnResizer rsz2 (intf2, 100, ColumnResizer::DONT_FILL, ColumnResizer::CONSISTENT_RESULTS);
}


/*override*/ void TagEditorDlgImpl::resizeEvent(QResizeEvent* pEvent)
{
    resizeTagEditor();
    QDialog::resizeEvent(pEvent);
}


void TagEditorDlgImpl::on_m_pQueryDiscogsB_clicked()
{
    if (!closeEditor()) { return; }

/*QPixmap pic ("/r/temp/1/tmp2/c pic/b/A/cg.jpg");
ImageInfo img (ImageInfo::OK, pic);
m_pCommonData->m_imageColl.addImage(img);
onTagWriterChanged();
//m_pCurrentAlbumG->adjustSize();
m_pTagWriter->resizeCols();
return;//*/

    AlbumInfo* pAlbumInfo;
    ImageInfo* pImageInfo;

    bool bOk;

    {
        DiscogsDownloader dlg (this, m_pCommonData->m_settings, m_pCommonData->m_bSaveDownloadedData);

        //dlg.exec();
        //dlg.getInfo("Beatles", "Help");
        //dlg.getInfo("Roxette", "Tourism");
        //if (dlg.getInfo("Kaplansky", "", 12, pAlbumInfo, pImageInfo))
        string strArtist, strAlbum;
        m_pTagWriter->getAlbumInfo(strArtist, strAlbum);
        bOk = dlg.getInfo(strArtist, strAlbum, cSize(m_pTagWriter->m_vpMp3HandlerTagData), pAlbumInfo, pImageInfo); // !!! it's important that the dialog is in an enclosed scope; otherwise column resizing doesn't work well or m_pCurrentAlbumG gets shrinked to a really small size
    }


    if (bOk)
    {
        int nImgPos (-1);

        if (0 != pImageInfo)
        {
            nImgPos = m_pTagWriter->addImage(*pImageInfo, 0 == pAlbumInfo ? TagWriter::CONSIDER_UNASSIGNED : TagWriter::CONSIDER_ASSIGNED);

            delete pImageInfo;
        }

        if (0 != pAlbumInfo)
        {
            if (-1 != nImgPos)
            {
                pAlbumInfo->m_imageInfo = m_pTagWriter->getImageColl()[nImgPos].m_imageInfo;
            }
            pAlbumInfo->m_strSourceName = "Discogs";
            m_pTagWriter->addAlbumInfo(*pAlbumInfo);
            delete pAlbumInfo;
        }

        //onTagWriterChanged(); // also calls resizeTagEditor();
    }
}



void TagEditorDlgImpl::on_m_pQueryMusicBrainzB_clicked()
{
    if (!closeEditor()) { return; }

    AlbumInfo* pAlbumInfo;
    ImageInfo* pImageInfo;

    bool bOk;

    {
        MusicBrainzDownloader dlg (this, m_pCommonData->m_settings, m_pCommonData->m_bSaveDownloadedData);

        //dlg.exec();
        //dlg.getInfo("Beatles", "Help");
        //dlg.getInfo("Roxette", "Tourism");
        //if (dlg.getInfo("Kaplansky", "", 12, pAlbumInfo, pImageInfo))
        string strArtist, strAlbum;
        m_pTagWriter->getAlbumInfo(strArtist, strAlbum);
        bOk = dlg.getInfo(strArtist, strAlbum, cSize(m_pTagWriter->m_vpMp3HandlerTagData), pAlbumInfo, pImageInfo); // !!! it's important that the dialog is in an enclosed scope; otherwise column resizing doesn't work well or m_pCurrentAlbumG gets shrinked to a really small size
    }


    if (bOk)
    {
        int nImgPos (-1);

        if (0 != pImageInfo)
        {
            nImgPos = m_pTagWriter->addImage(*pImageInfo, 0 == pAlbumInfo ? TagWriter::CONSIDER_UNASSIGNED : TagWriter::CONSIDER_ASSIGNED);

            //resizeTagEditor();
            delete pImageInfo;
        }

        if (0 != pAlbumInfo)
        {
            if (-1 != nImgPos)
            {
                pAlbumInfo->m_imageInfo = m_pTagWriter->getImageColl()[nImgPos].m_imageInfo;
            }
            pAlbumInfo->m_strSourceName = "MusicBrainz";
            m_pTagWriter->addAlbumInfo(*pAlbumInfo);
            delete pAlbumInfo;
        }

        //onTagWriterChanged(); // also calls resizeTagEditor();
    }
}





void TagEditorDlgImpl::on_m_pEditPatternsB_clicked()
{
    if (!closeEditor()) { return; }

    vector<pair<string, int> > v;

    vector<string> v1 (m_pTagWriter->getPatterns());
    for (int i = 0, n = cSize(v1); i < n; ++i)
    {
        v.push_back(make_pair(v1[i], -1));
    }

    vector<string> u;
    //u.push_back("%a/%b[[ ](%y)]/[[%r]%n][ ][-[ ]]%t");
    /*u.push_back("%a/%b[[ ](%y)]/[[%r]%n][.][ ][-[ ]]%t"); //ttt OS specific
    u.push_back("%b[[ ](%y)]/[[%r]%n][.][ ]%a[ ]-[ ]%t");
    u.push_back("%a - %b/[[%r]%n][.][ ][-[ ]]%t");
    u.push_back("[%n][.][ ][-[ ]]%t");*/

    u.push_back("%a/%b[(%y)]/[[%r]%n][.][ ][-]%t");
    u.push_back("%b[(%y)]/[[%r]%n][.]%a-%t");
    u.push_back("%a-%b/[[%r]%n][.][ ][-]%t");
    u.push_back("/%n[.]%a-%t");
    u.push_back("[%n][.][ ][-]%t");

    TagEdtPatternsDlgImpl dlg (this, m_pCommonData->m_settings, u);
    if (dlg.run(v))
    {
        m_pTagWriter->updatePatterns(v);
        createPatternButtons();
    }
}


void TagEditorDlgImpl::on_m_pPaletteB_clicked()
{
    if (!closeEditor()) { return; }

    PaletteDlgImpl dlg (m_pCommonData, this);
    dlg.exec();

    m_pCommonData->m_settings.saveMiscConfigSettings(m_pCommonData);
}


// adds new ImageInfoPanelWdgImpl instances, connects assign button and calls resizeTagEditor()
void TagEditorDlgImpl::onImagesChanged()
{
    QLayout* pLayout (m_pImgScrollArea->widget()->layout());
    const ImageColl& imgColl (m_pTagWriter->getImageColl());
    int nVecCnt (imgColl.size());

    int nPanelCnt (pLayout->count());
    CB_ASSERT (nVecCnt >= nPanelCnt);

    for (int i = nPanelCnt; i < nVecCnt; ++i)
    {
        ImageInfoPanelWdgImpl* p (new ImageInfoPanelWdgImpl(this, imgColl[i], i));
        pLayout->addWidget(p);
        p->show();
        connect(p, SIGNAL(assignImage(int)), m_pTagWriter, SLOT(onAssignImage(int)));
        connect(p, SIGNAL(eraseFile(int)), m_pTagWriter, SLOT(onEraseFile(int)));
        m_pTagWriter->addImgWidget(p);
    }

    resizeTagEditor();
}



void TagEditorDlgImpl::onVarArtistsUpdated(bool bVarArtists)
{
    static QPixmap picSa (":/images/va_sa.svg");
    static QPixmap picVa (":/images/va_va.svg");

    m_pVarArtistsB->setIcon(bVarArtists ? picVa : picSa);
}


void TagEditorDlgImpl::onAlbumChanged(/*bool bContentOnly*/)
{
    if (m_pTagWriter->m_vpMp3HandlerTagData.empty())
    { //  there were some files, the transform settings caused the files to be removed when saving; close the dialog;
        reject();
        return;
    }

    resizeTagEditor();
    m_pCurrentAlbumModel->emitLayoutChanged();

    const Mp3Handler* p (m_pTagWriter->m_vpMp3HandlerTagData.at(0)->getMp3Handler());
    QString qs (toNativeSeparators(convStr(p->getDir())));
#ifndef WIN32
#else
    if (2 == qs.size() && ':' == qs[1])
    {
        qs += "\\";
    }
#endif
    m_pCrtDirTagEdtE->setText(qs);
}


void TagEditorDlgImpl::on_m_pConfigB_clicked()
{
    if (!closeEditor()) { return; }

    SaveOpt eSaveOpt (save(IMPLICIT)); if (SAVED != eSaveOpt && DISCARDED != eSaveOpt) { return; }

    ConfigDlgImpl dlg (m_transfConfig, m_pCommonData, this, ConfigDlgImpl::SOME_TABS);

    if (dlg.run())
    {
        m_pCommonData->m_settings.saveMiscConfigSettings(m_pCommonData);
        m_pCommonData->m_settings.saveTransfConfig(m_transfConfig); // transformation
        resizeIcons();
        resizeTagEditor();
    }

    m_pTagWriter->reloadAll("", TagWriter::CLEAR_DATA, TagWriter::CLEAR_ASSGN); // !!! regardless of the conf being cancelled, so it's consistent with the question asked at the beginning
    setupVarArtistsBtn();
}



void TagEditorDlgImpl::onFileChanged()
{
    m_pCurrentFileModel->emitLayoutChanged();

    const Mp3HandlerTagData* p (m_pTagWriter->getCrtMp3HandlerTagData());
    if (0 != p)
    {
        int nPic (p->getImage());
        m_pTagWriter->selectImg(nPic);
    }

    //resizeTagEditor();
    resizeFile();
}



//========================================================================================================================================================
//========================================================================================================================================================
//========================================================================================================================================================



void TagEditorDlgImpl::loadTagWriterInf()
{
    { // tags
        m_pTagWriter->clearShowedNonSeqWarn();

        bool bErr1 (false), bErr2 (false);
        vector<string> vstrNames (m_pCommonData->m_settings.loadVector("tagWriter/tagNames", bErr1));
        vector<string> vstrPositions (m_pCommonData->m_settings.loadVector("tagWriter/tagPositions", bErr2));

        vector<TagReaderInfo> v;

        int n (min(cSize(vstrNames), cSize(vstrPositions)));

        for (int i = 0; i < n; ++i)
        {
            string strName (vstrNames[i]);
            int nPos (atoi(vstrPositions[i].c_str()));
            if (strName.empty() || nPos < 0) // ttt2 the test for nPos can be improved; we know that a value was found in the file, but it may even be non-numeric
            {
                bErr1 = true;
                break;
            }
            // ttt2 perhaps fix duplicate or missing positions, but it's no big deal; TagWriter takes care of duplicates itself and if something is missing it will get added back when needed
            v.push_back(TagReaderInfo(strName, nPos, TagReaderInfo::ONE_OF_MANY));
        }

        if (bErr1 || bErr2)
        {
            QMessageBox::warning(this, "Error setting up the tag order", "An invalid value was found in the configuration file. You'll have to sort the tags again.");
        }

        if (v.empty())
        { // use default
            v.push_back(TagReaderInfo(Id3V230Stream::getClassDisplayName(), 0, TagReaderInfo::ONE_OF_MANY));
            v.push_back(TagReaderInfo(Id3V230Stream::getClassDisplayName(), 1, TagReaderInfo::ONE_OF_MANY));
            v.push_back(TagReaderInfo(Id3V240Stream::getClassDisplayName(), 0, TagReaderInfo::ONE_OF_MANY));
            v.push_back(TagReaderInfo(Id3V240Stream::getClassDisplayName(), 1, TagReaderInfo::ONE_OF_MANY));
            v.push_back(TagReaderInfo(Id3V1Stream::getClassDisplayName(), 0, TagReaderInfo::ONE_OF_MANY));
            v.push_back(TagReaderInfo(Id3V1Stream::getClassDisplayName(), 1, TagReaderInfo::ONE_OF_MANY));
        }

        m_pTagWriter->addKnownInf(v);
    }

    { // pattern readers
        bool bErr (false);
        vector<string> vstrPatterns (m_pCommonData->m_settings.loadVector("tagWriter/patterns", bErr));
//if (bErr) { qDebug("error at loadVector(\"tagWriter/patterns\")"); } //ttt remove
        int n (cSize(vstrPatterns));

        vector<pair<string, int> > v;

        {
            for (int i = 0; i < n; ++i)
            {
                string strPatt (vstrPatterns[i]);
                if (strPatt.empty())
                {
                    bErr = true;
                }
                v.push_back(make_pair(strPatt, -1));
            }
        }

        /*if (v.empty())
        { // use default (only if the user didn't remove all patterns on purpose)
            //v.push_back(make_pair(string("%a/%b[[ ](%y)]/[[%r]%n][ ][-[ ]]%t"), -1));

            v.push_back(make_pair(string("%a/%b[[ ](%y)]/[[%r]%n][.][ ][-[ ]]%t"), -1)); //ttt OS specific
            v.push_back(make_pair(string("%b[[ ](%y)]/[[%r]%n][.][ ]%a[ ]-[ ]%t"), -1));
            v.push_back(make_pair(string("%a - %b/[[%r]%n][.][ ][-[ ]]%t"), -1));
            v.push_back(make_pair(string("[%n][.][ ][-[ ]]%t"), -1));
        }
        else if (1 == cSize(v) && "*" == v[0].first)
        {
            v.clear();
        }*/

        bErr = !m_pTagWriter->updatePatterns(v) || bErr;

        bool bErr2;
        vector<string> vstrActivePatterns (m_pCommonData->m_settings.loadVector("tagWriter/activePatterns", bErr2));
        set<int> snActivePatterns;
        for (int i = 0; i < cSize(vstrActivePatterns); ++i)
        {
            snActivePatterns.insert(atoi(vstrActivePatterns[i].c_str()));
        }
        m_pTagWriter->setActivePatterns(snActivePatterns);

//if (bErr) { qDebug("error when reading patterns"); } //ttt remove
        if (bErr || bErr2)
        {
            QMessageBox::warning(this, "Error setting up patterns", "An invalid value was found in the configuration file. You'll have to set up the patterns manually.");
        }
    }
}




void TagEditorDlgImpl::saveTagWriterInf()
{
    {
        vector<string> vstrNames, vstrPositions;
        const vector<TagReaderInfo>& v (m_pTagWriter->getSortedKnownTagReaders());
        char a [15];
        for (int i = 0, n = cSize(v); i < n; ++i)
        {
            vstrNames.push_back(v[i].m_strName);
            sprintf(a, "%d", v[i].m_nPos);
            vstrPositions.push_back(a);
        }

        m_pCommonData->m_settings.saveVector("tagWriter/tagNames", vstrNames);
        m_pCommonData->m_settings.saveVector("tagWriter/tagPositions", vstrPositions);
    }


    {
        vector<string> v (m_pTagWriter->getPatterns());
        /*if (v.empty())
        {
            v.push_back("*");
        }*/
        m_pCommonData->m_settings.saveVector("tagWriter/patterns", v);

        vector<string> u;
        char a [15];
        const set<int>& snActivePatterns (m_pTagWriter->getActivePatterns());
        for (set<int>::const_iterator it = snActivePatterns.begin(); it != snActivePatterns.end(); ++it)
        {
            sprintf(a, "%d", *it);
            u.push_back(a);
        }
        m_pCommonData->m_settings.saveVector("tagWriter/activePatterns", u);
    }
}



void TagEditorDlgImpl::on_m_pToggleAssignedB_clicked()
{
    if (!closeEditor()) { return; }

    m_pAssgnBtnWrp->setState(m_pTagWriter->toggleAssigned(m_pAssgnBtnWrp->getState()));
}


void TagEditorDlgImpl::on_m_pReloadB_clicked()
{
    if (!closeEditor()) { return; }

    bool bHasUnsavedAssgn (false);
    for (int i = 0, n = cSize(m_pTagWriter->m_vpMp3HandlerTagData); i < n; ++i)
    {
        bool bAssgn (false);
        bool bNonId3V2 (false);
        m_pTagWriter->hasUnsaved(i, bAssgn, bNonId3V2);
        bHasUnsavedAssgn = bHasUnsavedAssgn || bAssgn;
    }

    if (bHasUnsavedAssgn)
    {
        int k (showMessage(this, QMessageBox::Warning, 1, 1, "Warning", "Reloading the current album causes all unsaved changes to be lost. Really reload?", "Reload", "Cancel"));
        if (0 != k) { return; }
    }
    m_pTagWriter->reloadAll(m_pTagWriter->getCurrentName(), TagWriter::CLEAR_DATA, TagWriter::CLEAR_ASSGN);
    updateAssigned();
}


//ttt2 perhaps transf to set recording time based on file date; another to set file date based on rec time

// copies the values from the first row to the other rows for columns that have at least a cell selected (doesn't matter if more than 1 cells are selected);
void TagEditorDlgImpl::on_m_pCopyFirstB_clicked()
{
    if (!closeEditor()) { return; }

    m_pTagWriter->copyFirst();
}


void TagEditorDlgImpl::on_m_pVarArtistsB_clicked()
{
    if (!closeEditor()) { return; }

    m_pTagWriter->toggleVarArtists();
}


void TagEditorDlgImpl::on_m_pCaseB_clicked()
{
    QMenu menu;
    vector<QAction*> vpAct;

    QAction* pAct;
    for (int i = TC_NONE; i <= TC_SENTENCE; ++i)
    {
        pAct = new QAction(QString("Artists - ") + getCaseAsStr(TextCaseOptions(i)), &menu); menu.addAction(pAct); vpAct.push_back(pAct);
    }
    menu.addSeparator();
    for (int i = TC_NONE; i <= TC_SENTENCE; ++i)
    {
        pAct = new QAction(QString("Others - ") + getCaseAsStr(TextCaseOptions(i)), &menu); menu.addAction(pAct); vpAct.push_back(pAct);
    }

    for (int i = 0; i < cSize(vpAct); ++i) { vpAct[i]->setCheckable(true); }

    CB_ASSERT(m_eArtistsCase >= TC_NONE && m_eArtistsCase <= TC_SENTENCE);
    CB_ASSERT(m_eOthersCase >= TC_NONE && m_eOthersCase <= TC_SENTENCE);

    vpAct[m_eArtistsCase - TC_NONE]->setChecked(true);
    vpAct[m_eOthersCase - TC_NONE + 5]->setChecked(true);


    QAction* p (menu.exec(m_pCaseB->mapToGlobal(QPoint(0, m_pCaseB->height()))));
    if (0 != p)
    {
        int nIndex (std::find(vpAct.begin(), vpAct.end(), p) - vpAct.begin());
        if (nIndex < 5)
        {
            m_eArtistsCase = TextCaseOptions(nIndex - 1);
        }
        else
        {
            m_eOthersCase = TextCaseOptions(nIndex - 1 - 5);
        }

        m_pTagWriter->reloadAll(m_pTagWriter->getCurrentName(), TagWriter::DONT_CLEAR_DATA, TagWriter::DONT_CLEAR_ASSGN);
    }
}



void TagEditorDlgImpl::on_m_pSaveB_clicked() //ttt2 perhaps make this save selected list, by using SHIFT
{
    if (!closeEditor()) { return; }

    save(EXPLICIT);
    //SaveOpt eRes save(true);
    /*if (SAVED == save(true))
    {
        //m_pTagWriter->clearOrigVal();
    }*/
}


void TagEditorDlgImpl::on_m_pPasteB_clicked()
{
    if (!closeEditor()) { return; }

    m_pTagWriter->paste();
}



void TagEditorDlgImpl::on_m_pSortB_clicked()
{
    if (!closeEditor()) { return; }

    clearSelection();
    m_pTagWriter->sort();
    //m_pCurrentAlbumModel->emitLayoutChanged();
}



void TagEditorDlgImpl::onAlbSelChanged()
{
    updateAssigned();

    /*QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());
    int n (pSelModel->currentIndex().row());
qDebug("onAlbSelChanged - CurrentFile=%d", n);
    m_pTagWriter->setCrt(n);*/ // !!! incorrect, because currentIndex() returns the previous index; that's why onAlbCrtChanged() is needed
}

void TagEditorDlgImpl::onAlbCrtChanged()
{
    QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());
    int n (pSelModel->currentIndex().row());
    if (-1 == n) { return; }
    m_pTagWriter->setCrt(n);
}



void TagEditorDlgImpl::onFileSelSectionMoved(int /*nLogicalIndex*/, int nOldVisualIndex, int nNewVisualIndex)
{
    NonblockingGuard sectionMovedGuard (m_bSectionMovedLock);

    if (!sectionMovedGuard)
    {
        return;
    }


    QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());
    QModelIndex selNdx (pSelModel->currentIndex());
    //QModelIndex topNdx (m_pCurrentAlbumG->indexAt(QPoint(0, 0)));
    int nHrzPos (m_pCurrentAlbumG->horizontalScrollBar()->value());
    int nVertPos (m_pCurrentAlbumG->verticalScrollBar()->value());

    m_pCurrentFileG->horizontalHeader()->moveSection(nNewVisualIndex, nOldVisualIndex);
    m_pTagWriter->moveReader(nOldVisualIndex, nNewVisualIndex);

    pSelModel->setCurrentIndex(selNdx, QItemSelectionModel::SelectCurrent);
    m_pCurrentAlbumG->horizontalScrollBar()->setValue(nHrzPos);
    m_pCurrentAlbumG->verticalScrollBar()->setValue(nVertPos);
//ttt2 perhaps detect when files are removed/changed; while most tags in Id3V2 are stored with the handler, the longer ones (e.g. pictures, but others might be affected too) are dumped from memory by Id3V230Frame's constructor and retrieved from disk when needed; the user should be notified when something becomes unavailable; not sure where to do this, though: Id3V2StreamBase::getImage() is good for images, but other frames might need something similar;
}



void TagEditorDlgImpl::on_m_pNextB_clicked()
{
    if (!closeEditor()) { return; }

    SaveOpt eSaveOpt (save(IMPLICIT)); if (SAVED != eSaveOpt && DISCARDED != eSaveOpt) { return; }

    if (m_pCommonData->nextAlbum())
    {
        ValueRestorer<bool> rst1 (m_bIsNavigating);
        m_bIsNavigating = true;

        m_pTagWriter->clearShowedNonSeqWarn();
        m_pTagWriter->reloadAll("", TagWriter::CLEAR_DATA, TagWriter::CLEAR_ASSGN);
        clearSelection(); // actually here it selects the first cell

        QTimer::singleShot(1, this, SLOT(onShowPatternNote()));
    }
}


void TagEditorDlgImpl::on_m_pPrevB_clicked()
{
    if (!closeEditor()) { return; }

    SaveOpt eSaveOpt (save(IMPLICIT)); if (SAVED != eSaveOpt && DISCARDED != eSaveOpt) { return; }

    if (m_pCommonData->prevAlbum())
    {
        ValueRestorer<bool> rst1 (m_bIsNavigating);
        m_bIsNavigating = true;

        m_pTagWriter->clearShowedNonSeqWarn();
        m_pTagWriter->reloadAll("", TagWriter::CLEAR_DATA, TagWriter::CLEAR_ASSGN);
        clearSelection(); // actually here it selects the first cell

        QTimer::singleShot(1, this, SLOT(onShowPatternNote()));
    }
}



void TagEditorDlgImpl::clearSelection()
{
    QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());
    pSelModel->setCurrentIndex(m_pCurrentAlbumModel->index(0, 0), QItemSelectionModel::SelectCurrent); // !!! needed because otherwise pSelModel->clear() might trigger onAlbSelChanged() for an invalid cell (e.g. when current album has 15 songs, with 12th selected, and the next only has 10)
    pSelModel->clear();
    pSelModel->setCurrentIndex(m_pCurrentAlbumModel->index(0, 0), QItemSelectionModel::SelectCurrent); // !!! needed to have a cell selected //ttt2 try and get rid of  duplicate call to setCurrentIndex;
}


void TagEditorDlgImpl::selectMainCrt() // selects the song that is current in the main window (to be called on the constructor);
{
    QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());
    const Mp3Handler* pCrt (m_pCommonData->getCrtMp3Handler());

    for (int i = 0, n = cSize(m_pTagWriter->m_vpMp3HandlerTagData); i < n; ++i)
    {
        const Mp3Handler* p (m_pTagWriter->m_vpMp3HandlerTagData[i]->getMp3Handler());
        if (p == pCrt)
        {
            pSelModel->setCurrentIndex(m_pCurrentAlbumModel->index(i, 0), QItemSelectionModel::SelectCurrent);
            return;
        }
    }

    CB_ASSERT (false);
}




void TagEditorDlgImpl::updateAssigned()
{
    m_pAssgnBtnWrp->setState(m_pTagWriter->updateAssigned(getSelFields()));
}


// returns the selceted fields, with the first elem as the song number and the second as the field index in TagReader::Feature (it converts columns to fields using TagReader::FEATURE_ON_POS); first column (file name) is ignored
vector<pair<int, int> > TagEditorDlgImpl::getSelFields() const
{
    QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());

    QModelIndexList listSel (pSelModel->selectedIndexes());
    vector<pair<int, int> > vFields;
    for (QModelIndexList::iterator it = listSel.begin(), end = listSel.end(); it != end; ++it)
    {
        QModelIndex ndx (*it);
        int nSong (ndx.row());
        int nField (ndx.column());
        if (nField > 0)
        {
            vFields.push_back(make_pair(nSong, TagReader::FEATURE_ON_POS[nField - 1]));
        }
    }

    return vFields;
}


void TagEditorDlgImpl::resizeIcons()
{
    vector<QToolButton*> v;
    v.push_back(m_pPrevB);
    v.push_back(m_pNextB);
    v.push_back(m_pConfigB);
    v.push_back(m_pSaveB);
    v.push_back(m_pReloadB);
    v.push_back(m_pVarArtistsB);
    v.push_back(m_pCaseB);
    v.push_back(m_pCopyFirstB);
    v.push_back(m_pSortB);
    v.push_back(m_pToggleAssignedB);
    v.push_back(m_pPasteB);
    v.push_back(m_pEditPatternsB);
    v.push_back(m_pPaletteB);

    int k (m_pCommonData->m_nMainWndIconSize);
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        QToolButton* p (v[i]);
        p->setMaximumSize(k, k);
        p->setMinimumSize(k, k);
        p->setIconSize(QSize(k - 4, k - 4));
    }

    {
        QToolButton* p (m_pQueryDiscogsB);
        int h ((k - 4)*2/3);
        //int w (h*135/49);
        int w (h*135/60);
        p->setMaximumSize(w + 4, k);
        p->setMinimumSize(w + 4, k);
        p->setIconSize(QSize(w, h));
    }

    {
        QToolButton* p (m_pQueryMusicBrainzB);
        int h (k - 4);
        int w (h*171/118);
        p->setMaximumSize(w + 4, k);
        p->setMinimumSize(w + 4, k);
        p->setIconSize(QSize(w, h));
    }
}


#if 0
/*override*/ bool TagEditorDlgImpl::eventFilter(QObject* pObj, QEvent* pEvent)
{
    QKeyEvent* pKeyEvent (dynamic_cast<QKeyEvent*>(pEvent));
    int nKey (0 == pKeyEvent ? 0 : pKeyEvent->key());

//static int s_nCnt (0);
    /*if (0 != pKeyEvent && Qt::Key_Escape == nKey)
    {
        qDebug("%d. %s %d", s_nCnt++, pObj->objectName().toUtf8().constData(), (int)pEvent->type()); //return QDialog::eventFilter(pObj, pEvent);
    }*/
static bool bIgnoreNextEsc (false);
    if (0 != pKeyEvent && Qt::Key_Escape == nKey && this == pObj && QEvent::ShortcutOverride == pEvent->type())
    {
        //qDebug("kill esc");
        //return true;
        //SaveOpt eSaveOpt (save(false)); if (SAVED != eSaveOpt && DISCARDED != eSaveOpt) { return true; }
        //if (this != pObj) { return true; }
        //qDebug("kill esc");
        //return true;
        qDebug(" >> save");
        //qDebug("%d. %s %d", s_nCnt++, pObj->objectName().toUtf8().constData(), (int)pEvent->type()); //return QDialog::eventFilter(pObj, pEvent);
        //if (this == pObj)
        {
            SaveOpt eSaveOpt (save(false));
            if (SAVED == eSaveOpt || DISCARDED == eSaveOpt)
            {
                //bIgnoreNextEsc = false;
                goto e1;
            }
        }
        //qDebug(" ###### save");
        //SaveOpt eSaveOpt (save(false)); if (SAVED != eSaveOpt && DISCARDED != eSaveOpt) { return true; }
        bIgnoreNextEsc = true;
        return true;
    }

    //if (0 == pKeyEvent || Qt::Key_Escape != nKey) { return QDialog::eventFilter(pObj, pEvent); }

e1:
    if (0 != pKeyEvent && Qt::Key_Escape == nKey)
    {
        //qDebug("passed through: %d. %s %d", s_nCnt++, pObj->objectName().toUtf8().constData(), (int)pEvent->type());
    }

    if (0 != pKeyEvent && Qt::Key_Escape == nKey && this == pObj && QEvent::KeyPress == pEvent->type())
    {
        bool b (bIgnoreNextEsc);
        bIgnoreNextEsc = false;
        if (b)
        {
            qDebug(" KILL");
            return true;
        }
    }

    return QDialog::eventFilter(pObj, pEvent);
}
#endif

/*override*/ bool TagEditorDlgImpl::eventFilter(QObject* pObj, QEvent* pEvent)
{
    QKeyEvent* pKeyEvent (dynamic_cast<QKeyEvent*>(pEvent));
    int nKey (0 == pKeyEvent ? 0 : pKeyEvent->key());

/*static int s_nCnt (0);
    if (0 != pKeyEvent
        //&& Qt::Key_Escape == nKey
        )
    {
        qDebug("%d. %s %d", s_nCnt++, pObj->objectName().toUtf8().constData(), (int)pEvent->type()); //return QDialog::eventFilter(pObj, pEvent);
    }//*/

    if (0 != pKeyEvent && Qt::Key_Escape == nKey && this == pObj && QEvent::KeyPress == pEvent->type())
    {
        SaveOpt eSaveOpt (save(IMPLICIT));
        if (SAVED != eSaveOpt && DISCARDED != eSaveOpt)
        {
            return true;
        }
    }


    if (0 != pKeyEvent && Qt::Key_Delete == nKey && m_pCurrentAlbumG == pObj && QEvent::KeyPress == pEvent->type())
    {
        eraseSelFields();

        return true;
    }


    return QDialog::eventFilter(pObj, pEvent);
}


void TagEditorDlgImpl::eraseSelFields() // erases the values in the selected fields
{
    vector<pair<int, int> > vSel (getSelFields());
    m_pTagWriter->eraseFields(vSel);
    //m_pAssgnBtnWrp->setState(m_pTagWriter->updateAssigned(vector<pair<int, int> >()));
    m_pAssgnBtnWrp->setState(m_pTagWriter->updateAssigned(vSel));

    m_pAssgnBtnWrp->setState(m_pTagWriter->updateAssigned(vector<pair<int, int> >())); // we don't want to keep any previous value
    updateAssigned(); // needed for the "assign" button to work, because the previous line cleared m_pTagWriter->m_sSelOrigVal

    m_pTagWriter->reloadAll(m_pTagWriter->getCurrentName(), TagWriter::DONT_CLEAR_DATA, TagWriter::DONT_CLEAR_ASSGN); //ttt2 way too many ugly calls, including 2 required calls to m_pAssgnBtnWrp->setState(); restructure the whole "assigned" thing;
    /*
        some details (not completely up-to-date):
            TagWriter::toggleAssigned() should be called when the user clicks on the assign button; changes status of selected cells and returns the new state of m_pToggleAssignedB
            TagWriter::updateAssigned() should be called when the selection changes; returns the new state of m_pToggleAssignedB;

        both TagEditorDlgImpl and TagWriter have updateAssigned(), which is confusing

        better have toggleAssigned() and updateAssigned() use signals (and perhaps guards)
    */
}


/*override*/ void TagEditorDlgImpl::closeEvent(QCloseEvent* pEvent)
{
    pEvent->ignore();
    QCoreApplication::postEvent(this, new QKeyEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier)); // ttt2 not sure is a KeyRelease pair is expected
}




//ttt2 disabled widgets should have tooltips saying why are they disabled / how to enable them
//===================================================================================================================


class Id3V230Writer : public Transformation
{
    //void processId3V2Stream(Id3V2StreamBase& strm, ofstream_utf8& out);
    const TagWriter* m_pTagWriter;
    bool m_bKeepOneValidImg;
    bool m_bFastSave;

    void setupWriter(Id3V230StreamWriter& wrt, const Mp3HandlerTagData* pMp3HandlerTagData);
public:
    Id3V230Writer(const TagWriter* pTagWriter, bool bKeepOneValidImg, bool bFastSave) : m_pTagWriter(pTagWriter), m_bKeepOneValidImg(bKeepOneValidImg), m_bFastSave(bFastSave) {}

    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Saves user-edited ID3V2.3.0 tags."; }
    /*override*/ bool acceptsFastSave() const { return true; }

    static const char* getClassName() { return "Save ID3V2.3.0 tags"; }
};


void Id3V230Writer::setupWriter(Id3V230StreamWriter& wrt, const Mp3HandlerTagData* pMp3HandlerTagData)
{
    string s;
    s = pMp3HandlerTagData->getData(TagReader::TITLE); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_TITLE()); } else { wrt.addTextFrame(KnownFrames::LBL_TITLE(), s); }
    s = pMp3HandlerTagData->getData(TagReader::ARTIST); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_ARTIST()); } else { wrt.addTextFrame(KnownFrames::LBL_ARTIST(), s); }
    s = pMp3HandlerTagData->getData(TagReader::TRACK_NUMBER); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_TRACK_NUMBER()); } else { wrt.addTextFrame(KnownFrames::LBL_TRACK_NUMBER(), s); }
    s = pMp3HandlerTagData->getData(TagReader::GENRE); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_GENRE()); } else { wrt.addTextFrame(KnownFrames::LBL_GENRE(), s); }
    s = pMp3HandlerTagData->getData(TagReader::ALBUM); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_ALBUM()); } else { wrt.addTextFrame(KnownFrames::LBL_ALBUM(), s); }
    s = pMp3HandlerTagData->getData(TagReader::COMPOSER); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_COMPOSER()); } else { wrt.addTextFrame(KnownFrames::LBL_COMPOSER(), s); }
    s = pMp3HandlerTagData->getData(TagReader::VARIOUS_ARTISTS); wrt.setVariousArtists(!s.empty());
    s = pMp3HandlerTagData->getData(TagReader::TIME); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_TIME_DATE_230()); wrt.removeFrames(KnownFrames::LBL_TIME_YEAR_230()); wrt.removeFrames(KnownFrames::LBL_TIME_240()); } else { wrt.setRecTime(TagTimestamp(s)); }

    s = pMp3HandlerTagData->getData(TagReader::IMAGE);
    if (s.empty())
    {
        wrt.removeFrames(KnownFrames::LBL_IMAGE(), -1); // !!! this removes all APIC frames, including those non-cover frames that got assigned to Id3V2StreamBase::m_pPicFrame
    }
    else
    {
        int nImg (pMp3HandlerTagData->getImage());
        CB_ASSERT (nImg >= 0);
        const ImageColl& imgColl (m_pTagWriter->getImageColl());
        const ImageInfo& imgInfo (imgColl[nImg].m_imageInfo);
        int nImgSize (imgInfo.getSize());
        const char* pImgData (imgInfo.getComprData());

        QByteArray recomprImg; // to be used only for ImageInfo::INVALID

        const char* szEncoding (0);
        switch (imgInfo.getCompr())
        {
        case ImageInfo::JPG: szEncoding = "image/jpg"; break;
        case ImageInfo::PNG: szEncoding = "image/png"; break;
        case ImageInfo::INVALID:
            {
                QImage scaledPic;
                ImageInfo::compress(imgInfo.getImage(), scaledPic, recomprImg);
                nImgSize = recomprImg.size();
                pImgData = recomprImg.constData();
                szEncoding = "image/jpg";
                break;
            }
        default:
            CB_ASSERT (false);
        }
        int nEncSize (strlen(szEncoding));
        int nSize (1 + nEncSize + 1 + 1 + 1 + nImgSize);
        vector<char> frm (nSize);
        char* q (&frm[0]);
        *q++ = 0; // enc
        strcpy(q, szEncoding);
        q += nEncSize + 1; // enc + term
        *q++ = Id3V2Frame::PT_COVER;
        *q++ = 0; // null-term descr
        memcpy(q, pImgData, nImgSize);
        wrt.addImg(frm);
    }

    {
        //s = pMp3HandlerTagData->getData(TagReader::RATING);
        double d (pMp3HandlerTagData->getRating());
        CB_ASSERT (d <= 5);

        if (d < 0)
        {
            wrt.removeFrames(KnownFrames::LBL_RATING());
        }
        else
        {
            d = 1 + d*254/5;
            vector<char> frm (2);
            frm[0] = 0;
            frm[1] = (int)d; // ttt2 signed char on some CPUs might have issues; OK on x86, though
            wrt.addBinaryFrame(KnownFrames::LBL_RATING(), frm);
        }
    }
}
//ttt2 when multiple id3v2 are found, the bkg color doesn't change if the 2 id3v2 are switched; probably ok, though, in the sense that if at least one field is assigned, all the others will be used when saving

/*override*/ Transformation::Result Id3V230Writer::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    LAST_STEP("Id3V230Writer::apply() " + h.getName());
    const vector<DataStream*>& vpStreams (h.getStreams());

    const Mp3HandlerTagData* pMp3HandlerTagData (0);
    for (int i = 0, n = cSize(m_pTagWriter->m_vpMp3HandlerTagData); i < n; ++i)
    {
        const Mp3HandlerTagData* p (m_pTagWriter->m_vpMp3HandlerTagData[i]);
        if (p->getMp3Handler() == &h)
        {
            pMp3HandlerTagData = p;
            goto e1;
        }
    }

    //return NOT_CHANGED;
    CB_ASSERT (false); // only handlers from m_pTagWriter (that also need saving) are supposed to get here;

e1:
    CB_ASSERT (0 != pMp3HandlerTagData);

    ifstream_utf8 in (h.getName().c_str(), ios::binary);

    { // temp
        Id3V2StreamBase* pId3V2Source (0);
        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            pId3V2Source = dynamic_cast<Id3V2StreamBase*>(vpStreams[i]);
            if (0 != pId3V2Source) { break; }
        }

        Id3V230StreamWriter wrt (m_bKeepOneValidImg, m_bFastSave, pId3V2Source, h.getName()); // OK if pId3V2Source is 0

        setupWriter(wrt, pMp3HandlerTagData);

        if (m_bFastSave && !vpStreams.empty() && pId3V2Source == vpStreams[0])
        {
            try
            {
                long long nSize, nOrigTime;
                getFileInfo(h.getName(), nOrigTime, nSize);

                {
                    fstream_utf8 f (h.getName().c_str(), ios::binary | ios_base::in | ios_base::out);

                    wrt.write(f, int(pId3V2Source->getSize()));
                }

                h.reloadId3V2(); //ttt2 perhaps return the pointer to the old Id3V2 instead of destroying it, and keep it somewhere until saving is done; then m_bIsFastSaving won't be needed, and no "N/A"s will be displayed in ID3V2 fields when saving; OTOH those pointers can't be kept fully alive, because they can't retrieve their data from disk; so better leave it as is;

                if (transfConfig.m_options.m_bKeepOrigTime)
                {
                    setFileDate(h.getName(), nOrigTime);
                }
                else
                {
                    getFileInfo(h.getName(), nOrigTime, nSize);
                    h.m_nFastSaveTime = nOrigTime;
                }

                return NOT_CHANGED; // !!!
            }
            catch (const WriteError&)
            {
                // !!! nothing: normally this gets triggered before changing the file in any way (the reason being that it doesn't fit), so we go on and create a temporary file; //ttt2 the exception might get thrown on versioned filesystems, that use copy-on-write, if the disk becomes full, bu even then there's not much that can be done;
            }
        }

        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);

        ofstream_utf8 out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        wrt.write(out); // may throw, but it will be caught

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            bool bCopy (0 == dynamic_cast<Id3V2StreamBase*>(p));

            BrokenDataStream* pBrk (dynamic_cast<BrokenDataStream*>(p));
            if (bCopy && 0 != pBrk && (pBrk->getBaseName() == Id3V230Stream::getClassDisplayName() || pBrk->getBaseName() == Id3V240Stream::getClassDisplayName()))
            {
                bCopy = false;
            }

            if (bCopy)
            {
                p->copy(in, out);
            }
        }
    }

    return CHANGED_NO_RECALL;
}


//========================================================================================================================
//========================================================================================================================
//========================================================================================================================

//ttt2 perhaps all licences in root dir

// based on configuration, either just saves the tags or asks the user for confirmation; returns true iff all tags have been saved or if none needed saving; it should be followed by a reload(), either for the current or for the next/prev album; if bExplicitCall is true, the "ASK" option is turned into "SAVE";
TagEditorDlgImpl::SaveOpt TagEditorDlgImpl::save(bool bImplicitCall)
{
    string strCrt (m_pTagWriter->getCurrentName());

    deque<const Mp3Handler*> vpHndlr;
    bool bHasUnsavedAssgn (false);
    bool bHasUnsavedNonId3V2 (false);
    for (int i = 0, n = cSize(m_pTagWriter->m_vpMp3HandlerTagData); i < n; ++i)
    {
        //const Mp3Handler* p (m_pTagWriter->m_vpMp3HandlerTagData[i]->getMp3Handler());

        bool bAssgn (false);
        bool bNonId3V2 (false);
        m_pTagWriter->hasUnsaved(i, bAssgn, bNonId3V2);
        if (
                (bAssgn && (!bImplicitCall || CommonData::DISCARD != m_pCommonData->m_eAssignSave)) ||
                (bNonId3V2 && (!bImplicitCall || CommonData::DISCARD != m_pCommonData->m_eNonId3v2Save))
            )
        {
            vpHndlr.push_back(m_pTagWriter->m_vpMp3HandlerTagData[i]->getMp3Handler());
        }
        bHasUnsavedAssgn = bHasUnsavedAssgn || bAssgn;
        bHasUnsavedNonId3V2 = bHasUnsavedNonId3V2 || bNonId3V2;
    }
//ttt2 perhaps separate setting for showing and saving non-id3v2 fields

    {
        int nUnassignedImagesCnt (m_pTagWriter->getUnassignedImagesCount());
        if (bImplicitCall && nUnassignedImagesCnt > 0)
        {
            int nOpt (showMessage(this, QMessageBox::Question, 1, 1, "Confirm", (nUnassignedImagesCnt > 1 ? QString("You added %1 images but then you didn't assign them to any songs. What do you want to do?").arg(nUnassignedImagesCnt) : QString("You added an image but then you didn't assign it to any song. What do you want to do?")), "&Discard", "&Cancel"));
            if (1 == nOpt) { return CANCELLED; }
        }
    }

    if (vpHndlr.empty())
    {
        return SAVED;
    }

    if (
            bImplicitCall &&
            (
                (bHasUnsavedAssgn && CommonData::ASK == m_pCommonData->m_eAssignSave) ||
                (bHasUnsavedNonId3V2 && CommonData::ASK == m_pCommonData->m_eNonId3v2Save)
            )
        )
    { // ask
        int nOpt;
        if ((bHasUnsavedAssgn && CommonData::ASK == m_pCommonData->m_eAssignSave) && (bHasUnsavedNonId3V2 && CommonData::DISCARD == m_pCommonData->m_eNonId3v2Save))
        {
            nOpt = showMessage(this, QMessageBox::Question, 0, 2, "Confirm", "There are unsaved fields that you assigned a value to, as well as fields whose value doesn't match the ID3V2 value. What do you want to do?", "&Save", "&Discard", "&Cancel");
        }
        else if (bHasUnsavedAssgn && CommonData::ASK == m_pCommonData->m_eAssignSave)
        {
            nOpt = showMessage(this, QMessageBox::Question, 0, 2, "Confirm", "There are unsaved fields that you assigned a value to. What do you want to do?", "&Save", "&Discard", "&Cancel");
        }
        else
        {
            nOpt = showMessage(this, QMessageBox::Question, 0, 2, "Confirm", "There are fields whose value doesn't match the ID3V2 value. What do you want to do?", "&Save", "&Discard", "&Cancel");
        }

        if (1 == nOpt) { return DISCARDED; }
        if (2 == nOpt) { return CANCELLED; }
    }

    m_bDataSaved = true;

    Id3V230Writer wrt (m_pTagWriter, m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave());
    vector<Transformation*> vpTransf;
    vpTransf.push_back(&wrt);

    bool bRes;
    {
        ValueRestorer<bool> rst (m_bIsFastSaving);
        m_bIsFastSaving = m_pCommonData->useFastSave();

        ValueRestorer<bool> rst1 (m_bIsSaving);
        m_bIsSaving = true;

        bRes = transform(vpHndlr, vpTransf, "Saving ID3V2.3.0 tags", this, m_pCommonData, m_transfConfig);
    }

    m_pTagWriter->reloadAll(strCrt, TagWriter::DONT_CLEAR_DATA, TagWriter::CLEAR_ASSGN);
    //m_pTagWriter->updateAssigned(vector<pair<int, int> >());

    m_pAssgnBtnWrp->setState(m_pTagWriter->updateAssigned(vector<pair<int, int> >())); // we don't want to keep any previous value
    updateAssigned(); // needed for the "assign" button to work, because the previous line cleared m_pTagWriter->m_sSelOrigVal
    return bRes ? SAVED : PARTIALLY_SAVED;
}

//ttt2 hide VA column if not used
void TagEditorDlgImpl::onHelp()
{
    openHelp("190_tag_editor.html");
}


extern bool s_bToldAboutPatternsInCrtRun;

void TagEditorDlgImpl::onShowPatternNote()
{
    if (m_pTagWriter->shouldShowPatternsNote())
    {
        s_bToldAboutPatternsInCrtRun = true;

        HtmlMsg::msg(this, 0, 0, &m_pCommonData->m_bToldAboutPatterns, HtmlMsg::DEFAULT, "Info", "<p style=\"margin-bottom:1px; margin-top:12px; \">Some fields are missing or may be incomplete. While this is usually solved by downloading correct information, there are a cases when this approach doesn't work, like custom compilations, rare albums, or missing tracks.</p>"

        "<p style=\"margin-bottom:1px; margin-top:12px; \">If your current folder fits one of these cases or you simply have consistently named files that you would prefer to use as a source of track info, you may want to take a look at the tag editor's patterns, at <a href=\"http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/220_tag_editor_patterns.html\">http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/220_tag_editor_patterns.html</a>.</p>", 550, 300, "OK");
    }
}




//========================================================================================================================================================
//========================================================================================================================================================
//========================================================================================================================================================



CurrentFileDelegate::CurrentFileDelegate(QTableView* pTableView, const CommonData* pCommonData) : QItemDelegate(pTableView), m_pTableView(pTableView), m_pCommonData(pCommonData)
{
    CB_CHECK1 (0 != pTableView, std::runtime_error("NULL QTableView not allowed"));
    //connect(pTableView->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), pTableView, SLOT(resizeRowsToContents()));
}

/*override*/ void CurrentFileDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    pPainter->save();

    //pPainter->fillRect(option.rect, QBrush(m_listPainter.getColor(m_listPainter.getAvailable()[index.row()], index.column(), pPainter->background().color()))); //ttt2 make sure background() is the option to use
    /*QStyleOptionViewItemV2 myOption (option);
    myOption.displayAlignment = m_listPainter.getAlignment(index.column());*/
    QString s (index.model()->data(index, Qt::DisplayRole).toString());
    if ("\1" == s)
    { // tag not present
        pPainter->fillRect(option.rect, QBrush(m_pCommonData->m_vTagEdtColors[CommonData::COLOR_FILE_TAG_MISSING]));
    }
    else if ("\2" == s)
    { // not applicable
        pPainter->fillRect(option.rect, QBrush(m_pCommonData->m_vTagEdtColors[CommonData::COLOR_FILE_NA]));
    }
    else if ("\3" == s)
    { // applicable, but no data found
        pPainter->fillRect(option.rect, QBrush(m_pCommonData->m_vTagEdtColors[CommonData::COLOR_FILE_NO_DATA]));
    }
    else
    {
        pPainter->fillRect(option.rect, QBrush(m_pCommonData->m_vTagEdtColors[CommonData::COLOR_FILE_NORM]));
        QItemDelegate::paint(pPainter, option, index);
    }


    /*
    //ttt2 first 3 cases don't show the dotted line because QItemDelegate::paint() doesn't get called; perhaps drawDecoration() should be called, but copying these things from qitemdelegate.cpp didn't help:

    QVariant value;
    value = index.data(Qt::DecorationRole);
    QPixmap pixmap;
    pixmap = decoration(option, value);

    drawDecoration(pPainter, option, QRect(0, 0, 15, 15), pixmap);

??? or perhaps use     QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect?, &option, pPainter);


    */

    pPainter->restore();
}


QSize CurrentFileDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
return QItemDelegate::sizeHint(option, index);
/*
    if (!index.isValid()) { return QSize(); }
    //cout << option.rect.width() << "x" << option.rect.height() << " ";
    int nMargin (QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1);
    //cout << "margin=" << nMargin << endl;

    int j (index.column());
    int nColWidth (m_pTableView->horizontalHeader()->sectionSize(j));
    //QRect r (0, 0, nColWidth - 2*nMargin - 1, 10000); // !!! this "-1" is what's different from Qt's implementation //ttt2 see if this is fixed in 4.4 2008.30.06 - apparently it's not fixed and the workaround no longer works

    //QSize res (option.fontMetrics.boundingRect(r, Qt::AlignTop | Qt::TextWordWrap, index.data(Qt::DisplayRole).toString()).size());

    //cout << "at (" << index.row() << "," << index.column() << "): " << res.width() << "x" << res.height();

    QSize res (nColWidth, CELL_HEIGHT);
    //res.setWidth(nColWidth);

    //cout << " => " << res.width() << "x" << res.height() << endl;

    //QSize res (fontMetrics().size(0, text()));
    return res;
*/
}//*/

//ttt2 perhaps allow selected cells to show background color, so it's possible to know if a cell is assigned, id3v2 or non-id3v2 even when it's selected; options include replaceing background with a frame, background with a darkened color based on the normal background, negation of normal color, some pattern




CurrentAlbumDelegate::CurrentAlbumDelegate(QTableView* pTableView, TagEditorDlgImpl* pTagEditorDlgImpl) : QItemDelegate(pTableView), m_pTableView(pTableView), m_pTagEditorDlgImpl(pTagEditorDlgImpl), m_pTagWriter(pTagEditorDlgImpl->getTagWriter())
{
    CB_CHECK1 (0 != pTableView, std::runtime_error("NULL QTableView not allowed"));
    //connect(pTableView->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), pTableView, SLOT(resizeRowsToContents()));
}

/*override*/ void CurrentAlbumDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (m_pTagEditorDlgImpl->isSaving() || m_pTagEditorDlgImpl->isNavigating())
    {
        QItemDelegate::paint(pPainter, option, index);
        return;
    }


    pPainter->save();

    //pPainter->fillRect(option.rect, QBrush(m_listPainter.getColor(m_listPainter.getAvailable()[index.row()], index.column(), pPainter->background().color()))); //ttt2 make sure background() is the option to use
    /*QStyleOptionViewItemV2 myOption (option);
    myOption.displayAlignment = m_listPainter.getAlignment(index.column());*/
    //QString s (index.model()->data(index, Qt::DisplayRole).toString());
    int nField (index.column());
    Mp3HandlerTagData::Status eStatus (nField > 0 ? m_pTagWriter->getStatus(index.row(), TagReader::FEATURE_ON_POS[nField - 1]) : Mp3HandlerTagData::ID3V2_VAL);
    QColor col;
    switch (eStatus)
    {
    case Mp3HandlerTagData::EMPTY:
    case Mp3HandlerTagData::ID3V2_VAL: col = m_pTagEditorDlgImpl->getCommonData()->m_vTagEdtColors[CommonData::COLOR_ALB_NORM]; break;

    case Mp3HandlerTagData::NON_ID3V2_VAL: col = m_pTagEditorDlgImpl->getCommonData()->m_vTagEdtColors[CommonData::COLOR_ALB_NONID3V2]; break;

    case Mp3HandlerTagData::ASSIGNED: col = m_pTagEditorDlgImpl->getCommonData()->m_vTagEdtColors[CommonData::COLOR_ALB_ASSIGNED]; break;
    }

    //if (1 == index.column())
    {
        pPainter->fillRect(option.rect, QBrush(col));
    }

    //else
    {
        QItemDelegate::paint(pPainter, option, index);
    }

    pPainter->restore();
}
//ttt2 2009.04.06 - for all delegates: see if they can be removed by using the standard delegate and adding more functionality to the models; see Qt::ForegroundRole, Qt::TextAlignmentRole, Qt::FontRole & Co

/*override*/ QWidget* CurrentAlbumDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& style, const QModelIndex& index) const
{
    int nField (index.column());
    if (0 == nField || 5 == nField || 8 == nField) { return 0; }
    QWidget* pEditor (QItemDelegate::createEditor(pParent, style, index));
    //qDebug("%s", p->metaObject()->className());
    connect(pEditor, SIGNAL(destroyed(QObject*)), this, SLOT(onEditorDestroyed(QObject*)));
    m_spEditors.insert(pEditor);
//qDebug("create %p", pEditor);
    return pEditor;
}

void CurrentAlbumDelegate::onEditorDestroyed(QObject* p)
{
//qDebug("destroy %p", p);
    CB_ASSERT (1 == m_spEditors.count(p));
    m_spEditors.erase(p);
}

bool CurrentAlbumDelegate::closeEditor() // closes the editor opened with F2, saving the data; returns false if there was some error and it couldn't close
{
    CB_ASSERT (cSize(m_spEditors) <= 1);
    if (m_spEditors.empty())
    {
        return true;
    }
    //delete m_pEditor;

    if (0 != showMessage(m_pTableView, QMessageBox::Warning, 1, 1, "Warning", "You are editing data in a cell. If you proceed that change will be lost. Proceed and lose the data?", "Proceed", "Cancel")) { return false; } //ttt2 try and post somehow the content of the editor (?? perhaps send it a keyboard message that ENTER was pressed)
    //ttt2 review this

    delete *m_spEditors.begin();
    return true;
}


//ttt2 perhaps manufacture track numbers when pasting tables, if track numbers don't exist



//======================================================================================================================
//======================================================================================================================
//======================================================================================================================



/*
tag editor performance:

TagWriter::reloadAll() is called twice when going to a new album, but that's not very important, because reloading no longer takes a lot of time; if there are no images it's very fast, but even with images it's no big deal;

most time is used in rescanning a modified file, in Mp3Handler::parse(), as this example of saving a modified song shows:
    save: 0.23" (copying streams from one file to another)
    read ID3: 0.29" (because it parses images)
    read MPEG audio: 1.10"

OTOH TagWriter::reloadAll() typically takes less than 0.10" for a whole album, even when pictures are present; what makes this slower is having pictures in the current directory, because they get rescanned at each reload, while the ones inside the MP3s don't. So eliminating the duplicate call wouldn't achieve much. The duplicate was needed at some point in time because of some Qt bug (or so it looked like); not sure if it's really needed, but removing it is almost guaranteed to result in some bugs (like randomly resizing the image panel) that are not consistently reproducible.)


Rescanning of saved files might be eliminated if speed is so important, but it seems a bad idea; the main reason is that without rescanning the notes and the streams are going to be out of synch, and this is fundamentally unsolvable. Marking such files as "dirty" doesn't seem a very good idea, and could be quite confusing and hard to get right. Not marking them is worse. Perhaps "fast editor" option ...


A less important performance issue is in ImageInfoPanelWdgImpl::ImageInfoPanelWdgImpl(): assigning of an image to a label takes more than 0.2"



*/

//ttt2 in tag editor: magnify image for one album, Ctrl+C, go to another album, Ctrl+V -> format error
