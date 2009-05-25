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

//ttt1 simple / detailed files tab, with "simple" only allowing to "create backups in ..."
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


class TransfListPainter : public ListPainter
{
    /*override*/ int getColCount() const { return 2; }
    /*override*/ std::string getColTitle(int nCol) const { return 0 == nCol ? "Action" : "Description"; }
    /*override*/ QColor getColor(int /*nIndex*/, int /*nCol*/, QColor origColor) const { return origColor; }
    /*override*/ int getColWidth(int /*nCol*/) const { return -1; } // positive values are used for fixed widths, while negative ones are for "stretched"
    /*override*/ int getHdrHeight() const { return CELL_HEIGHT; }
    /*override*/ Qt::Alignment getAlignment(int /*nCol*/) const { return Qt::AlignTop | Qt::AlignLeft; }
    /*override*/ std::string getTooltip(TooltipKey eTooltipKey) const;
    /*override*/ void reset();

    const SubList& m_vDefaultSel; // to be used by reset()
public:
    TransfListPainter(const CommonData* pCommonData, const SubList& vOrigSel, const SubList& vSel, const SubList& vDefaultSel);
    ~TransfListPainter();
};



TransfListPainter::TransfListPainter(const CommonData* pCommonData, const SubList& vOrigSel, const SubList& vSel, const SubList& vDefaultSel) : ListPainter(""), m_vDefaultSel(vDefaultSel)
{
//qDebug("init TransfListPainter with origsel %d and sel %d", cSize(vOrigSel), cSize(vSel));
    for (int i = 0, n = cSize(pCommonData->getTransf()); i < n; ++i)
    {
        m_vpOrigAll.push_back(new TransfListElem(pCommonData->getTransf()[i]));
    }
    m_vpResetAll = m_vpOrigAll; // !!! no new pointers
    m_vOrigSel = vOrigSel;
    m_vSel = vSel;
}


TransfListPainter::~TransfListPainter()
{
    clearPtrContainer(m_vpOrigAll);
}


/*override*/ string TransfListPainter::getTooltip(TooltipKey eTooltipKey) const
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


