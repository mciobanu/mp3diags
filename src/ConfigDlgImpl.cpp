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
#include  <QTextCodec>
#include  <QSettings>
#include  <QFontDialog>
#include  <QColorDialog>
#include  <QPainter>
#include  <QStackedLayout>

#include  "ConfigDlgImpl.h"

#include  "StructuralTransformation.h"
#include  "Transformation.h"
#include  "OsFile.h"
#include  "Helpers.h"
#include  "Id3Transf.h"
#include  "Id3V230Stream.h"
#include  "CommonData.h"
#include  "StoredSettings.h"

////#include  <iostream> //ttt remove


using namespace std;
using namespace pearl;

//ttt2 simple / detailed files tab, with "simple" only allowing to "create backups in ..."
//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


class TransfListElem : public ListElem
{
    /*override*/ std::string getText(int nCol) const;
    const Transformation* m_pTransformation; // doesn't own the pointer
    //CommonData* m_pCommonData;
public:
    TransfListElem(const Transformation* pTransformation/*, CommonData* pCommonData*/) : m_pTransformation(pTransformation)/*, m_pCommonData(pCommonData)*/ {}
    const Transformation* getTransformation() const { return m_pTransformation; }
};

/*override*/ std::string TransfListElem::getText(int nCol) const
{
    if (0 == nCol) { return m_pTransformation->getActionName(); }
    return m_pTransformation->getDescription();
}


class CustomTransfListPainter : public ListPainter
{
    /*override*/ int getColCount() const { return 2; }
    /*override*/ std::string getColTitle(int nCol) const { return 0 == nCol ? "Action" : "Description"; }
    /*override*/ void getColor(int /*nIndex*/, int /*nColumn*/, bool /*bSubList*/, QColor& /*bckgColor*/, QColor& /*penColor*/, double& /*dGradStart*/, double& /*dGradEnd*/) const { }
    /*override*/ int getColWidth(int /*nCol*/) const { return -1; } // positive values are used for fixed widths, while negative ones are for "stretched"
    /*override*/ int getHdrHeight() const { return CELL_HEIGHT; }
    /*override*/ Qt::Alignment getAlignment(int /*nCol*/) const { return Qt::AlignTop | Qt::AlignLeft; }
    /*override*/ std::string getTooltip(TooltipKey eTooltipKey) const;
    /*override*/ void reset();

    const SubList& m_vDefaultSel; // to be used by reset()
public:
    CustomTransfListPainter(const CommonData* pCommonData, const SubList& vOrigSel, const SubList& vSel, const SubList& vDefaultSel);
    ~CustomTransfListPainter();
};



CustomTransfListPainter::CustomTransfListPainter(const CommonData* pCommonData, const SubList& vOrigSel, const SubList& vSel, const SubList& vDefaultSel) : ListPainter(""), m_vDefaultSel(vDefaultSel)
{
//qDebug("init CustomTransfListPainter with origsel %d and sel %d", cSize(vOrigSel), cSize(vSel));
    for (int i = 0, n = cSize(pCommonData->getAllTransf()); i < n; ++i)
    {
        m_vpOrigAll.push_back(new TransfListElem(pCommonData->getAllTransf()[i]));
    }
    m_vpResetAll = m_vpOrigAll; // !!! no new pointers
    m_vOrigSel = vOrigSel;
    m_vSel = vSel;
}


CustomTransfListPainter::~CustomTransfListPainter()
{
    clearPtrContainer(m_vpOrigAll);
}


/*override*/ string CustomTransfListPainter::getTooltip(TooltipKey eTooltipKey) const
{
    switch (eTooltipKey)
    {
    case SELECTED_G: return "";//"Notes to be included";
    case AVAILABLE_G: return "";//"Available notes";
    case ADD_B: return "Add selected transformation(s)";
    case DELETE_B: return "Remove selected transformation(s)";
    case ADD_ALL_B: return "";//"Add all transformations";
    case DELETE_ALL_B: return "";//"Remove all transformations";
    case RESTORE_DEFAULT_B: return "Restore current list to its default value";
    case RESTORE_OPEN_B: return "Restore current list to the configuration it had when the window was open";
    default: CB_ASSERT(false);
    }
}


/*override*/ void CustomTransfListPainter::reset()
{
    m_vSel = m_vDefaultSel;
}

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


class VisibleTransfPainter : public ListPainter
{
    /*override*/ int getColCount() const { return 2; }
    /*override*/ std::string getColTitle(int nCol) const { return 0 == nCol ? "Action" : "Description"; }
    /*override*/ void getColor(int /*nIndex*/, int /*nColumn*/, bool /*bSubList*/, QColor& /*bckgColor*/, QColor& /*penColor*/, double& /*dGradStart*/, double& /*dGradEnd*/) const { }
    /*override*/ int getColWidth(int /*nCol*/) const { return -1; } // positive values are used for fixed widths, while negative ones are for "stretched"
    /*override*/ int getHdrHeight() const { return CELL_HEIGHT; }
    /*override*/ Qt::Alignment getAlignment(int /*nCol*/) const { return Qt::AlignTop | Qt::AlignLeft; }
    /*override*/ std::string getTooltip(TooltipKey eTooltipKey) const;
    /*override*/ void reset();

    const SubList& m_vDefaultSel; // to be used by reset()

public:
    VisibleTransfPainter(const CommonData* pCommonData, const SubList& vOrigSel, const SubList& vSel, const SubList& vDefaultSel);
    ~VisibleTransfPainter();
};



VisibleTransfPainter::VisibleTransfPainter(const CommonData* pCommonData, const SubList& vOrigSel, const SubList& vSel, const SubList& vDefaultSel) : ListPainter(""), m_vDefaultSel(vDefaultSel)
{
    const vector<Transformation*>& v (pCommonData->getAllTransf());
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        const Transformation* p (v[i]);
        m_vpOrigAll.push_back(new TransfListElem(p));
    }

    m_vpResetAll = m_vpOrigAll; // !!! no new pointers
    m_vOrigSel = vOrigSel;
    m_vSel = vSel;
}


VisibleTransfPainter::~VisibleTransfPainter()
{
    clearPtrContainer(m_vpOrigAll);
}


/*override*/ string VisibleTransfPainter::getTooltip(TooltipKey eTooltipKey) const
{
    switch (eTooltipKey)
    {
    case SELECTED_G: return "";//"Notes to be included";
    case AVAILABLE_G: return "";//"Available notes";
    case ADD_B: return "Add selected transformation(s)";
    case DELETE_B: return "Remove selected transformation(s)";
    case ADD_ALL_B: return "";//"Add all transformations";
    case DELETE_ALL_B: return "";//"Remove all transformations";
    case RESTORE_DEFAULT_B: return "Restore current list to its default value";
    case RESTORE_OPEN_B: return "Restore current list to the configuration it had when the window was open";
    default: CB_ASSERT(false);
    }
}


