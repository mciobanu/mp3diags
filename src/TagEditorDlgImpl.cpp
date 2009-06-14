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


using namespace std;
using namespace pearl;

using namespace TagEditor;

//======================================================================================================================
//======================================================================================================================
//======================================================================================================================


CurrentAlbumModel::CurrentAlbumModel(TagEditorDlgImpl* pTagEditorDlgImpl) : m_pTagEditorDlgImpl(pTagEditorDlgImpl), m_pTagWriter(pTagEditorDlgImpl->getTagWriter()), m_pCommonData(pTagEditorDlgImpl->getCommonData())
{
}



/*override*/ int CurrentAlbumModel::rowCount(const QModelIndex&) const
{
    return cSize(m_pTagWriter->m_vpMp3HandlerTagData);
}


/*override*/ int CurrentAlbumModel::columnCount(const QModelIndex&) const
{
    return TagReader::LIST_END + 1;
}


/*override*/ QVariant CurrentAlbumModel::data(const QModelIndex& index, int nRole) const
{
    if (!index.isValid()) { return QVariant(); }
    int i (index.row());
    int j (index.column());

    if (Qt::DisplayRole != nRole && Qt::ToolTipRole != nRole && Qt::EditRole != nRole) { return QVariant(); }
    QString s;

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
        QMessageBox::critical(m_pTagEditorDlgImpl, "Error", "The data contained errors and couldn't be saved");
        return false; // ttt1 if it gets here the data is lost; perhaps CurrentAlbumDelegate should be modified more extensively, to not close the editor on Enter if this returns false;
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
    if (!index.isValid() || 0 == m_pTagWriter->getCurrentHndl()) { return QVariant(); }
    int i (index.row());
    int j (index.column());

    if (Qt::DisplayRole != nRole && Qt::ToolTipRole != nRole) { return QVariant(); }
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




TagEditorDlgImpl::TagEditorDlgImpl(QWidget* pParent, CommonData* pCommonData, TransfConfig& transfConfig) : QDialog(pParent, getDialogWndFlags()), Ui::TagEditorDlg(), m_pCommonData(pCommonData), m_bSectionMovedLock(false), m_transfConfig(transfConfig)
{
    setupUi(this);

    static bool s_bColInit (false);
    if (!s_bColInit)
    {
        s_bColInit = true;
        //TagWriter::ALBFILE_NORM_COLOR = QColor(m_pTableView->palette().color(QPalette::Active, QPalette::Base));
        TagEditorDlgImpl::ALBFILE_NORM_COLOR = QColor(QPalette().color(QPalette::Active, QPalette::Base));
    }


    m_pAssgnBtnWrp = new AssgnBtnWrp (m_pAssignedB);

    {
        m_pTagWriter = new TagWriter(m_pCommonData, this);
        loadTagWriterInf();
        connect(m_pTagWriter, SIGNAL(albumChanged()), this, SLOT(onAlbumChanged()));
        connect(m_pTagWriter, SIGNAL(fileChanged()), this, SLOT(onFileChanged()));
        connect(m_pTagWriter, SIGNAL(imagesChanged()), this, SLOT(onImagesChanged()));
    }

    m_pCurrentAlbumModel = new CurrentAlbumModel(this);
    m_pCurrentFileModel = new CurrentFileModel(this);

    {
        m_pCurrentAlbumG->verticalHeader()->setResizeMode(QHeaderView::Interactive);
        m_pCurrentAlbumG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT + 1);
        m_pCurrentAlbumG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT + 1);//*/

        m_pCurrentAlbumG->setModel(m_pCurrentAlbumModel);

        m_pAlbumDel = new CurrentAlbumDelegate(m_pCurrentAlbumG, this);
        m_pCurrentAlbumG->setItemDelegate(m_pAlbumDel);


        //connect(m_pCurrentAlbumG, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onAlbSelChanged())); // ttt1 see if both this and next are needed (next seems enough)
        connect(m_pCurrentAlbumG->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onAlbSelChanged()));
    }


    {
        m_pCurrentFileG->verticalHeader()->setResizeMode(QHeaderView::Interactive);
        m_pCurrentFileG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT + 1);
        m_pCurrentFileG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT + 1);//*/

        m_pCurrentFileG->setModel(m_pCurrentFileModel);
        CurrentFileDelegate* pDel (new CurrentFileDelegate(m_pCurrentFileG));

        connect(m_pCurrentFileG->horizontalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(onFileSelSectionMoved(int, int, int)));

        m_pCurrentFileG->setItemDelegate(pDel);
    }

    m_pCurrentFileG->horizontalHeader()->setMovable(true);

    {
        int nWidth, nHeight;
        QByteArray vPrevState;
        m_pCommonData->m_settings.loadTagEdtSettings(nWidth, nHeight, vPrevState);
        if (nWidth > 400 && nHeight > 400)
        {
            resize(nWidth, nHeight);
        }
        else
        {
            defaultResize(*this);
        }

        if (!vPrevState.isNull())
        {
            m_pTagEdtSplitter->restoreState(vPrevState);
        }
        m_pTagEdtSplitter->setOpaqueResize(false);
    }


    {
        delete m_pRemovableL; // m_pRemovableL's only job is to make m_pImagesW visible in QtDesigner

        QWidget* pWidget (new QWidget (this));
        pWidget->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

        QHBoxLayout* pScrlLayout (new QHBoxLayout (pWidget));
        pWidget->setLayout(pScrlLayout);

        pWidget->layout()->setContentsMargins(0, 0, 0, 4 + 1); // "+1" added to look better, but don't know how to calculated

        m_pImgScrollArea = new QScrollArea (m_pImagesW);

        m_pImagesW->layout()->addWidget(m_pImgScrollArea);

        m_pImgScrollArea->setWidget(pWidget);
        m_pImgScrollArea->setFrameShape(QFrame::NoFrame);
    }

    //layout()->update();
    /*layout()->update();
    widget_4->layout()->update();
    widget->layout()->update();
    layout()->update();
    widget_4->layout()->update();
    widget->layout()->update();*/

    m_pTagWriter->reloadAll("", TagWriter::CLEAR);
    selectMainCrt();

    resizeIcons();

    m_pCurrentAlbumG->installEventFilter(this);
    installEventFilter(this);

    QTimer::singleShot(1, this, SLOT(onShow())); // just calls resizeTagEditor(); !!! needed to properly resize the table columns; album and file tables have very small widths until they are actually shown, so calling resizeTagEditor() earlier is pointless; calling update() on various layouts seems pointless as well; (see also DoubleList::resizeEvent() )
}