/*override*/ void TransfListPainter::reset()
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
            qDebug("%s %d", pObj->objectName().toUtf8().data(), (int)pEvent->type());
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
        QDialog(pParent, 0),
        Ui::ConfigDlg(),
        NoteListPainter("<all notes>"),
        m_transfCfg(transfCfg),

        m_pCommonData(pCommonData),
        m_pTransfListPainter(0),
        m_pTransfDoubleList(0),
        m_vvCustomTransf(pCommonData->getCustomTransf()),
        m_nCurrentTransf(-1),
        m_vvDefaultTransf(CUSTOM_TRANSF_CNT)
{
    setupUi(this);

    if (!bFull)
    {
        //m_pMainTabWidget->removeTab(4);
        m_pMainTabWidget->removeTab(3);
        m_pMainTabWidget->removeTab(2);
        m_pMainTabWidget->removeTab(1);
        m_pLocaleGrp->hide();
        m_pCaseGrp->hide();
        //m_pRenameW->hide();
        m_pOthersMiscGrp->hide();
        m_pNormalizeGrp->hide();
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
                            Id3V2FrameDataLoader wrp (*pFrm);
                            const char* pData (wrp.getData());
                            CB_ASSERT (0 == pData[0]); // "Latin1" encoding
                            QByteArray arr (pData + 1, pFrm->m_nMemDataSize - 1);
                            if (!qstr.isEmpty())
                            {
                                qstr += '\n';
                            }
                            qstr += QString::fromLatin1(arr);
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

    m_pSrcDirE->setText(convStr(getSepTerminatedDir(transfCfg.getSrcDir())));

    { // ProcOrig
        switch (transfCfg.m_optionsWrp.m_opt.m_nProcOrigChange)
        {
        case 0: m_pPODontChangeRB->setChecked(true); break;
        case 1: m_pPOEraseRB->setChecked(true); break;
        case 2: m_pPOMoveAlwaysChangeRB->setChecked(true); break;
        case 3: m_pPOMoveChangeIfNeededRB->setChecked(true); break;
        case 4: m_pPOChangeNameRB->setChecked(true); break;
        case 5: m_pPOMoveOrEraseRB->setChecked(true); break;
        default:
            CB_ASSERT(false); // the constructor of TransfConfig should have detected it
        }

        if (transfCfg.m_optionsWrp.m_opt.m_bProcOrigUseLabel) { m_pPOUseLabelRB->setChecked(true); } else { m_pPODontUseLabelRB->setChecked(true); }
        if (transfCfg.m_optionsWrp.m_opt.m_bProcOrigAlwayUseCounter) { m_pPOAlwaysUseCounterRB->setChecked(true); } else { m_pPOUseCounterIfNeededRB->setChecked(true); }

        m_pPODestE->setText(convStr(getSepTerminatedDir(transfCfg.getProcOrigDir())));
    }

    { // UnprocOrig
        switch (transfCfg.m_optionsWrp.m_opt.m_nUnprocOrigChange)
        {
        case 0: m_pUODontChangeRB->setChecked(true); break;
        case 1: m_pUOEraseRB->setChecked(true); break;
        case 2: m_pUOMoveAlwaysChangeRB->setChecked(true); break;
        case 3: m_pUOMoveChangeIfNeededRB->setChecked(true); break;
        case 4: m_pUOChangeNameRB->setChecked(true); break;
        default:
            CB_ASSERT(false); // the constructor of TransfConfig should have detected it
        }

        if (transfCfg.m_optionsWrp.m_opt.m_bUnprocOrigUseLabel) { m_pUOUseLabelRB->setChecked(true); } else { m_pUODontUseLabelRB->setChecked(true); }
        if (transfCfg.m_optionsWrp.m_opt.m_bUnprocOrigAlwayUseCounter) { m_pUOAlwaysUseCounterRB->setChecked(true); } else { m_pUOUseCounterIfNeededRB->setChecked(true); }

        m_pUODestE->setText(convStr(getSepTerminatedDir(transfCfg.getUnprocOrigDir())));
    }

    { // Processed
        switch (transfCfg.m_optionsWrp.m_opt.m_nProcessedCreate)
        {
        case 0: m_pProcDontCreateRB->setChecked(true); break;
        case 1: m_pProcCreateAlwaysChangeRB->setChecked(true); break;
        case 2: m_pProcCreateChangeIfNeededRB->setChecked(true); break;
        default:
            CB_ASSERT(false); // the constructor of TransfConfig should have detected it
        }

        if (transfCfg.m_optionsWrp.m_opt.m_bProcessedUseLabel) { m_pProcUseLabelRB->setChecked(true); } else { m_pProcDontUseLabelRB->setChecked(true); }
        if (transfCfg.m_optionsWrp.m_opt.m_bProcessedAlwayUseCounter) { m_pProcAlwaysUseCounterRB->setChecked(true); } else { m_pProcUseCounterIfNeededRB->setChecked(true); }
        if (transfCfg.m_optionsWrp.m_opt.m_bProcessedUseSeparateDir) { m_pProcUseSeparateDirRB->setChecked(true); } else { m_pProcUseSrcRB->setChecked(true); }

        m_pProcDestE->setText(convStr(getSepTerminatedDir(transfCfg.getProcessedDir())));
    }

    {
        m_pKeepOrigTimeCkB->setChecked(transfCfg.m_optionsWrp.m_opt.m_bKeepOrigTime);
    }

    { // Temp
        if (transfCfg.m_optionsWrp.m_opt.m_bTempCreate) { m_pTempCreateRB->setChecked(true); } else { m_pTempDontCreateRB->setChecked(true); }
        m_pTempDestE->setText(convStr(getSepTerminatedDir(transfCfg.getTempDir())));
    }

    { // Comp
        if (transfCfg.m_optionsWrp.m_opt.m_bCompCreate) { m_pCompCreateRB->setChecked(true); } else { m_pCompDontCreateRB->setChecked(true); }
        m_pCompDestE->setText(convStr(getSepTerminatedDir(transfCfg.getCompDir())));
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
        initDefaultCustomTransf(i, m_vvDefaultTransf, m_pCommonData);
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
        QList<QByteArray> l (QTextCodec::availableCodecs());
        for (int i = 0, n = l.size(); i < n; ++i)
        {
            lNames << QString::fromLatin1(l[i]);
        }

        m_pLocaleCbB->addItems(lNames);
        int n (m_pLocaleCbB->findText(m_pCommonData->m_locale));
        if (-1 == n) { n = 0; }
        m_pLocaleCbB->setCurrentIndex(n);
    }

    { // case
        QStringList lNames;
        lNames << "Lower case: first part. second part.";
        lNames << "Upper case: FIRST PART. SECOND PART.";
        lNames << "Title case: First Part. Second Part.";
        lNames << "Phrase case: First part. Second part.";

        m_pArtistsCaseCbB->addItems(lNames);
        m_pOthersCaseCbB->addItems(lNames);
        m_pArtistsCaseCbB->setCurrentIndex((int)m_pCommonData->m_eCaseForArtists);
        m_pOthersCaseCbB->setCurrentIndex((int)m_pCommonData->m_eCaseForOthers);
    }

    { // tag editor
        m_pWarnOnNonSeqTracksCkB->setChecked(m_pCommonData->m_bWarnOnNonSeqTracks);
        m_pWarnOnPasteToNonSeqTracksCkB->setChecked(m_pCommonData->m_bWarnPastingToNonSeqTracks);
    }

    {  //misc
        m_pScanAtStartupCkB->setChecked(m_pCommonData->m_bScanAtStartup);
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

        m_generalFont = m_pCommonData->getGeneralFont();
        m_fixedFont = m_pCommonData->getFixedFont();
        setFontLabels();
    }

    m_pSrcDirE->setFocus();
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

void SessionSettings::loadTransfConfig(TransfConfig& transfConfig) const
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
    }
    catch (const IncorrectDirName&)
    {
        TransfConfig tc ("*", "*", "*", "*", "*", "*", -1);
        transfConfig = tc;
    }
}