/*override*/ void VisibleTransfPainter::reset()
{
    m_vSel = m_vDefaultSel;
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


namespace BpsInfo
{
    int s_aValidBps[] = { 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 288, 320, 10000 };
    //int s_aValidBps[] = { 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 10000 };
    int getIndex(int nVal)
    {
        if (nVal >= 10000) { return 0; }
        int i (0);
        for (; s_aValidBps[i] < nVal; ++i) {}
        if (10000 == s_aValidBps[i]) { return i - 1; }
        return i;
    }

    const QStringList& getValueList()
    {
        static QStringList lst;
        if (lst.isEmpty())
        {
            for (int i = 0; ; ++i)
            {
                if (10000 == s_aValidBps[i]) { break; }
                lst << QString::number(s_aValidBps[i]);
            }
        }

        return lst;
    }
} // namespace BpsInfo


//=====================================================================================================================

/*
class EventFilter : public QObject
{
    bool eventFilter(QObject* pObj, QEvent* pEvent)
    {
        if (QEvent::Paint != pEvent->type())
        {
            qDebug("%s %d", pObj->objectName().toUtf8().constData(), (int)pEvent->type());
        }

        static bool b (true);
        if (QEvent::Hide == pEvent->type())
        {
            b = !b;
            if (b)
            {
                qDebug("not allowed");
                pEvent->ignore();
                return true;
            }
        }
        return QObject::eventFilter(pObj, pEvent);
    }
public:
    EventFilter(QObject* pParent) : QObject(pParent) {}
};
*/


ConfigDlgImpl::ConfigDlgImpl(TransfConfig& transfCfg, CommonData* pCommonData, QWidget* pParent, bool bFull) :
        QDialog(pParent, getDialogWndFlags()),
        Ui::ConfigDlg(),
        NoteListPainterBase(pCommonData, "<all notes>"),
        m_transfCfg(transfCfg),

        m_pCommonData(pCommonData),
        m_pCustomTransfListPainter(0),
        m_pCustomTransfDoubleList(0),
        m_vvnCustomTransf(pCommonData->getCustomTransf()),
        m_nCurrentTransf(-1),
        m_vvnDefaultCustomTransf(CUSTOM_TRANSF_CNT),
        m_pVisibleTransfPainter(0),
        m_vnVisibleTransf(pCommonData->getVisibleTransf())
{
    setupUi(this);

#ifdef DISABLE_CHECK_FOR_UPDATES
    m_pCheckForUpdatesCkB->hide();
#endif

    if (!bFull)
    {
        m_pMainTabWidget->removeTab(6);
        m_pMainTabWidget->removeTab(5);
        m_pMainTabWidget->removeTab(4);
        m_pMainTabWidget->removeTab(2);
        m_pMainTabWidget->removeTab(1);
        m_pLocaleGrp->hide();
        m_pCaseGrp->hide();
        //m_pRenameW->hide();
        m_pOthersMiscGrp->hide();
        //m_pScanAtStartupCkB->hide();
        //m_pShowDebugCkB->hide();
        //m_pShowSessCkB->hide();
        //m_pIconConfW->hide();
        //m_pFontsW->hide();
        m_pNormalizeGrp->hide();
        m_pRenamerGrp->hide();
    }

    m_pSourceDirF->hide();

    int nWidth, nHeight;
    m_pCommonData->m_settings.loadConfigSize(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 400)
    {
        resize(nWidth, nHeight);
    }
    else
    {
        defaultResize(*this);
    }


    { // set text for ID3V2 codepage test
        QString qstr;
        const Mp3Handler* p (m_pCommonData->getCrtMp3Handler());
        if (0 != p)
        {
            const vector<DataStream*>& vpStreams (p->getStreams());
            for (int i = 0, n = cSize(vpStreams); i < n; ++i)
            {
                DataStream* p (vpStreams[i]);
                Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));

                if (0 != pId3V2)
                {
                    const vector<Id3V2Frame*>& vpFrames (pId3V2->getFrames());
                    for (int i = 0, n = cSize(vpFrames); i < n; ++i)
                    {
                        const Id3V2Frame* pFrm (vpFrames[i]);
                        if ('T' == pFrm->m_szName[0] && pFrm->m_bHasLatin1NonAscii)
                        {
                            if (!qstr.isEmpty())
                            {
                                qstr += '\n';
                            }
                            qstr += convStr(pFrm->getUtf8String());
                        }
                    }
                }
            }
        }

        if (qstr.isEmpty())
        {
            qstr = "If you don't know exactly what codepage you want, it's better to make current a file having an ID3V2 tag that contains text frames using the Latin-1 encoding and having non-ASCII characters. Then the content of those frames will replace this text, allowing you to decide which codepage is a match for your file.";
        }

        m_codepageTestText = qstr.toLatin1();
    }



    /*EventFilter* pEventFilter (new EventFilter(this));
    tab->installEventFilter(pEventFilter);
    //tab_3->installEventFilter(pEventFilter);
    */

    {

        {
            delete m_pRemovableL;

            delete m_pFileSettingsW->layout();
            m_pFileSettingsLayout = new QStackedLayout(m_pFileSettingsW);
            //m_pFileSettingsLayout->setContentsMargins(0, 50, 50, 0);
            //m_pFileSettingsW->setLayout(m_pFileSettingsLayout);
            m_pFileSettingsLayout->addWidget(m_pSimpleViewTab);
            m_pFileSettingsLayout->addWidget(m_pFullViewTab);

            delete m_pDetailsTabWidget;
        }

        m_pPODestE->setText(toNativeSeparators(convStr(getSepTerminatedDir(transfCfg.getProcOrigDir()))));
        m_pPODest2E->setText(m_pPODestE->text());
        m_pUODestE->setText(toNativeSeparators(convStr(getSepTerminatedDir(transfCfg.getUnprocOrigDir()))));
        m_pProcDestE->setText(toNativeSeparators(convStr(getSepTerminatedDir(transfCfg.getProcessedDir()))));
        m_pTempDestE->setText(toNativeSeparators(convStr(getSepTerminatedDir(transfCfg.getTempDir()))));
        m_pCompDestE->setText(toNativeSeparators(convStr(getSepTerminatedDir(transfCfg.getCompDir()))));

        m_pSrcDirE->setText(toNativeSeparators(convStr(getSepTerminatedDir(transfCfg.getSrcDir()))));

        //setSimpleViewOpt(transfCfg.m_options); //!!! pointless; doesn't work if keepTime is true; anyway, the on_m_pSimpleViewB_clicked() call below will select the right radiobutton
        setFullViewOpt(transfCfg.m_options); // !!! the buttons should be set for both views
        setSimpleViewOpt(transfCfg.m_options);

        TransfConfig::Options opt (transfCfg.m_options);

        if (opt.asBackup() == opt || opt.asNonBackup() == opt)
        {
            m_pSimpleViewB->setChecked(true);
            on_m_pSimpleViewB_clicked();
        }
        else
        {
            m_pFullViewB->setChecked(true);
            on_m_pFullViewB_clicked();
        }


        m_pKeepOrigTimeCkB->setChecked(transfCfg.m_options.m_bKeepOrigTime);
    }



    // -------------------------------------------- ignored ------------------------------------------------------

    m_pIgnoredNotesListHldr->setLayout(new QHBoxLayout());

    const vector<const Note*>& vpAllNotes (Notes::getAllNotes());
    for (int i = 0, n = cSize(vpAllNotes); i < n; ++i)
    {
        m_vpOrigAll.push_back(new NoteListElem(vpAllNotes[i], m_pCommonData));
    }

    m_vOrigSel = pCommonData->getIgnoredNotes();
    //cout << "constr:\n"; printContainer(pCommonData->m_vnIgnoredNotes, cout);

    m_vSel = m_vOrigSel;

    m_pDoubleList = new DoubleList(
            *this,
            DoubleList::ADD_ALL | DoubleList::DEL_ALL | DoubleList::RESTORE_OPEN | DoubleList::RESTORE_DEFAULT,
            DoubleList::SINGLE_UNSORTABLE,
            "Other notes",
            "Ignore notes",
            this);

    m_pIgnoredNotesListHldr->layout()->addWidget(m_pDoubleList);
    m_pIgnoredNotesListHldr->layout()->setContentsMargins(0, 0, 0, 0);


    // -------------------------------------------- custom transformations ------------------------------------------------------

    m_pTransfListHldr->setLayout(new QHBoxLayout());
    m_pTransfListHldr->layout()->setContentsMargins(0, 0, 0, 0);

    m_vpTransfButtons.push_back(m_pCustomTransform1B);
    m_vpTransfButtons.push_back(m_pCustomTransform2B);
    m_vpTransfButtons.push_back(m_pCustomTransform3B);
    m_vpTransfButtons.push_back(m_pCustomTransform4B); // CUSTOM_TRANSF_CNT

    m_defaultPalette = m_pCustomTransform1T->palette();
    m_wndPalette = m_pCustomTransform1T->palette();

    m_wndPalette.setColor(QPalette::Base, m_wndPalette.color(QPalette::Disabled, QPalette::Window));
    m_vpTransfLabels.push_back(m_pCustomTransform1T);
    m_vpTransfLabels.push_back(m_pCustomTransform2T);
    m_vpTransfLabels.push_back(m_pCustomTransform3T);
    m_vpTransfLabels.push_back(m_pCustomTransform4T); // CUSTOM_TRANSF_CNT

    for (int i = 0; i < CUSTOM_TRANSF_CNT; ++i)
    {
        m_vpTransfLabels[i]->setPalette(m_wndPalette);
        refreshTransfText(i);
        initDefaultCustomTransf(i, m_vvnDefaultCustomTransf, m_pCommonData);
    }

    selectCustomTransf(0);

    const QStringList& freqLst (BpsInfo::getValueList());

    { m_pQualStCbrCbB->insertItems(0, freqLst); m_pQualStCbrCbB->setCurrentIndex(BpsInfo::getIndex(m_pCommonData->getQualThresholds().m_nStereoCbr/1000)); }
    { m_pQualJtStCbrCbB->insertItems(0, freqLst); m_pQualJtStCbrCbB->setCurrentIndex(BpsInfo::getIndex(m_pCommonData->getQualThresholds().m_nJointStereoCbr/1000)); }
    { m_pQualDlChCbrCbB->insertItems(0, freqLst); m_pQualDlChCbrCbB->setCurrentIndex(BpsInfo::getIndex(m_pCommonData->getQualThresholds().m_nDoubleChannelCbr/1000)); }

    // ttt2 try and create some custom widget based on QSpinBox for CBR, so only valid frequencies can be chosen;

    m_pQualStVbrSB->setValue(m_pCommonData->getQualThresholds().m_nStereoVbr/1000);
    m_pQualJtStVbrSB->setValue(m_pCommonData->getQualThresholds().m_nJointStereoVbr/1000);
    m_pQualDlChVbrSB->setValue(m_pCommonData->getQualThresholds().m_nDoubleChannelVbr/1000);

    { // locale
        QStringList lNames;
        set<QString> s;
//set<QString> s1;
        QList<QByteArray> l (QTextCodec::availableCodecs());
        for (int i = 0, n = l.size(); i < n; ++i)
        {
            //lNames << QString::fromLatin1(l[i]);
            //lNames << QTextCodec::codecForName(l[i])->name();
            s.insert(QTextCodec::codecForName(l[i])->name()); // a codec is known by several names; by doing this we eliminate redundant names and make the list a lot smaller
            //s.insert(QTextCodec::codecForName(l[i])->name() + QString::fromLatin1(l[i]));
//qDebug("%s", (QString::fromLatin1(l[i]) + "  /  " + QTextCodec::codecForName(l[i])->name()).toUtf8().data());
//s1.insert(QTextCodec::codecForName(l[i])->name() + "  /  " + QString::fromLatin1(l[i]));
        }

        for (set<QString>::const_iterator it = s.begin(); it != s.end(); ++it)
        {
            lNames << *it;
        }
//for (set<QString>::const_iterator it = s1.begin(); it != s1.end(); ++it) { qDebug("%s", it->toUtf8().data()); }

//lNames.sort();

        m_pLocaleCbB->addItems(lNames);
        int n (m_pLocaleCbB->findText(m_pCommonData->m_locale));
        if (-1 == n) { n = 0; }
        m_pLocaleCbB->setCurrentIndex(n);
    }

    { // case
        QStringList lNames;
        /*lNames << "Lower case: first part. second part.";
        lNames << "Upper case: FIRST PART. SECOND PART.";
        lNames << "Title case: First Part. Second Part.";
        lNames << "Sentence case: First part. Second part.";*/ // ttt2 perhaps put this back; as of 2009.10.15, "." is no longer supported as a sentence ending
        lNames << "lower case";
        lNames << "UPPER CASE";
        lNames << "Title Case";
        lNames << "Sentence case";

        m_pArtistsCaseCbB->addItems(lNames);
        m_pOthersCaseCbB->addItems(lNames);
        m_pArtistsCaseCbB->setCurrentIndex((int)m_pCommonData->m_eCaseForArtists);
        m_pOthersCaseCbB->setCurrentIndex((int)m_pCommonData->m_eCaseForOthers);
    }

    { // colors
        m_vpColButtons.push_back(m_pCol0B);
        m_vpColButtons.push_back(m_pCol1B);
        m_vpColButtons.push_back(m_pCol2B);
        m_vpColButtons.push_back(m_pCol3B);
        m_vpColButtons.push_back(m_pCol4B);
        m_vpColButtons.push_back(m_pCol5B);
        m_vpColButtons.push_back(m_pCol6B);
        m_vpColButtons.push_back(m_pCol7B);
        m_vpColButtons.push_back(m_pCol8B);
        m_vpColButtons.push_back(m_pCol9B);
        m_vpColButtons.push_back(m_pCol10B);
        m_vpColButtons.push_back(m_pCol11B);
        m_vpColButtons.push_back(m_pCol12B);
        m_vpColButtons.push_back(m_pCol13B);

        m_vNoteCategColors = m_pCommonData->m_vNoteCategColors;

        for (int i = 0; i < cSize(m_vpColButtons); ++i)
        {
            setBtnColor(i);
        }
    }

    { // tag editor
        m_pWarnOnNonSeqTracksCkB->setChecked(m_pCommonData->m_bWarnOnNonSeqTracks);
        m_pWarnOnPasteToNonSeqTracksCkB->setChecked(m_pCommonData->m_bWarnPastingToNonSeqTracks);
    }

    {  //misc
        m_pScanAtStartupCkB->setChecked(m_pCommonData->m_bScanAtStartup);
        m_pFastSaveCkB->setChecked(m_pCommonData->useFastSave());
        m_pShowExportCkB->setChecked(m_pCommonData->m_bShowExport);
        m_pShowDebugCkB->setChecked(m_pCommonData->m_bShowDebug);
        m_pShowSessCkB->setChecked(m_pCommonData->m_bShowSessions);
        m_pNormalizerE->setText(convStr(m_pCommonData->m_strNormalizeCmd));
        m_pKeepNormOpenCkB->setChecked(m_pCommonData->m_bKeepNormWndOpen);

        switch (m_pCommonData->m_eAssignSave)
        {
        case CommonData::SAVE: m_pAssgnSaveRB->setChecked(true); break;
        case CommonData::DISCARD: m_pAssgnDiscardRB->setChecked(true); break;
        case CommonData::ASK: m_pAssgnAskRB->setChecked(true); break;
        default: CB_ASSERT (false);
        }

        switch (m_pCommonData->m_eNonId3v2Save)
        {
        case CommonData::SAVE: m_pNonId3v2SaveRB->setChecked(true); break;
        case CommonData::DISCARD: m_pNonId3v2DiscardRB->setChecked(true); break;
        case CommonData::ASK: m_pNonId3v2AskRB->setChecked(true); break;
        default: CB_ASSERT (false);
        }

        m_pIconSizeSB->setValue(m_pCommonData->m_nMainWndIconSize);
        m_pAutoSizeIconsCkB->setChecked(m_pCommonData->m_bAutoSizeIcons);
        m_pKeepOneValidImgCkB->setChecked(m_pCommonData->m_bKeepOneValidImg);
        m_pWmpCkB->setChecked(m_pCommonData->m_bWmpVarArtists);
        m_pItunesCkB->setChecked(m_pCommonData->m_bItunesVarArtists);
        m_pMaxImgSizeSB->setValue(ImageInfo::MAX_IMAGE_SIZE/1024);
        m_pTraceToFileCkB->setChecked(m_pCommonData->isTraceToFileEnabled());

        m_pInvalidCharsE->setText(convStr(m_pCommonData->m_strRenamerInvalidChars));
        m_pInvalidReplacementE->setText(convStr(m_pCommonData->m_strRenamerReplacementString));

        m_pCheckForUpdatesCkB->setChecked("yes" == m_pCommonData->m_strCheckForNewVersions);

        m_generalFont = m_pCommonData->getNewGeneralFont();
        m_pDecrLabelFontSB->setValue(m_pCommonData->getLabelFontSizeDecr());
        m_fixedFont = m_pCommonData->getNewFixedFont();
        setFontLabels();
    }

    {
        initDefaultVisibleTransf(m_vnDefaultVisibleTransf, m_pCommonData);

        m_pVisibleTransfPainter = new VisibleTransfPainter(m_pCommonData, m_pCommonData->getVisibleTransf(), m_vnVisibleTransf, m_vnDefaultVisibleTransf);
        m_pVisibleTransfDoubleList = new DoubleList(
                *m_pVisibleTransfPainter,
                DoubleList::RESTORE_OPEN | DoubleList::RESTORE_DEFAULT,
                DoubleList::SINGLE_SORTABLE,
                "Invisible transformations",
                "Visible transformations",
                this);

        m_pVisibleTransformsHndlr->setLayout(new QHBoxLayout());
        m_pVisibleTransformsHndlr->layout()->setContentsMargins(0, 0, 0, 0);

        m_pVisibleTransformsHndlr->layout()->addWidget(m_pVisibleTransfDoubleList);
    }

    m_pSrcDirE->setFocus();

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }

    m_pInvalidCharsE->setToolTip("Characters in this list get replaced with the string below, in \"Replace with\"\n\n"
        "An underlined font is used to allow spaces to be seen");
    m_pInvalidReplacementE->setToolTip("This string replaces invalid characters in the file renamer\"\n\n"
        "An underlined font is used to allow spaces to be seen");

    {
        m_pShellTempSessCkB->setChecked(ShellIntegration::isTempSessionEnabled());
        m_pShellVisibleSessCkB->setChecked(ShellIntegration::isVisibleSessionEnabled());
        m_pShellHiddenSessCkB->setChecked(ShellIntegration::isHiddenSessionEnabled());

        bool b (ShellIntegration::isShellIntegrationEditable());
        m_pShellTempSessCkB->setEnabled(b);
        m_pShellVisibleSessCkB->setEnabled(b);
        m_pShellHiddenSessCkB->setEnabled(b);

        string strShellErr (ShellIntegration::getShellIntegrationError());

        m_pShellErrorL->setText(convStr(strShellErr));
    }
}