TagEditorDlgImpl::~TagEditorDlgImpl()
{
    saveTagWriterInf();
    QByteArray vState (m_pTagEdtSplitter->saveState());
    m_pCommonData->m_settings.saveTagEdtSettings(width(), height(), vState);
    delete m_pCurrentAlbumModel;
    delete m_pCurrentFileModel;
    delete m_pTagWriter;
    delete m_pAssgnBtnWrp;
}


string TagEditorDlgImpl::run()
{
    exec();
    return m_pTagWriter->getCurrentName();
}



void TagEditorDlgImpl::resizeTagEditor()
{
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
            nRes -= 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, &m_tbl); //ttt1 Qt 4.4 (and below) - specific; in 4.5 there's a QStyle::PM_ScrollView_ScrollBarSpacing
        }*/

        nHeight += nScrollBarWidth;
    }
    m_pImagesW->setMaximumSize(100000, nHeight); // ttt1 see about adding space for horizontal scrollbar


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
    SimpleQTableViewWidthInterface intf2 (*m_pCurrentFileG);
    ColumnResizer rsz2 (intf2, 100, ColumnResizer::DONT_FILL, ColumnResizer::CONSISTENT_RESULTS);
}


/*override*/ void TagEditorDlgImpl::resizeEvent(QResizeEvent* /*pEvent*/)
{
    resizeTagEditor();
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
            nImgPos = m_pTagWriter->addImage(*pImageInfo);

            delete pImageInfo;
        }

        if (0 != pAlbumInfo)
        {
            if (-1 != nImgPos)
            {
                pAlbumInfo->m_imageInfo = m_pTagWriter->getImageColl()[nImgPos];
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
            nImgPos = m_pTagWriter->addImage(*pImageInfo);

            //resizeTagEditor();
            delete pImageInfo;
        }

        if (0 != pAlbumInfo)
        {
            if (-1 != nImgPos)
            {
                pAlbumInfo->m_imageInfo = m_pTagWriter->getImageColl()[nImgPos];
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
    /*u.push_back("%a/%b[[ ](%y)]/[[%r]%n][.][ ][-[ ]]%t"); //ttt1 OS specific
    u.push_back("%b[[ ](%y)]/[[%r]%n][.][ ]%a[ ]-[ ]%t");
    u.push_back("%a - %b/[[%r]%n][.][ ][-[ ]]%t");
    u.push_back("[%n][.][ ][-[ ]]%t");*/

    u.push_back("%a/%b[(%y)]/[[%r]%n][.][ ][-]%t"); //ttt1 OS specific
    u.push_back("%b[(%y)]/[[%r]%n][.]%a-%t");
    u.push_back("%a-%b/[[%r]%n][.][ ][-]%t");
    u.push_back("/%n[.]%a-%t");
    u.push_back("[%n][.][ ][-]%t");

    TagEdtPatternsDlgImpl dlg (this, m_pCommonData->m_settings, u);
    if (dlg.run(v))
    {
        m_pTagWriter->updatePatterns(v);
    }
}


void TagEditorDlgImpl::on_m_pPaletteB_clicked()
{
    if (!closeEditor()) { return; }

    PaletteDlgImpl dlg (this);
    dlg.exec();
}


// adds new ImageInfoPanelWdgImpl instances, connects assign button and calls resizeTagEditor()
void TagEditorDlgImpl::onImagesChanged()
{
    QLayout* pLayout (m_pImgScrollArea->widget()->layout());
    int nPanelCnt (pLayout->count());
    const ImageColl& imgColl (m_pTagWriter->getImageColl());
    int nVecCnt (imgColl.size());
    CB_ASSERT (nVecCnt >= nPanelCnt);

    for (int i = nPanelCnt; i < nVecCnt; ++i)
    {
        ImageInfoPanelWdgImpl* p (new ImageInfoPanelWdgImpl(this, imgColl[i], i));
        m_pImgScrollArea->widget()->layout()->addWidget(p);
        p->show();
        connect(p, SIGNAL(assignImage(int)), m_pTagWriter, SLOT(onAssignImage(int)));
        m_pTagWriter->addImgWidget(p);
    }

    resizeTagEditor();
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
    m_pCrtDirTagEdtE->setText(convStr(p->getDir()));
}

void TagEditorDlgImpl::on_m_pConfigB_clicked()
{
    if (!closeEditor()) { return; }

    ConfigDlgImpl dlg (m_transfConfig, m_pCommonData, this, ConfigDlgImpl::SOME_TABS);

    if (dlg.run())
    {
        m_pCommonData->m_settings.saveMiscConfigSettings(m_pCommonData);
        m_pCommonData->m_settings.saveTransfConfig(m_transfConfig); // transformation
        resizeIcons();
        resizeTagEditor();
    }
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

            v.push_back(make_pair(string("%a/%b[[ ](%y)]/[[%r]%n][.][ ][-[ ]]%t"), -1)); //ttt1 OS specific
            v.push_back(make_pair(string("%b[[ ](%y)]/[[%r]%n][.][ ]%a[ ]-[ ]%t"), -1));
            v.push_back(make_pair(string("%a - %b/[[%r]%n][.][ ][-[ ]]%t"), -1));
            v.push_back(make_pair(string("[%n][.][ ][-[ ]]%t"), -1));
        }
        else if (1 == cSize(v) && "*" == v[0].first)
        {
            v.clear();
        }*/

        bErr = !m_pTagWriter->updatePatterns(v) || bErr;
//if (bErr) { qDebug("error when reading patterns"); } //ttt remove
        if (bErr)
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
    }
}