bool ConfigDlgImpl::run()
{
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
        break;

    case 1:
        v.push_back(pCommonData->getTransfPos(Id3V2Rescuer::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnsupportedId3V2Remover::getClassName()));
        v.push_back(pCommonData->getTransfPos(MultipleId3StreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(Id3V1ToId3V2Copier::getClassName()));
        break;

    case 2:
        v.push_back(pCommonData->getTransfPos(InnerNonAudioRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnknownDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(BrokenDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(UnsupportedDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(TruncatedMpegDataStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(NullStreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(MultipleId3StreamRemover::getClassName()));
        v.push_back(pCommonData->getTransfPos(MismatchedXingRemover::getClassName()));
        break;

    case 3: // CUSTOM_TRANSF_CNT
        //v.push_back(pCommonData->getTransfPos(InnerNonAudioRemover::getClassName())); //ttt1 see if there is a better way to set this up
        break;

    default:
        CB_ASSERT (false);
    }
}


ConfigDlgImpl::~ConfigDlgImpl()
{
    clearPtrContainer(m_vpOrigAll);
    clearPtrContainer(m_vpResetAll); // doesn't matter if it was used or not, or if m_bResultInReset is true or false
    delete m_pTransfListPainter;
}


void ConfigDlgImpl::refreshTransfText(int k)
{
    QString s;
    for (int i = 0, n = cSize(m_vvCustomTransf[k]); i < n; ++i)
    {
        s += m_pCommonData->getTransf()[m_vvCustomTransf[k][i]]->getActionName();
        if (i < n - 1) { s += "\n"; }
    }
    m_vpTransfLabels[k]->setText(s);
}


void ConfigDlgImpl::selectCustomTransf(int k) // 0 <= k <= 2
{
    getTransfData();
    delete m_pTransfDoubleList;
    delete m_pTransfListPainter;
    m_pTransfListPainter = new TransfListPainter(m_pCommonData, m_pCommonData->getCustomTransf()[k], m_vvCustomTransf[k], m_vvDefaultTransf[k]);

    m_pTransfDoubleList = new DoubleList(
            *m_pTransfListPainter,
            DoubleList::RESTORE_OPEN | DoubleList::RESTORE_DEFAULT,
            DoubleList::MULTIPLE,
            "All transformations",
            "Used transformations",
            this);

    m_pTransfListHldr->layout()->addWidget(m_pTransfDoubleList);
    //m_pTransfListHldr->layout()->activate();
    //?->300x358//m_pTransfDoubleList->layout()->activate();
    //m_pTransfDoubleList->resizeOnShow();
    //m_pDetailsTabWidget->setCurrentIndex(0);
    //m_pDetailsTabWidget->setCurrentIndex(2);

    for (int i = 0; i < CUSTOM_TRANSF_CNT; ++i)
    {
        m_vpTransfButtons[i]->setChecked(false); m_vpTransfLabels[i]->setPalette(m_wndPalette);
    }

    m_vpTransfButtons[k]->setChecked(true); m_vpTransfLabels[k]->setPalette(m_defaultPalette);
    m_nCurrentTransf = k;

    connect(m_pTransfDoubleList, SIGNAL(dataChanged()), this, SLOT(onTransfDataChanged()));
}





void ConfigDlgImpl::onTransfDataChanged()
{
    getTransfData();
}


void ConfigDlgImpl::getTransfData()
{
    if (-1 == m_nCurrentTransf) { return; }
    m_vvCustomTransf[m_nCurrentTransf] = m_pTransfListPainter->getSel();
//qDebug("transf %d at %d", cSize(m_vvCustomTransf[m_nCurrentTransf]), m_nCurrentTransf);
    refreshTransfText(m_nCurrentTransf);
}



void ConfigDlgImpl::on_m_pOkB_clicked()
{
    //logState("on_m_pOkB_clicked 1");
    try
    {
        TransfConfig::OptionsWrp wrp;
        wrp.m_nVal = 0;

        wrp.m_opt.m_nProcOrigChange = m_pPODontChangeRB->isChecked() ? 0 : m_pPOEraseRB->isChecked() ? 1 : m_pPOMoveAlwaysChangeRB->isChecked() ? 2 : m_pPOMoveChangeIfNeededRB->isChecked() ? 3 : m_pPOChangeNameRB->isChecked() ? 4 : m_pPOMoveOrEraseRB->isChecked() ? 5 : 7;
        wrp.m_opt.m_bProcOrigUseLabel = m_pPOUseLabelRB->isChecked();
        wrp.m_opt.m_bProcOrigAlwayUseCounter = m_pPOAlwaysUseCounterRB->isChecked();

        wrp.m_opt.m_nUnprocOrigChange = m_pUODontChangeRB->isChecked() ? 0 : m_pUOEraseRB->isChecked() ? 1 : m_pUOMoveAlwaysChangeRB->isChecked() ? 2 : m_pUOMoveChangeIfNeededRB->isChecked() ? 3 : m_pUOChangeNameRB->isChecked() ? 4 : 7;
        wrp.m_opt.m_bUnprocOrigUseLabel = m_pUOUseLabelRB->isChecked();
        wrp.m_opt.m_bUnprocOrigAlwayUseCounter = m_pUOAlwaysUseCounterRB->isChecked();

        wrp.m_opt.m_nProcessedCreate = m_pProcDontCreateRB->isChecked() ? 0 : m_pProcCreateAlwaysChangeRB->isChecked() ? 1 : m_pProcCreateChangeIfNeededRB->isChecked() ? 2 : 3;
        wrp.m_opt.m_bProcessedUseLabel = m_pProcUseLabelRB->isChecked();
        wrp.m_opt.m_bProcessedAlwayUseCounter = m_pProcAlwaysUseCounterRB->isChecked();
        wrp.m_opt.m_bProcessedUseSeparateDir = m_pProcUseSeparateDirRB->isChecked();

        wrp.m_opt.m_bTempCreate = m_pTempCreateRB->isChecked();

        wrp.m_opt.m_bCompCreate = m_pCompCreateRB->isChecked();

        wrp.m_opt.m_bKeepOrigTime = m_pKeepOrigTimeCkB->isChecked();

        TransfConfig cfg (
                getNonSepTerminatedDir(convStr(m_pSrcDirE->text())),
                getNonSepTerminatedDir(convStr(m_pPODestE->text())),
                getNonSepTerminatedDir(convStr(m_pUODestE->text())),
                getNonSepTerminatedDir(convStr(m_pProcDestE->text())),
                getNonSepTerminatedDir(convStr(m_pTempDestE->text())),
                getNonSepTerminatedDir(convStr(m_pCompDestE->text())),
                wrp.m_nVal
            );
        m_transfCfg = cfg;

        { // ignored
            m_pCommonData->setIgnoredNotes(m_vSel);
            //cout << "OK:\n"; printContainer(m_pCommonData->m_vnIgnoredNotes, cout);
        }

        { // custom transformations
            getTransfData();
            m_pCommonData->setCustomTransf(m_vvCustomTransf);
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
            m_pCommonData->m_pCodec = (QTextCodec::codecForName(m_pCommonData->m_locale));
            CB_ASSERT (0 != m_pCommonData->m_pCodec);
        }

        {
            m_pCommonData->m_eCaseForArtists = (CommonData::Case)m_pArtistsCaseCbB->currentIndex();
            m_pCommonData->m_eCaseForOthers = (CommonData::Case)m_pOthersCaseCbB->currentIndex();
        }

        { // tag editor
            m_pCommonData->m_bWarnOnNonSeqTracks = m_pWarnOnNonSeqTracksCkB->isChecked();
            m_pCommonData->m_bWarnPastingToNonSeqTracks = m_pWarnOnPasteToNonSeqTracksCkB->isChecked();
        }

        { // misc
            m_pCommonData->m_bScanAtStartup = m_pScanAtStartupCkB->isChecked();
            m_pCommonData->m_bShowDebug = m_pShowDebugCkB->isChecked();
            m_pCommonData->m_bShowSessions = m_pShowSessCkB->isChecked();
            m_pCommonData->m_strNormalizeCmd = convStr(m_pNormalizerE->text());
            m_pCommonData->m_bKeepNormWndOpen = m_pKeepNormOpenCkB->isChecked();

            m_pCommonData->m_eAssignSave = m_pAssgnSaveRB->isChecked() ? CommonData::SAVE : m_pAssgnDiscardRB->isChecked() ? CommonData::DISCARD : CommonData::ASK;
            m_pCommonData->m_eNonId3v2Save = m_pNonId3v2SaveRB->isChecked() ? CommonData::SAVE : m_pNonId3v2DiscardRB->isChecked() ? CommonData::DISCARD : CommonData::ASK;

            m_pCommonData->m_nMainWndIconSize = m_pIconSizeSB->value();
            m_pCommonData->m_bAutoSizeIcons = m_pAutoSizeIconsCkB->isChecked();
            m_pCommonData->m_bKeepOneValidImg = m_pKeepOneValidImgCkB->isChecked();

            m_pCommonData->setGeneralFont(convStr(m_generalFont.family()), m_generalFont.pointSize());
            m_pCommonData->setFixedFont(convStr(m_fixedFont.family()), m_fixedFont.pointSize());
        }

        accept();
    }
    catch (const IncorrectDirName&)
    {
        QMessageBox::critical(this, "Invalid folder name", "A folder name is incorrect."); //ttt1 say which name
    }
}

void ConfigDlgImpl::on_m_pCancelB_clicked()
{
    reject();
}



void ConfigDlgImpl::on_m_pChangeGenFontB_clicked()
{
    bool bOk;
    QFont font = QFontDialog::getFont(&bOk, m_generalFont, this);
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
}




void ConfigDlgImpl::logState(const char* /*szPlace*/) const
{
    /*cout << szPlace << ": m_filter.m_vSelDirs=" << m_pCommonData->m_filter.m_vSelDirs.size() << " m_availableDirs.m_vDirs=" << m_availableDirs.m_vDirs.size() << " m_selectedDirs.m_vSelDirs=" << m_selectedDirs.m_vDirs.size() << endl;*/
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
    QString qstrStart (pEdt->text());
    qstrStart = convStr(getExistingDir(convStr(qstrStart)));
    if (qstrStart.isEmpty()) { qstrStart = "/"; } //ttt1 linux-specific
    QFileDialog dlg (this, "Select folder", qstrStart, "All files (*)");

    dlg.setFileMode(QFileDialog::Directory);
    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    QString s (fileNames.first());

    QString s1 (getPathSep());

    if ("//" == s) //ttt1 Linux-specific ; see if there's a need for something similar on Windows
    {
        s = "/";
    }
    if (!s.endsWith(s1))
    {
        s += s1;
    }
    pEdt->setText(s);
}

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

//ttt2 dir config: perhaps something simpler, with a "more options" button

//ttt1 Font style is ignored (see DejaVu Sans / Light on machines with antialiased fonts)