void ConfigDlgImpl::setBtnColor(int n)
{
//    QPalette pal (m_vpColButtons[n]->palette());
    //QPalette pal (m_pCol0B->palette());
/*    pal.setBrush(QPalette::Button, m_vNoteCategColors[n]);
    pal.setBrush(QPalette::Window, m_vNoteCategColors[n]);
    pal.setBrush(QPalette::Midlight, QColor(255, 0, 0));
    pal.setBrush(QPalette::Dark, QColor(255, 0, 0));
    pal.setBrush(QPalette::Mid, QColor(255, 0, 0));
    pal.setBrush(QPalette::Shadow, QColor(255, 0, 0));*/
    //m_vpColButtons[n]->setPalette(pal);

    int f (QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, m_vpColButtons.at(n)) + 2); //ttt2 hard-coded "2"
    int w (m_vpColButtons[n]->width() - f), h (m_vpColButtons[n]->height() - f);
    QPixmap pic (w, h);
    QPainter pntr (&pic);
    pntr.fillRect(0, 0, w, h, m_vNoteCategColors.at(n));

    m_vpColButtons[n]->setIcon(pic);
    m_vpColButtons[n]->setIconSize(QSize(w, h));
}

void ConfigDlgImpl::onButtonClicked(int n)
{
    QColor c (QColorDialog::getColor(m_vNoteCategColors.at(n), this));
    if (!c.isValid()) { return; }
    m_vNoteCategColors[n] = c;
    setBtnColor(n);
}