void TagEditorDlgImpl::on_m_pAssignedB_clicked()
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
    m_pTagWriter->reloadAll(m_pTagWriter->getCurrentName(), TagWriter::CLEAR);
    updateAssigned();
}

// copies the values from the first row to the other rows for columns that have at least a cell selected (doesn't matter if more than 1 cells are selected);
void TagEditorDlgImpl::on_m_pCopyFirstB_clicked()
{
    if (!closeEditor()) { return; }

    m_pTagWriter->copyFirst();
}

void TagEditorDlgImpl::on_m_pSaveB_clicked() //ttt1 perhaps make this save selected list, by using SHIFT
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



void TagEditorDlgImpl::on_m_bSortB_clicked()
{
    if (!closeEditor()) { return; }

    clearSelection();
    m_pTagWriter->sort();
    //m_pCurrentAlbumModel->emitLayoutChanged();
}



void TagEditorDlgImpl::onAlbSelChanged()
{
    updateAssigned();

    QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());

    int n (pSelModel->currentIndex().row());
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
//ttt1 perhaps detect when files are removed/changed; while most tags in Id3V2 are stored with the handler, the longer ones (e.g. pictures, but others might be affected too) are dumped from memory by Id3V230Frame's constructor and retrieved from disk when needed; the user should be notified when something becomes unavailable; not sure where to do this, though: Id3V2StreamBase::getImage() is good for images, but other frames might need something similar;
}



void TagEditorDlgImpl::on_m_pNextB_clicked()
{
    if (!closeEditor()) { return; }

    SaveOpt eSaveOpt (save(IMPLICIT)); if (SAVED != eSaveOpt && DISCARDED != eSaveOpt) { return; }

    if (m_pCommonData->nextAlbum())
    {
        m_pTagWriter->clearShowedNonSeqWarn();
        m_pTagWriter->reloadAll("", TagWriter::CLEAR);
        clearSelection(); // actually here it selects the first cell
    }
}


void TagEditorDlgImpl::on_m_pPrevB_clicked()
{
    if (!closeEditor()) { return; }

    SaveOpt eSaveOpt (save(IMPLICIT)); if (SAVED != eSaveOpt && DISCARDED != eSaveOpt) { return; }

    if (m_pCommonData->prevAlbum())
    {
        m_pTagWriter->clearShowedNonSeqWarn();
        m_pTagWriter->reloadAll("", TagWriter::CLEAR);
        clearSelection(); // actually here it selects the first cell
    }
}



void TagEditorDlgImpl::clearSelection()
{
    QItemSelectionModel* pSelModel (m_pCurrentAlbumG->selectionModel());
    pSelModel->setCurrentIndex(m_pCurrentAlbumModel->index(0, 0), QItemSelectionModel::SelectCurrent); // !!! needed because otherwise pSelModel->clear() might trigger onAlbSelChanged() for an invalid cell (e.g. when current album has 15 songs, with 12th selected, and the next only has 10)
    pSelModel->clear();
    pSelModel->setCurrentIndex(m_pCurrentAlbumModel->index(0, 0), QItemSelectionModel::SelectCurrent); // !!! needed to have a cell selected //ttt1 try and get rid of  duplicate call to setCurrentIndex;
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
    v.push_back(m_pCopyFirstB);
    v.push_back(m_bSortB);
    v.push_back(m_pAssignedB);
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
        qDebug("%d. %s %d", s_nCnt++, pObj->objectName().toUtf8().data(), (int)pEvent->type()); //return QDialog::eventFilter(pObj, pEvent);
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
        //qDebug("%d. %s %d", s_nCnt++, pObj->objectName().toUtf8().data(), (int)pEvent->type()); //return QDialog::eventFilter(pObj, pEvent);
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
        //qDebug("passed through: %d. %s %d", s_nCnt++, pObj->objectName().toUtf8().data(), (int)pEvent->type());
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
        qDebug("%d. %s %d", s_nCnt++, pObj->objectName().toUtf8().data(), (int)pEvent->type()); //return QDialog::eventFilter(pObj, pEvent);
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

    m_pTagWriter->reloadAll(m_pTagWriter->getCurrentName(), TagWriter::DONT_CLEAR); //ttt1 way too many ugly calls, including 2 required calls to m_pAssgnBtnWrp->setState(); restructure the whole "assigned" thing;
    /*
        some details (not completely up-to-date):
            TagWriter::toggleAssigned() should be called when the user clicks on the assign button; changes status of selected cells and returns the new state of m_pAssignedB
            TagWriter::updateAssigned() should be called when the selection changes; returns the new state of m_pAssignedB;

        both TagEditorDlgImpl and TagWriter have updateAssigned(), which is confusing

        better have toggleAssigned() and updateAssigned() use signals (and perhaps guards)
    */
}


/*override*/ void TagEditorDlgImpl::closeEvent(QCloseEvent* pEvent)
{
    pEvent->ignore();
    QCoreApplication::postEvent(this, new QKeyEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier)); // ttt2 not sure is a KeyRelease pair is expected
}




//ttt1 disabled widgets should have tooltips saying why are they disabled / how to enable them
//===================================================================================================================


class Id3V230Writer : public Transformation
{
    //void processId3V2Stream(Id3V2StreamBase& strm, ofstream& out);
    const TagWriter* m_pTagWriter;
    bool m_bKeepOneValidImg;
public:
    Id3V230Writer(const TagWriter* pTagWriter, bool bKeepOneValidImg) : m_pTagWriter(pTagWriter), m_bKeepOneValidImg(bKeepOneValidImg) {}

    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Saves user-edited ID3V2.3.0 tags."; }

    static const char* getClassName() { return "Save ID3V2.3.0 tags"; }
};