void ConfigDlgImpl::on_m_pResetColorsB_clicked()
{
    QColor c (getDefaultBkgCol());
    for (int i = 0; i < cSize(m_vNoteCategColors) - 1; ++i) // !!! "-1" because there is no configuration for CUSTOM colors
    {
        m_vNoteCategColors[i] = c;
        setBtnColor(i);
    }
}

void SessionSettings::saveTransfConfig(const TransfConfig& transfConfig)
{
    m_pSettings->remove("transformation");
    m_pSettings->setValue("transformation/srcDir",          convStr(transfConfig.getSrcDir()));
    m_pSettings->setValue("transformation/procOrigDir",     convStr(transfConfig.getProcOrigDir()));
    m_pSettings->setValue("transformation/unprocOrigDir",   convStr(transfConfig.getUnprocOrigDir()));
    m_pSettings->setValue("transformation/processedDir",    convStr(transfConfig.getProcessedDir()));
    m_pSettings->setValue("transformation/tempDir",         convStr(transfConfig.getTempDir()));
    m_pSettings->setValue("transformation/compDir",         convStr(transfConfig.getCompDir()));

    m_pSettings->setValue("transformation/options",     transfConfig.getOptions());

    /*if (transfConfig.getOptions() != 14349) //ttt remove
    {
        qDebug("cfg changed: %x", transfConfig.getOptions());
    }*/
}

// returns false if there was some error while loading (so the user can be told about defaults being used and those defaults could get saved)
bool SessionSettings::loadTransfConfig(TransfConfig& transfConfig) const
{
    try
    {
        //TransfConfig tc ("/r/temp/1", "/r/temp/1", "/r/temp/1/proc", "/r/temp/1/temp_mp3");
        TransfConfig tc (
                convStr(m_pSettings->value("transformation/srcDir", "*").toString()),
                convStr(m_pSettings->value("transformation/procOrigDir", "*").toString()),
                convStr(m_pSettings->value("transformation/unprocOrigDir", "*").toString()),
                convStr(m_pSettings->value("transformation/processedDir", "*").toString()),
                convStr(m_pSettings->value("transformation/tempDir", "*").toString()),
                convStr(m_pSettings->value("transformation/compDir", "*").toString()),
                m_pSettings->value("transformation/options", -1).toInt()
            );
        transfConfig = tc;
        return !tc.hadInitError();
    }
    catch (const IncorrectDirName&)
    {
        TransfConfig tc ("*", "*", "*", "*", "*", "*", -1);
        transfConfig = tc;
        return false;
    }
}



bool ConfigDlgImpl::run()
{
    TRACER("ConfigDlgImpl::run()");
    if (QDialog::Accepted != exec()) { return false; }
    m_pCommonData->m_settings.saveConfigSize(width(), height());
    return true;
}


void ConfigDlgImpl::on_m_pLocaleCbB_currentIndexChanged(int)
{
    QTextCodec* pCodec (QTextCodec::codecForName(m_pLocaleCbB->currentText().toLatin1()));
    CB_ASSERT (0 != pCodec);
    QString qstrTxt (pCodec->toUnicode(m_codepageTestText));
    m_pCodepageTestM->setText(qstrTxt);
}