/*override*/ Transformation::Result Id3V230Writer::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
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

    ifstream in (h.getName().c_str(), ios::binary);

    { // temp
        Id3V2StreamBase* pId3V2Source (0);
        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            pId3V2Source = dynamic_cast<Id3V2StreamBase*>(vpStreams[i]);
            if (0 != pId3V2Source) { break; }
        }

        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        Id3V230StreamWriter wrt (pId3V2Source, m_bKeepOneValidImg); // OK if pId3V2Source is 0
        {
            string s;
            s = pMp3HandlerTagData->getData(TagReader::TITLE); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_TITLE()); } else { wrt.addTextFrame(KnownFrames::LBL_TITLE(), s); }
            s = pMp3HandlerTagData->getData(TagReader::ARTIST); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_ARTIST()); } else { wrt.addTextFrame(KnownFrames::LBL_ARTIST(), s); }
            s = pMp3HandlerTagData->getData(TagReader::TRACK_NUMBER); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_TRACK_NUMBER()); } else { wrt.addTextFrame(KnownFrames::LBL_TRACK_NUMBER(), s); }
            s = pMp3HandlerTagData->getData(TagReader::GENRE); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_GENRE()); } else { wrt.addTextFrame(KnownFrames::LBL_GENRE(), s); }
            s = pMp3HandlerTagData->getData(TagReader::ALBUM); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_ALBUM()); } else { wrt.addTextFrame(KnownFrames::LBL_ALBUM(), s); }
            s = pMp3HandlerTagData->getData(TagReader::COMPOSER); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_COMPOSER()); } else { wrt.addTextFrame(KnownFrames::LBL_COMPOSER(), s); }
            s = pMp3HandlerTagData->getData(TagReader::TIME); if (s.empty()) { wrt.removeFrames(KnownFrames::LBL_TIME_DATE_230()); wrt.removeFrames(KnownFrames::LBL_TIME_YEAR_230()); wrt.removeFrames(KnownFrames::LBL_TIME_240()); } else { wrt.setRecTime(TagTimestamp(s)); }

            s = pMp3HandlerTagData->getData(TagReader::IMAGE);
            if (s.empty())
            {
                wrt.removeFrames(KnownFrames::LBL_IMAGE(), Id3V2Frame::COVER);
            }
            else
            {
                int nImg (pMp3HandlerTagData->getImage());
                CB_ASSERT (nImg >= 0);
                const ImageColl& imgColl (m_pTagWriter->getImageColl());
                const ImageInfo& imgInfo (imgColl[nImg]);
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
                        QPixmap scaledPic;
                        ImageInfo::compress(imgInfo.getPixmap(), scaledPic, recomprImg);
                        nImgSize = recomprImg.size();
                        pImgData = recomprImg.data();
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
                *q++ = Id3V2Frame::COVER;
                *q++ = 0; // null-term descr
                memcpy(q, pImgData, nImgSize);
                wrt.addImage(frm);
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

        wrt.write(out); // may throw, but it will be caught

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));

            if (0 == pId3V2)
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

//ttt1 perhaps all licences in root dir

// based on configuration, either just saves the tags or asks the user for confirmation; returns true iff all tags have been saved or if none needed saving; it should be followed by a reload(), either for the current or for the next/prev album; if bExplicitCall is true, the "ASK" option is turned into "SAVE";
TagEditorDlgImpl::SaveOpt TagEditorDlgImpl::save(bool bExplicitCall)
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
                (bAssgn && (bExplicitCall || CommonData::DISCARD != m_pCommonData->m_eAssignSave)) ||
                (bNonId3V2 && (bExplicitCall || CommonData::DISCARD != m_pCommonData->m_eNonId3v2Save))
            )
        {
            vpHndlr.push_back(m_pTagWriter->m_vpMp3HandlerTagData[i]->getMp3Handler());
        }
        bHasUnsavedAssgn = bHasUnsavedAssgn || bAssgn;
        bHasUnsavedNonId3V2 = bHasUnsavedNonId3V2 || bNonId3V2;
    }
//ttt1 perhaps separate setting for showing and saving non-id3v2 fields

    if (vpHndlr.empty()) { return SAVED; }

    if (
            !bExplicitCall &&
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

    Id3V230Writer wrt (m_pTagWriter, m_pCommonData->m_bKeepOneValidImg);
    vector<Transformation*> vpTransf;
    vpTransf.push_back(&wrt);

    bool bRes (transform(vpHndlr, vpTransf, "Saving ID3V2.3.0 tags", this, m_pCommonData, m_transfConfig));
    m_pTagWriter->reloadAll(strCrt, TagWriter::CLEAR); //ttt1 don't clear
    //m_pTagWriter->updateAssigned(vector<pair<int, int> >());
    m_pAssgnBtnWrp->setState(m_pTagWriter->updateAssigned(vector<pair<int, int> >())); // we don't want to keep any previous value
    updateAssigned(); // needed for the "assign" button to work, because the previous line cleared m_pTagWriter->m_sSelOrigVal
    return bRes ? SAVED : PARTIALLY_SAVED;
}


//========================================================================================================================================================
//========================================================================================================================================================
//========================================================================================================================================================


/*static*/ QColor TagEditorDlgImpl::FILE_TAG_MISSING_COLOR (0xdddddd);
/*static*/ QColor TagEditorDlgImpl::FILE_NA_COLOR (0xf0f0ff);
/*static*/ QColor TagEditorDlgImpl::FILE_NO_DATA_COLOR (0xffffdd);


CurrentFileDelegate::CurrentFileDelegate(QTableView* pTableView) : QItemDelegate(pTableView), m_pTableView(pTableView)
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
        pPainter->fillRect(option.rect, QBrush(TagEditorDlgImpl::FILE_TAG_MISSING_COLOR));
    }
    else if ("\2" == s)
    { // not applicable
        pPainter->fillRect(option.rect, QBrush(TagEditorDlgImpl::FILE_NA_COLOR));
    }
    else if ("\3" == s)
    { // applicable, but no data found
        pPainter->fillRect(option.rect, QBrush(TagEditorDlgImpl::FILE_NO_DATA_COLOR));
    }
    else
    {
        QItemDelegate::paint(pPainter, option, index);
    }

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
    //QRect r (0, 0, nColWidth - 2*nMargin - 1, 10000); // !!! this "-1" is what's different from Qt's implementation //ttt1 see if this is fixed in 4.4 2008.30.06 - apparently it's not fixed and the workaround no longer works

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


/*static*/ QColor TagEditorDlgImpl::ALBFILE_NORM_COLOR;
/*static*/ QColor TagEditorDlgImpl::ALB_NONID3V2_COLOR (0xffffdd/*0xccffcc*/);
/*static*/ QColor TagEditorDlgImpl::ALB_ASSIGNED_COLOR (0xccccff);


CurrentAlbumDelegate::CurrentAlbumDelegate(QTableView* pTableView, TagEditorDlgImpl* pTagEditorDlgImpl) : QItemDelegate(pTableView), m_pTableView(pTableView), m_pTagEditorDlgImpl(pTagEditorDlgImpl), m_pTagWriter(pTagEditorDlgImpl->getTagWriter())
{
    CB_CHECK1 (0 != pTableView, std::runtime_error("NULL QTableView not allowed"));
    //connect(pTableView->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), pTableView, SLOT(resizeRowsToContents()));
}

/*override*/ void CurrentAlbumDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
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
    case Mp3HandlerTagData::ID3V2_VAL: col = TagEditorDlgImpl::ALBFILE_NORM_COLOR; break;

    case Mp3HandlerTagData::NON_ID3V2_VAL: col = TagEditorDlgImpl::ALB_NONID3V2_COLOR; break;

    case Mp3HandlerTagData::ASSIGNED: col = TagEditorDlgImpl::ALB_ASSIGNED_COLOR; break;
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
//ttt1 2009.04.06 - for all delegates: see if they can be removed by using the standard delegate and adding more functionality to the models; see Qt::ForegroundRole, Qt::TextAlignmentRole, Qt::FontRole & Co

/*override*/ QWidget* CurrentAlbumDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& style, const QModelIndex& index) const
{
    int nField (index.column());
    if (0 == nField || 7 == nField) { return 0; }
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

    if (0 != showMessage(m_pTableView, QMessageBox::Warning, 1, 1, "Warning", "You are editing data in a cell. If you proceed that change will be lost. Proceed and lose the data?", "Proceed", "Cancel")) { return false; } //ttt1 try and post somehow the content of the editor (?? perhaps send it a keyboard message that ENTER was pressed)
    //ttt1 review this

    delete *m_spEditors.begin();
    return true;
}


//ttt1 perhaps manufacture track numbers when pasting tables, if track numbers don't exist



//======================================================================================================================
//======================================================================================================================
//======================================================================================================================

//ttt0 paste single line should change single cell