//ttt2 treat "orig" and "proc" files as "orig" when processing them


void initDefaultCustomTransf(int k, vector<vector<int> >& vv, CommonData* pCommonData)
{
    vector<int>& v (vv[k]);
    switch (k)
    {
    case 0:
        v.push_back(pCommonData->getTransfPos(SingleBitRepairer::getClassName()));
        v.push_back(pCommonData->getTransfPos(InnerNonAudioRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(TruncatedMpegDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(VbrRepairer::getClassName()));
        v.push_back(pCommonData->getTransfPos(MismatchedXingRemover::getClassName())); // !!! takes care of broken Xing headers for CBR files (VbrRepairer only deals with VBR files)
        break;

    case 1:
        v.push_back(pCommonData->getTransfPos(Id3V2Rescuer::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnsupportedId3V2Remover::getClassName()));
        //v.push_back(pCommonData->getTransfPos(MultipleId3StreamRemover::getClassName()));
        //v.push_back(pCommonData->getTransfPos(Id3V1ToId3V2Copier::getClassName()));
        break;

    case 2:
        v.push_back(pCommonData->getTransfPos(InnerNonAudioRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnknownDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(BrokenDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnsupportedDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(TruncatedMpegDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(NullStreamRemover::getClassName()));
        //v.push_back(pCommonData->getTransfPos(MultipleId3StreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(MismatchedXingRemover::getClassName()));
        break;

    case 3: // CUSTOM_TRANSF_CNT
        v.push_back(pCommonData->getTransfPos(SingleBitRepairer::getClassName()));
        v.push_back(pCommonData->getTransfPos(InnerNonAudioRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(TruncatedMpegDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(VbrRepairer::getClassName()));
        v.push_back(pCommonData->getTransfPos(MismatchedXingRemover::getClassName())); // !!! takes care of broken Xing headers for CBR files (VbrRepairer only deals with VBR files)

        v.push_back(pCommonData->getTransfPos(Id3V2Rescuer::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnsupportedId3V2Remover::getClassName()));
        v.push_back(pCommonData->getTransfPos(MultipleId3StreamRemover::getClassName()));
        //v.push_back(pCommonData->getTransfPos(Id3V1ToId3V2Copier::getClassName()));

        //v.push_back(pCommonData->getTransfPos(InnerNonAudioRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnknownDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(BrokenDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnsupportedDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(TruncatedMpegDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(NullStreamRemover::getClassName()));
        //v.push_back(pCommonData->getTransfPos(MultipleId3StreamRemover::getClassName()));
        //v.push_back(pCommonData->getTransfPos(MismatchedXingRemover::getClassName()));
        break;

    default:
        CB_ASSERT (false);
    }
}


void initDefaultVisibleTransf(vector<int>& v, CommonData* pCommonData)
{
    v.push_back(pCommonData->getTransfPos(SingleBitRepairer::getClassName()));
    v.push_back(pCommonData->getTransfPos(InnerNonAudioRemover::getClassName()));

    v.push_back(pCommonData->getTransfPos(UnknownDataStreamRemover::getClassName()));
    v.push_back(pCommonData->getTransfPos(BrokenDataStreamRemover::getClassName()));
    v.push_back(pCommonData->getTransfPos(UnsupportedDataStreamRemover::getClassName()));
    v.push_back(pCommonData->getTransfPos(TruncatedMpegDataStreamRemover::getClassName()));
    v.push_back(pCommonData->getTransfPos(NullStreamRemover::getClassName()));

    v.push_back(pCommonData->getTransfPos(BrokenId3V2Remover::getClassName()));
    v.push_back(pCommonData->getTransfPos(UnsupportedId3V2Remover::getClassName()));

    //v.push_back(pCommonData->getTransfPos(IdentityTransformation::getClassName()));

    v.push_back(pCommonData->getTransfPos(MultipleId3StreamRemover::getClassName()));
    v.push_back(pCommonData->getTransfPos(MismatchedXingRemover::getClassName()));

    v.push_back(pCommonData->getTransfPos(TruncatedAudioPadder::getClassName()));

    v.push_back(pCommonData->getTransfPos(VbrRepairer::getClassName()));
    v.push_back(pCommonData->getTransfPos(VbrRebuilder::getClassName()));

    v.push_back(pCommonData->getTransfPos(Id3V2Cleaner::getClassName()));
    v.push_back(pCommonData->getTransfPos(Id3V2Rescuer::getClassName()));
    v.push_back(pCommonData->getTransfPos(Id3V2UnicodeTransformer::getClassName()));
    //v.push_back(pCommonData->getTransfPos(Id3V2CaseTransformer::getClassName()));


    //v.push_back(pCommonData->getTransfPos(Id3V2ComposerAdder::getClassName()));
    //v.push_back(pCommonData->getTransfPos(Id3V2ComposerRemover::getClassName()));
    //v.push_back(pCommonData->getTransfPos(Id3V2ComposerCopier::getClassName()));

    //v.push_back(pCommonData->getTransfPos(SmallerImageRemover::getClassName()));
    //v.push_back(pCommonData->getTransfPos(Id3V1ToId3V2Copier::getClassName()));
    //v.push_back(pCommonData->getTransfPos(Id3V1Remover::getClassName()));

    //v.push_back(pCommonData->getTransfPos(Id3V2Compactor::getClassName()));
    //v.push_back(pCommonData->getTransfPos(Id3V2Expander::getClassName()));
}


ConfigDlgImpl::~ConfigDlgImpl()
{
    clearPtrContainer(m_vpOrigAll);
    clearPtrContainer(m_vpResetAll); // doesn't matter if it was used or not, or if m_bResultInReset is true or false
    delete m_pVisibleTransfPainter;
    delete m_pCustomTransfListPainter;
}


void ConfigDlgImpl::refreshTransfText(int k)
{
    QString s;
    for (int i = 0, n = cSize(m_vvnCustomTransf[k]); i < n; ++i)
    {
        s += m_pCommonData->getAllTransf()[m_vvnCustomTransf[k][i]]->getActionName();
        if (i < n - 1) { s += "\n"; }
    }
    m_vpTransfLabels[k]->setText(s);
}


void ConfigDlgImpl::selectCustomTransf(int k) // 0 <= k <= 2
{
    getTransfData();
    delete m_pCustomTransfDoubleList;
    delete m_pCustomTransfListPainter;
    m_pCustomTransfListPainter = new CustomTransfListPainter(m_pCommonData, m_pCommonData->getCustomTransf()[k], m_vvnCustomTransf[k], m_vvnDefaultCustomTransf[k]);

    m_pCustomTransfDoubleList = new DoubleList(
            *m_pCustomTransfListPainter,
            DoubleList::RESTORE_OPEN | DoubleList::RESTORE_DEFAULT,
            DoubleList::MULTIPLE,
            "All transformations",
            "Used transformations",
            this);

    m_pTransfListHldr->layout()->addWidget(m_pCustomTransfDoubleList);
    //m_pTransfListHldr->layout()->activate();
    //?->300x358//m_pCustomTransfDoubleList->layout()->activate();
    //m_pCustomTransfDoubleList->resizeOnShow();
    //m_pDetailsTabWidget->setCurrentIndex(0);
    //m_pDetailsTabWidget->setCurrentIndex(2);

    for (int i = 0; i < CUSTOM_TRANSF_CNT; ++i)
    {
        m_vpTransfButtons[i]->setChecked(false); m_vpTransfLabels[i]->setPalette(m_wndPalette);
    }

    m_vpTransfButtons[k]->setChecked(true); m_vpTransfLabels[k]->setPalette(m_defaultPalette);
    m_nCurrentTransf = k;

    connect(m_pCustomTransfDoubleList, SIGNAL(dataChanged()), this, SLOT(onTransfDataChanged()));
}





void ConfigDlgImpl::onTransfDataChanged()
{
    getTransfData();
}


void ConfigDlgImpl::getTransfData()
{
    if (-1 == m_nCurrentTransf) { return; }
    m_vvnCustomTransf[m_nCurrentTransf] = m_pCustomTransfListPainter->getSel();
//qDebug("transf %d at %d", cSize(m_vvnCustomTransf[m_nCurrentTransf]), m_nCurrentTransf);
    refreshTransfText(m_nCurrentTransf);
}


void ConfigDlgImpl::on_m_pOkB_clicked()
{
    {
        string strInv (convStr(m_pInvalidCharsE->text()));
        string strRepl (convStr(m_pInvalidReplacementE->text()));
        if (!strInv.empty())
        {
            string::size_type n (strRepl.find_first_of(strInv));
            if (string::npos != n)
            {
                QMessageBox::critical(this, "Error", QString("You can't have '") + strRepl[n] + "' in both the list of invalid characters and the string that invalid characters are replaced with.");
                return;
            }
        }
    }

    //logState("on_m_pOkB_clicked 1");
    try
    {
        TransfConfig::Options opt (getOpt());

        TransfConfig cfg (
                getNonSepTerminatedDir(convStr(fromNativeSeparators(m_pSrcDirE->text()))),
                getNonSepTerminatedDir(convStr(fromNativeSeparators((m_pSimpleViewB->isChecked() ? m_pPODest2E : m_pPODestE)->text()))),
                getNonSepTerminatedDir(convStr(fromNativeSeparators(m_pUODestE->text()))),
                getNonSepTerminatedDir(convStr(fromNativeSeparators(m_pProcDestE->text()))),
                getNonSepTerminatedDir(convStr(fromNativeSeparators(m_pTempDestE->text()))),
                getNonSepTerminatedDir(convStr(fromNativeSeparators(m_pCompDestE->text()))),
                opt.getVal()
            );
        m_transfCfg = cfg;

        { // ignored
            m_pCommonData->setIgnoredNotes(m_vSel);
            //cout << "OK:\n"; printContainer(m_pCommonData->m_vnIgnoredNotes, cout);
        }

        { // custom transformations
            getTransfData();
            m_pCommonData->setCustomTransf(m_vvnCustomTransf);
        }

        {
            //m_pCommonData->setVisibleTransf(m_vnVisibleTransf);
            m_pCommonData->setVisibleTransf(m_pVisibleTransfPainter->getSel());
        }

        {
            QualThresholds q;

            q.m_nStereoCbr = BpsInfo::s_aValidBps[m_pQualStCbrCbB->currentIndex()]*1000;
            q.m_nJointStereoCbr = BpsInfo::s_aValidBps[m_pQualJtStCbrCbB->currentIndex()]*1000;
            q.m_nDoubleChannelCbr = BpsInfo::s_aValidBps[m_pQualDlChCbrCbB->currentIndex()]*1000;

            q.m_nStereoVbr = m_pQualStVbrSB->value()*1000;
            q.m_nJointStereoVbr = m_pQualJtStVbrSB->value()*1000;
            q.m_nDoubleChannelVbr = m_pQualDlChVbrSB->value()*1000;

            m_pCommonData->setQualThresholds(q);
        }

        {
            m_pCommonData->m_locale = m_pLocaleCbB->currentText().toLatin1();
            m_pCommonData->m_pCodec = QTextCodec::codecForName(m_pCommonData->m_locale);
            CB_ASSERT (0 != m_pCommonData->m_pCodec);
        }

        { // case
            m_pCommonData->m_eCaseForArtists = (TextCaseOptions)m_pArtistsCaseCbB->currentIndex(); //ttt2 perhaps allow NONE
            m_pCommonData->m_eCaseForOthers = (TextCaseOptions)m_pOthersCaseCbB->currentIndex();
        }

        { // colors
            m_pCommonData->m_vNoteCategColors = m_vNoteCategColors;
        }

        { // tag editor
            m_pCommonData->m_bWarnOnNonSeqTracks = m_pWarnOnNonSeqTracksCkB->isChecked();
            m_pCommonData->m_bWarnPastingToNonSeqTracks = m_pWarnOnPasteToNonSeqTracksCkB->isChecked();
        }

        { // misc
            m_pCommonData->m_bScanAtStartup = m_pScanAtStartupCkB->isChecked();
            m_pCommonData->setFastSave(m_pFastSaveCkB->isChecked(), CommonData::UPDATE_TRANSFORMS);
            m_pCommonData->m_bShowExport = m_pShowExportCkB->isChecked();
            m_pCommonData->m_bShowDebug = m_pShowDebugCkB->isChecked();
            m_pCommonData->m_bShowSessions = m_pShowSessCkB->isChecked();
            m_pCommonData->m_strNormalizeCmd = convStr(m_pNormalizerE->text());
            m_pCommonData->m_bKeepNormWndOpen = m_pKeepNormOpenCkB->isChecked();

            m_pCommonData->m_eAssignSave = m_pAssgnSaveRB->isChecked() ? CommonData::SAVE : m_pAssgnDiscardRB->isChecked() ? CommonData::DISCARD : CommonData::ASK;
            m_pCommonData->m_eNonId3v2Save = m_pNonId3v2SaveRB->isChecked() ? CommonData::SAVE : m_pNonId3v2DiscardRB->isChecked() ? CommonData::DISCARD : CommonData::ASK;

            m_pCommonData->m_nMainWndIconSize = m_pIconSizeSB->value();
            m_pCommonData->m_bAutoSizeIcons = m_pAutoSizeIconsCkB->isChecked();
            m_pCommonData->m_bKeepOneValidImg = m_pKeepOneValidImgCkB->isChecked();
            m_pCommonData->m_bWmpVarArtists = m_pWmpCkB->isChecked();
            m_pCommonData->m_bItunesVarArtists = m_pItunesCkB->isChecked();
            ImageInfo::MAX_IMAGE_SIZE = m_pMaxImgSizeSB->value()*1024; //ttt2 inconsistent to keep this in static var and the others in CommonData; perhaps switch to a global CommonData that anybody can access, without passing it in params
            m_pCommonData->setTraceToFile(m_pTraceToFileCkB->isChecked());

            m_pCommonData->setFontInfo(convStr(m_generalFont.family()), m_generalFont.pointSize(), m_pDecrLabelFontSB->value(), convStr(m_fixedFont.family()), m_fixedFont.pointSize());

            m_pCommonData->m_strRenamerInvalidChars = convStr(m_pInvalidCharsE->text());
            m_pCommonData->m_strRenamerReplacementString = convStr(m_pInvalidReplacementE->text());

            m_pCommonData->m_strCheckForNewVersions = m_pCheckForUpdatesCkB->isChecked() ? "yes" : "no";
        }

        { // shell integration
            ShellIntegration::enableHiddenSession(m_pShellHiddenSessCkB->isChecked());
            ShellIntegration::enableVisibleSession(m_pShellVisibleSessCkB->isChecked());
            ShellIntegration::enableTempSession(m_pShellTempSessCkB->isChecked());
        }

        accept();
    }
    catch (const IncorrectDirName&)
    {
        QMessageBox::critical(this, "Invalid folder name", "A folder name is incorrect."); //ttt2 say which name
    }
}

void ConfigDlgImpl::on_m_pCancelB_clicked()
{
    reject();
}



void ConfigDlgImpl::on_m_pChangeGenFontB_clicked()
{
    bool bOk;
    QFont font = QFontDialog::getFont(&bOk, m_generalFont, this); //ttt2 see if possible to remove "What's this" button
    if (!bOk) { return; }

    m_generalFont = font;
    setFontLabels();
}


void ConfigDlgImpl::on_m_pChangeFixedFontB_clicked()
{
    bool bOk;
    QFont font = QFontDialog::getFont(&bOk, m_fixedFont, this);
    if (!bOk) { return; }

    m_fixedFont = font;
    setFontLabels();
}


void ConfigDlgImpl::setFontLabels()
{
    m_pGeneralFontL->setText(QString("%1, %2pt").arg(m_generalFont.family()).arg(m_generalFont.pointSize()));
    m_pGeneralFontL->setFont(m_generalFont);
    m_pFixedFontL->setText(QString("%1, %2pt").arg(m_fixedFont.family()).arg(m_fixedFont.pointSize()));
    m_pFixedFontL->setFont(m_fixedFont);

    QFont f (m_fixedFont);
    f.setUnderline(true);
    m_pInvalidReplacementE->setFont(f);
    m_pInvalidCharsE->setFont(f);
}




void ConfigDlgImpl::logState(const char* /*szPlace*/) const
{
    /*cout << szPlace << ": m_filter.m_vSelDirs=" << m_pCommonData->m_filter.m_vSelDirs.size() << " m_availableDirs.m_vstrDirs=" << m_availableDirs.m_vstrDirs.size() << " m_selectedDirs.m_vSelDirs=" << m_selectedDirs.m_vstrDirs.size() << endl;*/
}



//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------


/*override*/ string ConfigDlgImpl::getTooltip(TooltipKey eTooltipKey) const
{
    switch (eTooltipKey)
    {
    case SELECTED_G: return "";//"Notes to be included";
    case AVAILABLE_G: return "";//"Available notes";
    case ADD_B: return "Add selected note(s)";
    case DELETE_B: return "Remove selected note(s)";
    case ADD_ALL_B: return "Add all notes";
    case DELETE_ALL_B: return "Remove all notes";
    case RESTORE_DEFAULT_B: return "Restore lists to their default value";
    case RESTORE_OPEN_B: return "Restore lists to the configuration they had when the window was open";
    default: CB_ASSERT(false);
    }
}


/*override*/ void ConfigDlgImpl::reset()
{
    if (m_vpResetAll.empty())
    {
        const vector<const Note*>& v (Notes::getAllNotes());
        for (int i = 0, n = cSize(v); i < n; ++i)
        {
            const Note* p (v[i]);
            m_vpResetAll.push_back(new NoteListElem(p, m_pCommonData));
        }
    }

    m_vSel = Notes::getDefaultIgnoredNoteIds();
}


void ConfigDlgImpl::selectDir(QLineEdit* pEdt)
{
    QString qstrStart (fromNativeSeparators(pEdt->text()));
    qstrStart = convStr(getExistingDir(convStr(qstrStart)));
    if (qstrStart.isEmpty()) { qstrStart = getTempDir(); }
    QFileDialog dlg (this, "Select folder", qstrStart, "All files (*)");

    dlg.setFileMode(QFileDialog::Directory);
    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    QString s (fileNames.first());

    QString s1 (getPathSep());

    if ("//" == s)
    {
        s = "/";
    }
    if (!s.endsWith(s1))
    {
        s += s1;
    }
    pEdt->setText(toNativeSeparators(s));
}



void ConfigDlgImpl::onHelp()
{
    switch(m_pMainTabWidget->currentIndex())
    {
    case 0: openHelp("250_config_files.html"); break;
    case 1: openHelp("260_config_ignored_notes.html"); break;
    case 2: openHelp("270_config_custom_transf.html"); break;
    case 3: openHelp("290_config_transf_params.html"); break;
    case 4: openHelp("292_config_visible_transf.html"); break;
    case 5: openHelp("295_config_quality.html"); break;
    case 6: openHelp("297_config_colors.html"); break;
    case 7: openHelp("298_config_shell.html"); break;
    case 8: openHelp("300_config_others.html"); break;
    //tttr revise as needed

    default: /*openHelp("index.html");*/ break;
    }
}


void ConfigDlgImpl::on_m_pFastSaveCkB_stateChanged()
{
    if (0 == m_pVisibleTransfPainter) { return; }

    if (m_pFastSaveCkB->isChecked())
    {
        const vector<int>& vnAvail (m_pVisibleTransfPainter->getAvailable());
        const vector<const ListElem*>& vpAll (m_pVisibleTransfPainter->getAll());
        set<int> s;
        for (int i = 0; i < cSize(vnAvail); ++i)
        {
            const TransfListElem* p (dynamic_cast<const TransfListElem*>(vpAll.at(vnAvail.at(i))));
            CB_ASSERT (0 != p);
            if (p->getTransformation()->getActionName() == string(Id3V2Compactor::getClassName()) || p->getTransformation()->getActionName() == string(Id3V2Expander::getClassName()))
            {
                s.insert(i);
            }
        }

        m_pVisibleTransfDoubleList->add(s);
    }
    else
    {
        const vector<int>& vnSel (m_pVisibleTransfPainter->getSel());
        const vector<const ListElem*>& vpAll (m_pVisibleTransfPainter->getAll());
        set<int> s;
        for (int i = 0; i < cSize(vnSel); ++i)
        {
            const TransfListElem* p (dynamic_cast<const TransfListElem*>(vpAll.at(vnSel.at(i))));
            CB_ASSERT (0 != p);
            if (p->getTransformation()->getActionName() == string(Id3V2Compactor::getClassName()) || p->getTransformation()->getActionName() == string(Id3V2Expander::getClassName()))
            {
                s.insert(i);
            }
        }

        m_pVisibleTransfDoubleList->remove(s);
    }
}



TransfConfig::Options ConfigDlgImpl::getSimpleViewOpt() // doesn't set m_bKeepOrigTime
{
    TransfConfig::Options opt (getFullViewOpt());
    if (m_pBackupCustomRB->isChecked()) { return opt; }

    if (m_pDontCreateBackupRB->isChecked()) { return opt.asNonBackup(); }

    return opt.asBackup();
}


TransfConfig::Options ConfigDlgImpl::getFullViewOpt() // doesn't set m_bKeepOrigTime
{
    TransfConfig::Options opt;

    opt.m_eProcOrigChange = m_pPODontChangeRB->isChecked() ? TransfConfig::Options::PO_DONT_CHG : m_pPOEraseRB->isChecked() ? TransfConfig::Options::PO_ERASE : m_pPOMoveAlwaysChangeRB->isChecked() ? TransfConfig::Options::PO_MOVE_ALWAYS_RENAME : m_pPOMoveChangeIfNeededRB->isChecked() ? TransfConfig::Options::PO_MOVE_RENAME_IF_USED : m_pPOChangeNameRB->isChecked() ? TransfConfig::Options::PO_RENAME_SAME_DIR : m_pPOMoveOrEraseRB->isChecked() ? TransfConfig::Options::PO_MOVE_OR_ERASE : TransfConfig::Options::ProcOrig(7);
    CB_ASSERT (7 != opt.m_eProcOrigChange);
    opt.m_bProcOrigUseLabel = m_pPOUseLabelRB->isChecked();
    opt.m_bProcOrigAlwayUseCounter = m_pPOAlwaysUseCounterRB->isChecked();

    opt.m_eUnprocOrigChange = m_pUODontChangeRB->isChecked() ? TransfConfig::Options::UPO_DONT_CHG : m_pUOEraseRB->isChecked() ? TransfConfig::Options::UPO_ERASE : m_pUOMoveAlwaysChangeRB->isChecked() ? TransfConfig::Options::UPO_MOVE_ALWAYS_RENAME : m_pUOMoveChangeIfNeededRB->isChecked() ? TransfConfig::Options::UPO_MOVE_RENAME_IF_USED : m_pUOChangeNameRB->isChecked() ? TransfConfig::Options::UPO_RENAME_SAME_DIR : TransfConfig::Options::UnprocOrig(7);
    CB_ASSERT (7 != opt.m_eUnprocOrigChange);
    opt.m_bUnprocOrigUseLabel = m_pUOUseLabelRB->isChecked();
    opt.m_bUnprocOrigAlwayUseCounter = m_pUOAlwaysUseCounterRB->isChecked();

    opt.m_eProcessedCreate = m_pProcDontCreateRB->isChecked() ? TransfConfig::Options::PR_DONT_CREATE : m_pProcCreateAlwaysChangeRB->isChecked() ? TransfConfig::Options::PR_CREATE_ALWAYS_RENAME : m_pProcCreateChangeIfNeededRB->isChecked() ? TransfConfig::Options::PR_CREATE_RENAME_IF_USED : TransfConfig::Options::Processed(3);
    CB_ASSERT (3 != opt.m_eProcessedCreate);
    opt.m_bProcessedUseLabel = m_pProcUseLabelRB->isChecked();
    opt.m_bProcessedAlwayUseCounter = m_pProcAlwaysUseCounterRB->isChecked();
    opt.m_bProcessedUseSeparateDir = m_pProcUseSeparateDirRB->isChecked();

    opt.m_bTempCreate = m_pTempCreateRB->isChecked();

    opt.m_bCompCreate = m_pCompCreateRB->isChecked();

    return opt;
}


TransfConfig::Options ConfigDlgImpl::getOpt() // has the correct m_bKeepOrigTime
{
    TransfConfig::Options opt (m_pSimpleViewB->isChecked() ? getSimpleViewOpt() : getFullViewOpt());
    opt.m_bKeepOrigTime = m_pKeepOrigTimeCkB->isChecked();
    return opt;
}


void ConfigDlgImpl::setSimpleViewOpt(const TransfConfig::Options& opt) // m_bKeepOrigTime shouldn't be set
{
    if (opt == opt.asNonBackup())
    {
        m_pDontCreateBackupRB->setChecked(true);
    }
    else if (opt == opt.asBackup())
    {
        m_pCreateBackupRB->setChecked(true);
    }
    else
    {
        m_pBackupCustomRB->setChecked(true);
    }
}


void ConfigDlgImpl::setFullViewOpt(const TransfConfig::Options& opt) // m_bKeepOrigTime is ignored
{
    { // ProcOrig
        switch (opt.m_eProcOrigChange)
        {
        case TransfConfig::Options::PO_DONT_CHG: m_pPODontChangeRB->setChecked(true); break;
        case TransfConfig::Options::PO_ERASE: m_pPOEraseRB->setChecked(true); break;
        case TransfConfig::Options::PO_MOVE_ALWAYS_RENAME: m_pPOMoveAlwaysChangeRB->setChecked(true); break;
        case TransfConfig::Options::PO_MOVE_RENAME_IF_USED: m_pPOMoveChangeIfNeededRB->setChecked(true); break;
        case TransfConfig::Options::PO_RENAME_SAME_DIR: m_pPOChangeNameRB->setChecked(true); break;
        case TransfConfig::Options::PO_MOVE_OR_ERASE: m_pPOMoveOrEraseRB->setChecked(true); break;
        default:
            CB_ASSERT(false); // the constructor of TransfConfig should have detected it
        }

        if (opt.m_bProcOrigUseLabel) { m_pPOUseLabelRB->setChecked(true); } else { m_pPODontUseLabelRB->setChecked(true); }
        if (opt.m_bProcOrigAlwayUseCounter) { m_pPOAlwaysUseCounterRB->setChecked(true); } else { m_pPOUseCounterIfNeededRB->setChecked(true); }
    }

    { // UnprocOrig
        switch (opt.m_eUnprocOrigChange)
        {
        case TransfConfig::Options::UPO_DONT_CHG: m_pUODontChangeRB->setChecked(true); break;
        case TransfConfig::Options::UPO_ERASE: m_pUOEraseRB->setChecked(true); break;
        case TransfConfig::Options::UPO_MOVE_ALWAYS_RENAME: m_pUOMoveAlwaysChangeRB->setChecked(true); break;
        case TransfConfig::Options::UPO_MOVE_RENAME_IF_USED: m_pUOMoveChangeIfNeededRB->setChecked(true); break;
        case TransfConfig::Options::UPO_RENAME_SAME_DIR: m_pUOChangeNameRB->setChecked(true); break;
        default:
            CB_ASSERT(false); // the constructor of TransfConfig should have detected it
        }

        if (opt.m_bUnprocOrigUseLabel) { m_pUOUseLabelRB->setChecked(true); } else { m_pUODontUseLabelRB->setChecked(true); }
        if (opt.m_bUnprocOrigAlwayUseCounter) { m_pUOAlwaysUseCounterRB->setChecked(true); } else { m_pUOUseCounterIfNeededRB->setChecked(true); }

    }

    { // Processed
        switch (opt.m_eProcessedCreate)
        {
        case TransfConfig::Options::PR_DONT_CREATE: m_pProcDontCreateRB->setChecked(true); break;
        case TransfConfig::Options::PR_CREATE_ALWAYS_RENAME: m_pProcCreateAlwaysChangeRB->setChecked(true); break;
        case TransfConfig::Options::PR_CREATE_RENAME_IF_USED: m_pProcCreateChangeIfNeededRB->setChecked(true); break;
        default:
            CB_ASSERT(false); // the constructor of TransfConfig should have detected it
        }

        if (opt.m_bProcessedUseLabel) { m_pProcUseLabelRB->setChecked(true); } else { m_pProcDontUseLabelRB->setChecked(true); }
        if (opt.m_bProcessedAlwayUseCounter) { m_pProcAlwaysUseCounterRB->setChecked(true); } else { m_pProcUseCounterIfNeededRB->setChecked(true); }
        if (opt.m_bProcessedUseSeparateDir) { m_pProcUseSeparateDirRB->setChecked(true); } else { m_pProcUseSrcRB->setChecked(true); }
    }

    { // Temp
        if (opt.m_bTempCreate) { m_pTempCreateRB->setChecked(true); } else { m_pTempDontCreateRB->setChecked(true); }
    }

    { // Comp
        if (opt.m_bCompCreate) { m_pCompCreateRB->setChecked(true); } else { m_pCompDontCreateRB->setChecked(true); }
    }
}


void ConfigDlgImpl::on_m_pSimpleViewB_clicked()
{
    m_pFileSettingsLayout->setCurrentWidget(m_pSimpleViewTab);
    m_pFullViewB->setChecked(false);
    m_pPODest2E->setText(m_pPODestE->text());
    setSimpleViewOpt(getFullViewOpt());
}

void ConfigDlgImpl::on_m_pFullViewB_clicked()
{
    m_pFileSettingsLayout->setCurrentWidget(m_pFullViewTab);
    m_pSimpleViewB->setChecked(false);
    m_pPODestE->setText(m_pPODest2E->text());
    setFullViewOpt(getSimpleViewOpt());
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

//ttt2 dir config: perhaps something simpler, with a "more options" button

//ttt2 Font style is ignored (see DejaVu Sans / Light on machines with antialiased fonts)

//ttt2 proxy: QNetworkProxyFactory::systemProxyForQuery; QNetworkProxy; http://www.dbits.be/index.php/pc-problems/65-vistaproxycfg  https://sourceforge.net/projects/mp3diags/forums/forum/947207/topic/3415940


