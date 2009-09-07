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


#include  <cmath>
#include  <algorithm>
#include  <sstream>

#include  <QApplication>
#include  <QToolButton>
#include  <QTableView>
#include  <QDesktopWidget>
#include  <QSettings>
#include  <QTextCodec>
#include  <QHeaderView>
#include  <QMessageBox>
#include  <QPainter>

#include  "CommonData.h"

#include  "Helpers.h"
#include  "StructuralTransformation.h"
#include  "Id3Transf.h"
#include  "OsFile.h"
#include  "StoredSettings.h"
#include  "FilesModel.h"      // all files
#include  "NotesModel.h"      // current notes
#include  "StreamsModel.h"    // current streams
#include  "UniqueNotesModel.h"   // all notes
#include  "CommonTypes.h"

using namespace std;
using namespace pearl;



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


UniqueNotes::UniqueNotes() : m_bAllDirty(true), m_bFltDirty(true)
{
}


/*UniqueNotes::UniqueNotes(const UniqueNotes& other)
{
    addColl(other.getAllVec());
    setSel(other.getSelVec());
}*/


UniqueNotes::~UniqueNotes()
{
    clear();
}


void UniqueNotes::clear()
{
    m_spFlt.clear();
    //clearPtrContainer(m_spAll);
    m_spAll.clear();
    m_bAllDirty = true;
    m_bFltDirty = true;
}


bool UniqueNotes::addNote(const Note* pNote) // if the note doesn't exist in m_spAll, it adds the corresponding note from Notes; returns true if the param really was added;
{
    if (m_spAll.count(pNote) > 0)
    {
        return false;
    }

    const Note* p (Notes::getMaster(pNote));
    CB_ASSERT (0 != p);
    m_spAll.insert(p);
    m_bAllDirty = true;
    return true;
}



void UniqueNotes::updateVAll() const
{
    if (m_bAllDirty)
    {
        m_vpAll.clear();
        m_vpAll.insert(m_vpAll.end(), m_spAll.begin(), m_spAll.end());
        m_bAllDirty = false;
    }
}

void UniqueNotes::updateVFlt() const
{
    if (m_bFltDirty)
    {
        m_vpFlt.clear();
        m_vpFlt.insert(m_vpFlt.end(), m_spFlt.begin(), m_spFlt.end());
        m_bFltDirty = false;
    }
}

int UniqueNotes::getPos(const Note* pNote) const // position in the "all" notes; -1 if the note wasn't found;
{
    updateVAll();
    vector<const Note*>::const_iterator it (lower_bound(m_vpAll.begin(), m_vpAll.end(), pNote, CmpNotePtrById())); // easier than equal_range() in this case
    if (it == m_vpAll.end() || !CmpNotePtrById::equals(*it, pNote))
    { // trace or info
        return -1;
    }
    return it - m_vpAll.begin();
}


int UniqueNotes::getFltPos(const Note* pNote) const // position in the "sel" notes; -1 if the note wasn't found;
{
    updateVFlt();
    vector<const Note*>::const_iterator it (lower_bound(m_vpFlt.begin(), m_vpFlt.end(), pNote, CmpNotePtrById()));
    if (it == m_vpFlt.end() || !CmpNotePtrById::equals(*it, pNote))
    { // trace or info
        return -1;
    }
    return it - m_vpFlt.begin();
}


const Note* UniqueNotes::getFlt(int n) const
{
    updateVFlt();
    return m_vpFlt.at(n);
}


const Note* UniqueNotes::get(int n) const
{
    updateVAll();
    return m_vpAll.at(n);
}




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================




void SessionSettings::saveMiscConfigSettings(const CommonData* p)
{
    { // quality
        m_pSettings->setValue("quality/stereoCbrMinBitrate", p->getQualThresholds().m_nStereoCbr);
        m_pSettings->setValue("quality/jntStereoCbrMinBitrate", p->getQualThresholds().m_nJointStereoCbr);
        m_pSettings->setValue("quality/dualChnlCbrMinBitrate", p->getQualThresholds().m_nDoubleChannelCbr);

        m_pSettings->setValue("quality/stereoVbrMinBitrate", p->getQualThresholds().m_nStereoVbr);
        m_pSettings->setValue("quality/jntStereoVbrMinBitrate", p->getQualThresholds().m_nJointStereoVbr);
        m_pSettings->setValue("quality/dualChnlVbrMinBitrate", p->getQualThresholds().m_nDoubleChannelVbr);
    }

    { // ID3V2 transf
        m_pSettings->setValue("id3V2Transf/locale", p->m_locale);

        m_pSettings->setValue("id3V2Transf/caseForArtists", (int)p->m_eCaseForArtists);
        m_pSettings->setValue("id3V2Transf/caseForOthers", (int)p->m_eCaseForOthers);
    }

    { // tag editor
        m_pSettings->setValue("tagEditor/warnOnNonSeqTracks", p->m_bWarnOnNonSeqTracks);
        m_pSettings->setValue("tagEditor/warnOnPasteToNonSeqTracks", p->m_bWarnPastingToNonSeqTracks);
        m_pSettings->setValue("tagEditor/saveAssigned", (int)p->m_eAssignSave);
        m_pSettings->setValue("tagEditor/saveNonId3v2", (int)p->m_eNonId3v2Save);
        m_pSettings->setValue("tagEditor/maxImageSize", ImageInfo::MAX_IMAGE_SIZE);
    }

    { // misc
        m_pSettings->setValue("main/showDebug", p->m_bShowDebug);
        m_pSettings->setValue("main/showSessions", p->m_bShowSessions);
        m_pSettings->setValue("normalizer/command", convStr(p->m_strNormalizeCmd));
        m_pSettings->setValue("main/keepNormWndOpen", p->m_bKeepNormWndOpen);

        m_pSettings->setValue("debug/enableTracing", p->m_bTraceEnabled);
        m_pSettings->setValue("debug/useAllNotes", p->m_bUseAllNotes);
        m_pSettings->setValue("debug/logTransf", p->m_bLogTransf);
        m_pSettings->setValue("debug/saveDownloadedData", p->m_bSaveDownloadedData);
        m_pSettings->setValue("main/autoSizeIcons", p->m_bAutoSizeIcons);
        m_pSettings->setValue("main/keepOneValidImg", p->m_bKeepOneValidImg);
        m_pSettings->setValue("main/fastSave", p->useFastSave());
        m_pSettings->setValue("debug/traceToFile", p->isTraceToFileEnabled());

        QFont genFnt (p->getNewGeneralFont());
        m_pSettings->setValue("main/generalFontName", genFnt.family());
        m_pSettings->setValue("main/generalFontSize", genFnt.pointSize());
        m_pSettings->setValue("main/labelFontSizeDecr", p->getLabelFontSizeDecr());
        QFont fixedFnt (p->getNewFixedFont());
        m_pSettings->setValue("main/fixedFontName", fixedFnt.family());
        m_pSettings->setValue("main/fixedFontSize", fixedFnt.pointSize());

        m_pSettings->setValue("main/warnedAboutSel", p->m_bWarnedAboutSel);
        m_pSettings->setValue("main/warnedAboutBackup", p->m_bWarnedAboutBackup);
        m_pSettings->setValue("tagEditor/toldAboutPatterns", p->m_bToldAboutPatterns);
        m_pSettings->setValue("main/toldAboutSupport", p->m_bToldAboutSupport);

        m_pSettings->setValue("fileRenamer/invalidChars", convStr(p->m_strRenamerInvalidChars));
        m_pSettings->setValue("fileRenamer/replacementForInvalid", convStr(p->m_strRenamerReplacementString));

        m_pSettings->setValue("main/checkForNewVersions", convStr(p->m_strCheckForNewVersions));
        m_pSettings->setValue("main/timeLastNewVerCheck", p->m_timeLastNewVerCheck);
        m_pSettings->setValue("main/dontTellAboutVer", convStr(p->m_strDontTellAboutVer));
    }

    { // note categ colors
        vector<string> v;
        for (int i = 0; i < Note::CATEG_CNT; ++i)
        {
            QColor c (p->m_vNoteCategColors[i]);
            char a [20];
            sprintf(a, "%d %d %d", c.red(), c.green(), c.blue());
            v.push_back(a);
        }
        p->m_settings.saveVector("categories/colors", v);
    }

    { // tag edt colors
        vector<string> v;
        for (int i = 0; i < CommonData::COLOR_COL_CNT; ++i)
        {
            QColor c (p->m_vTagEdtColors[i]);
            char a [20];
            sprintf(a, "%d %d %d", c.red(), c.green(), c.blue());
            v.push_back(a);
        }
        p->m_settings.saveVector("tagEditor/colors", v);
    }
}


static bool isWhite(const QColor& c)
{
    //qDebug("%d %d %d", c.red(), c.green(), c.blue());
    return c.red() >= 254 && c.green() >= 254 && c.blue() >= 254;
}


void SessionSettings::loadMiscConfigSettings(CommonData* p) const
{
    { // quality
        QualThresholds q;
        q.m_nStereoCbr = m_pSettings->value("quality/stereoCbrMinBitrate", 192000).toInt();
        q.m_nJointStereoCbr = m_pSettings->value("quality/jntStereoCbrMinBitrate", 192000).toInt();
        q.m_nDoubleChannelCbr = m_pSettings->value("quality/dualChnlCbrMinBitrate", 192000).toInt();

        q.m_nStereoVbr = m_pSettings->value("quality/stereoVbrMinBitrate", 170000).toInt();
        q.m_nJointStereoVbr = m_pSettings->value("quality/jntStereoVbrMinBitrate", 160000).toInt();
        q.m_nDoubleChannelVbr = m_pSettings->value("quality/dualChnlVbrMinBitrate", 180000).toInt();

        p->setQualThresholds(q);
    }


    { // ID3V2 transf
        p->m_locale = m_pSettings->value("id3V2Transf/locale", "ISO 8859-1").toByteArray();
        p->m_pCodec = (QTextCodec::codecForName(p->m_locale));
        if (0 == p->m_pCodec)
        {
            QList<QByteArray> l (QTextCodec::availableCodecs());
            CB_ASSERT (l.size() > 0);
            p->m_locale = l.front();
            p->m_pCodec = (QTextCodec::codecForName(p->m_locale));
        }
        CB_ASSERT (0 != p->m_pCodec);

        p->m_eCaseForArtists = (CommonData::Case)m_pSettings->value("id3V2Transf/caseForArtists", 2).toInt();
        p->m_eCaseForOthers = (CommonData::Case)m_pSettings->value("id3V2Transf/caseForOthers", 3).toInt();
    }

    { // tag editor
        p->m_bWarnOnNonSeqTracks = m_pSettings->value("tagEditor/warnOnNonSeqTracks", true).toBool();
        p->m_bWarnPastingToNonSeqTracks = m_pSettings->value("tagEditor/warnOnPasteToNonSeqTracks", true).toBool();
        { int k (m_pSettings->value("tagEditor/saveAssigned", 2).toInt()); if (k < 0 || k > 2) { k = 2; } p->m_eAssignSave = CommonData::Save(k); }
        { int k (m_pSettings->value("tagEditor/saveNonId3v2", 2).toInt()); if (k < 0 || k > 2) { k = 2; } p->m_eNonId3v2Save = CommonData::Save(k); }
        ImageInfo::MAX_IMAGE_SIZE = m_pSettings->value("tagEditor/maxImageSize", 102400).toInt();
    }

    { // misc
        p->m_bShowDebug = m_pSettings->value("main/showDebug", false).toBool();
        p->m_bShowSessions = m_pSettings->value("main/showSessions", !p->isUniqueSession()).toBool();
        p->m_strNormalizeCmd = convStr(m_pSettings->value("normalizer/command", "mp3gain -a -k -p -t").toString());
        p->m_bKeepNormWndOpen = m_pSettings->value("main/keepNormWndOpen", false).toBool();

        p->m_bTraceEnabled = m_pSettings->value("debug/enableTracing", false).toBool();
        p->m_bUseAllNotes = m_pSettings->value("debug/useAllNotes", false).toBool();
        p->m_bLogTransf = m_pSettings->value("debug/logTransf", false).toBool();
        p->m_bSaveDownloadedData = m_pSettings->value("debug/saveDownloadedData", false).toBool();
        p->m_bAutoSizeIcons = m_pSettings->value("main/autoSizeIcons", true).toBool();
        p->m_bKeepOneValidImg = m_pSettings->value("main/keepOneValidImg", false).toBool();
        p->setFastSave(m_pSettings->value("main/fastSave", false).toBool(), CommonData::DONT_UPDATE_TRANSFORMS);
        p->setTraceToFile(m_pSettings->value("debug/traceToFile", false).toBool());

        QFont fnt;
        //qDebug("%d ==========================", fnt.pointSize());
        QFontInfo inf1 (QFont(m_pSettings->value("main/generalFontName", "SansSerif").toString(), m_pSettings->value("main/generalFontSize", fnt.pointSize()).toInt())); // ttt2 try and get the system defaults
        QFontInfo inf2 (QFont(m_pSettings->value("main/fixedFontName", "Courier").toString(), m_pSettings->value("main/fixedFontSize", fnt.pointSize()).toInt()));
        p->setFontInfo(convStr(inf1.family()), inf1.pointSize(), m_pSettings->value("main/labelFontSizeDecr", 0).toInt(), convStr(inf2.family()), inf2.pointSize());

        p->m_bWarnedAboutSel = m_pSettings->value("main/warnedAboutSel", false).toBool();
        p->m_bWarnedAboutBackup = m_pSettings->value("main/warnedAboutBackup", false).toBool();
        p->m_bToldAboutPatterns = m_pSettings->value("tagEditor/toldAboutPatterns", false).toBool();
        p->m_bToldAboutSupport = m_pSettings->value("main/toldAboutSupport", false).toBool();

#ifndef WIN32
        const char* DEFAULT_INVALID ("/\"\\*?<>|"); //ttt2 even in Unix, it might be a good idea to not allow ":" as well in some cases, depending on the file system
#else
        const char* DEFAULT_INVALID ("/\"\\*?<>|:"); //ttt2 perhaps have yellow background instead of underline, to be able to tell if there's a space or not; or have something like a progres bar below; or have each letter in its own rectangle;
#endif
        p->m_strRenamerInvalidChars = convStr(m_pSettings->value("fileRenamer/invalidChars", DEFAULT_INVALID).toString());
        p->m_strRenamerReplacementString = convStr(m_pSettings->value("fileRenamer/replacementForInvalid", "_").toString());

        p->m_strCheckForNewVersions = convStr(m_pSettings->value("main/checkForNewVersions", "").toString());
        p->m_timeLastNewVerCheck = m_pSettings->value("main/timeLastNewVerCheck").toDateTime();
        if (p->m_timeLastNewVerCheck.isNull())
        {
            QDateTime t1 (QDateTime::currentDateTime());
            p->m_timeLastNewVerCheck = t1.addYears(-1);
        }
        p->m_strDontTellAboutVer = convStr(m_pSettings->value("main/dontTellAboutVer", "").toString());
    }

    { // note categ colors
        //m_settings.saveVector("categories/colors", u);
        bool bErr;
        vector<string> v (loadVector("categories/colors", bErr));
        int i (0);
        for (; i < Note::CATEG_CNT && i < cSize(v); ++i)
        {
            int r (230), g (230), b (230);
            istringstream in (v[i]);
            in >> r >> g >> b;
            p->m_vNoteCategColors.push_back(QColor(r, g, b));
        }

        QColor c (getDefaultBkgCol());

        for (; i < Note::CATEG_CNT; ++i)
        {
            //p->m_vNoteCategColors.push_back(QColor(240, 240 + i, 240 - i));
            p->m_vNoteCategColors.push_back(c);
        }
    }

    { // tag edt colors
        bool bErr;
        vector<string> v (loadVector("tagEditor/colors", bErr));

        for (int i = 0; i < CommonData::COLOR_COL_CNT && i < cSize(v); ++i)
        {
            int r (230), g (230), b (230);
            istringstream in (v[i]);
            in >> r >> g >> b;
            p->m_vTagEdtColors.push_back(QColor(r, g, b));
        }

        QColor defNormal (QPalette().color(QPalette::Active, QPalette::Base));
        if (cSize(p->m_vTagEdtColors) <= 0) { p->m_vTagEdtColors.push_back(defNormal); } // COLOR_ALB_NORM
        if (cSize(p->m_vTagEdtColors) <= 1) { p->m_vTagEdtColors.push_back(QColor(0xffffdd)); } // COLOR_ALB_NONID3V2
        if (cSize(p->m_vTagEdtColors) <= 2) { p->m_vTagEdtColors.push_back(QColor(0xccccff)); } // COLOR_ALB_ASSIGNED
        if (cSize(p->m_vTagEdtColors) <= 3) { p->m_vTagEdtColors.push_back(QColor(defNormal)); } // COLOR_FILE_NORM
        if (cSize(p->m_vTagEdtColors) <= 4) { p->m_vTagEdtColors.push_back(QColor(0xdddddd)); } // COLOR_FILE_TAG_MISSING
        if (cSize(p->m_vTagEdtColors) <= 5) { p->m_vTagEdtColors.push_back(QColor(0xf0f0ff)); } // COLOR_FILE_NA
        if (cSize(p->m_vTagEdtColors) <= 6) { p->m_vTagEdtColors.push_back(QColor(0xffffdd)); } // COLOR_FILE_NO_DATA
    }
}


// adjusts the global font so it displays legible characters (on Windows it is possible under some unclear circumstances for all characters to be shown as small rectangles)
void fixAppFont(QFont& font, string& strNewFont, int& nNewSize)
{
    const char* aszFonts[] = { "Arial", "Helvetica", "Sans", "Courier", 0 };
    for (int i = 0; ; ++i)
    {
        const int SIZE (50);
        QImage img1 (SIZE, SIZE, QImage::Format_RGB32);
        QImage img2 (SIZE, SIZE, QImage::Format_RGB32);

        {
            QPainter pntr (&img1);
            pntr.fillRect(0, 0, SIZE, SIZE, QColor(255, 255, 255));
            pntr.drawText(5, SIZE/2 + 10, "ab");
        }

        {
            QPainter pntr (&img2);
            pntr.fillRect(0, 0, SIZE, SIZE, QColor(255, 255, 255));
            pntr.drawText(5, SIZE/2 + 10, "cd");
            //pntr.drawText(5, SIZE/2 + 10, "ab");
        }

        if (img1 != img2)
        {
            break;
        }

        qDebug("invalid font %s", strNewFont.c_str());
        const char* szFont (aszFonts[i]);
        if (0 == szFont)
        {
            exit(1); // don't know what else to do
        }

        strNewFont = szFont;
        if (nNewSize < 5 || nNewSize > 20) { nNewSize = 9; };
        font.setFamily(convStr(strNewFont));
        font.setPointSize(nNewSize);
        QApplication::setFont(font);
    }
}

static int MIN_FILE_WIDTH; // minimum width of the "file" field

void CommonData::setFontInfo(const std::string& strGenName, int nGenSize, int nLabelFontSizeDecr, const std::string& strFixedName, int nFixedSize)
{
    if (nGenSize < 5 || nGenSize > 20) { nGenSize = 9; }
    if (nFixedSize < 5 || nFixedSize > 20) { nFixedSize = 9; }
    if (nLabelFontSizeDecr < 0 || nLabelFontSizeDecr > 5) { nLabelFontSizeDecr = 0; }

    if (m_strGenFontName == strGenName && m_nGenFontSize == nGenSize && m_nLabelFontSizeDecr == nLabelFontSizeDecr && m_strFixedFontName == strFixedName && m_nFixedFontSize == nFixedSize) { return; } // nothing changed

    bool bFirstTime (m_strGenFontName.empty());
    m_strGenFontName = strGenName;
    m_nGenFontSize = nGenSize;
    m_nLabelFontSizeDecr = nLabelFontSizeDecr;
    m_strFixedFontName = strFixedName;
    m_nFixedFontSize = nFixedSize;

    if (!bFirstTime)
    {
        QMessageBox::warning(m_pFilesG, "Info", "The font changes will only be used after restarting the application."); //ttt1 try to get this work, probably needs to call QHeaderView::resizeSection(), as well as review all setMinimumSectionSize() and setDefaultSectionSize() calls;
        return;
    }

    m_generalFont.setFamily(convStr(strGenName));
    m_generalFont.setPointSize(nGenSize);
    QApplication::setFont(m_generalFont);

    fixAppFont(m_generalFont, m_strGenFontName, m_nGenFontSize);

    m_labelFont = m_generalFont;
    m_labelFont.setPointSize(nGenSize - nLabelFontSizeDecr);

    m_fixedFont.setFamily(convStr(strFixedName));
    //m_fixedFont.setFamily("B&H LucidaTypewriter");
    //m_fixedFont.setFamily("B&H LucidaTypewriter12c");
    //m_fixedFont.setFixedPitch(true);  // !!! needed to select some fixed font in case there's no "B&H LucidaTypewriter" installed
    m_fixedFont.setFixedPitch(true);
    m_fixedFont.setStyleHint(QFont::Courier);
    //f.setPixelSize(12); //ttt2 hard-coded "12"; //ttt1 some back-up font instead of just letting the system decide
    m_fixedFont.setPointSize(nFixedSize);


    CELL_HEIGHT = QApplication::fontMetrics().height() + 3;

    //qDebug("wdth %d", QApplication::fontMetrics().width("W"));
    //qDebug("wdth %d", QApplication::fontMetrics().maxWidth()); // too big, probably from non-ASCII-letter chars
    /*int n (QApplication::fontMetrics().width("W")); // ttt1 switch to W', M' and m' when needed
    n = max(n, QApplication::fontMetrics().width("M"));
    n = max(n, QApplication::fontMetrics().width("m"));
#ifndef WIN32
    CELL_WIDTH = n + 5;
#else
    CELL_WIDTH = n + 6; //ttt2 hard-coded "6"; see how to get the correct value
#endif*/

    {
        QPixmap img (100, 100); //ttt2 revisit the size; might need increase in the future
        QPainter pntr (&img);
        QFont f (m_labelFont);
        f.setWeight(QFont::Bold);
        pntr.setFont(f);
        int n (0);
        for (char c1 = 'a'; c1 <= 'z'; ++c1) //ttt3 ASCII-specific
        {
            for (char c2 = 'a'; c2 <= 'z'; ++c2)
            {
                if (c1 != 'm' && c1 != 'w' && c2 != 'm' && c2 != 'w')
                {
                    QString s;
                    s += c1;
                    s += c2;
                    QRect r (10, 10, 100, 100);
                    r = pntr.boundingRect(r, Qt::AlignLeft | Qt::AlignTop, s);
                    //CB_ASSERT (10 == r.x());
                    n = max(n, r.width());
                }
            }
            //qDebug("%s - %d %d %2d %2d", s.toUtf8().data(), r.x(), r.y(), r.width(), r.height());
        }

        CELL_WIDTH = n + 4; //ttt2 hard-coded "4"; see how to get the correct value
    }

    MIN_FILE_WIDTH = QApplication::fontMetrics().width("ABCDEFGHIJKLMNopqrstuvwxyz12345");


    /*m_pFilesG->horizontalHeader()->setMinimumSectionSize(CELL_WIDTH);
    m_pFilesG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
    m_pFilesG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);*/
}


QFont CommonData::getNewGeneralFont() const
{
    QFont f;
    f.setFamily(convStr(m_strGenFontName));
    f.setPointSize(m_nGenFontSize);
    QFontInfo info (f);
    return QFont (info.family(), info.pointSize());
}

QFont CommonData::getNewFixedFont() const
{
    QFont f;
    f.setFamily(convStr(m_strFixedFontName));
    f.setPointSize(m_nFixedFontSize);
    f.setStyleHint(QFont::Courier);
    f.setFixedPitch(true);
    QFontInfo info (f);
    return QFont (info.family(), info.pointSize());
}



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================



CommonData::CommonData(
        SessionSettings& settings,
        QTableView* pFilesG,
        QTableView* pNotesG,
        QTableView* pStreamsG,

        QTableView* pUniqueNotesG,

        QToolButton* pNoteFilterB,
        QToolButton* pDirFilterB,
        QToolButton* pModeAllB,
        QToolButton* pModeAlbumB,
        QToolButton* pModeSongB,

        bool bUniqueSession) :

        m_pFilesModel(0),
        m_pNotesModel(0),
        m_pStreamsModel(0),

        m_pUniqueNotesModel(0),

        m_pFilesG(pFilesG),
        m_pNotesG(pNotesG),
        m_pStreamsG(pStreamsG),

        m_pUniqueNotesG(pUniqueNotesG),

        m_bChangeGuard(false),
        m_pCodec(0),
        m_bUseAllNotes(false),
        m_bTraceEnabled(false),
        m_bAutoSizeIcons(true),
        m_nMainWndIconSize(40),
        m_settings(settings),
        m_bKeepOneValidImg(false),
        m_bLogTransf(false),
        m_bSaveDownloadedData(false),
        m_vvnCustomTransf(CUSTOM_TRANSF_CNT),
        //m_bDirty(false),

        m_nLabelFontSizeDecr(0),
        m_bUniqueSession(bUniqueSession),
        m_bFastSave(false),

        m_eViewMode(ALL),
        m_pNoteFilterB(pNoteFilterB),
        m_pDirFilterB(pDirFilterB),
        m_pModeAllB(pModeAllB),
        m_pModeAlbumB(pModeAlbumB),
        m_pModeSongB(pModeSongB)
{
    m_vpAllTransf.push_back(new SingleBitRepairer());
    m_vpAllTransf.push_back(new InnerNonAudioRemover());

    m_vpAllTransf.push_back(new UnknownDataStreamRemover());
    m_vpAllTransf.push_back(new BrokenDataStreamRemover());
    m_vpAllTransf.push_back(new UnsupportedDataStreamRemover());
    m_vpAllTransf.push_back(new TruncatedMpegDataStreamRemover());
    m_vpAllTransf.push_back(new NullStreamRemover());

    m_vpAllTransf.push_back(new BrokenId3V2Remover());
    m_vpAllTransf.push_back(new UnsupportedId3V2Remover());

    m_vpAllTransf.push_back(new IdentityTransformation());

    m_vpAllTransf.push_back(new MultipleId3StreamRemover());
    m_vpAllTransf.push_back(new MismatchedXingRemover());

    m_vpAllTransf.push_back(new TruncatedAudioPadder());

    m_vpAllTransf.push_back(new VbrRepairer());
    m_vpAllTransf.push_back(new VbrRebuilder());

    m_vpAllTransf.push_back(new Id3V2Cleaner(this));
    m_vpAllTransf.push_back(new Id3V2Rescuer(this));
    m_vpAllTransf.push_back(new Id3V2UnicodeTransformer(this));
    m_vpAllTransf.push_back(new Id3V2CaseTransformer(this));


    m_vpAllTransf.push_back(new Id3V2ComposerAdder(this));
    m_vpAllTransf.push_back(new Id3V2ComposerRemover(this));
    m_vpAllTransf.push_back(new Id3V2ComposerCopier(this));

    m_vpAllTransf.push_back(new SmallerImageRemover(this));
    m_vpAllTransf.push_back(new Id3V1ToId3V2Copier(this));
    m_vpAllTransf.push_back(new Id3V1Remover());

    m_vpAllTransf.push_back(new Id3V2Expander(this));
    m_vpAllTransf.push_back(new Id3V2Compactor(this));

    m_settings.loadDirs(m_vstrIncludeDirs, m_vstrExcludeDirs);

    try
    {
        m_dirTreeEnum.reset(m_vstrIncludeDirs, m_vstrExcludeDirs);
    }
    catch (const DirTreeEnumerator::InvalidDirs&)
    {
        QMessageBox::critical(m_pFilesG, "Error", "There was an error setting up the directories containing MP3 files. You will have to define them again.");
        m_vstrIncludeDirs.clear();
        m_vstrExcludeDirs.clear();
    }

    connect(&m_filter, SIGNAL(filterChanged()), this, SLOT(onFilterChanged()));
}


CommonData::~CommonData()
{
    clearPtrContainer(m_vpAllHandlers);
    clearPtrContainer(m_vpAllTransf);
}




// finds the position of a note in the global vector with notes sorted by severity and description;
// the position can then be used to find the corresponding label;
// returns -1 if the note is not found; (needed because trace and info notes aren't shown in the grid, so they are not found)
int CommonData::findPos(const Note* pNote) const
{
    return m_uniqueNotes.getFltPos(pNote);
}


// for debugging; prints what's current in m_pFilesG, using several methods
void CommonData::printFilesCrt() const
{
    QModelIndex x (m_pFilesG->selectionModel()->currentIndex());
    QModelIndex y (m_pFilesG->currentIndex());
    printf("SelModel: %dx%d  View: %dx%d  getFilesGCrtRow(): %d  vector.size(): %d\n", x.row(), x.column(), y.row(), y.column(), getFilesGCrtRow(),
        cSize(m_vpAllHandlers));//*/
}



const Mp3Handler* CommonData::getCrtMp3Handler() const // returns 0 if the list is empty
{
    int n (getFilesGCrtRow());
    return -1 == n ? 0 : m_vpViewHandlers[n];
}

const string& CommonData::getCrtName() const
{
    const Mp3Handler* p (getCrtMp3Handler());
    if (0 != p) { return p->getName(); }
    static string strEmpty;
    return strEmpty;
}


const vector<DataStream*>& CommonData::getCrtStreams() const
{
    int n (getFilesGCrtRow());
    //static int yy; qDebug("row %d %d", n, yy++);
    static std::vector<DataStream*> s_vEmpty;
    if (-1 == n) { return s_vEmpty; }
    return m_vpViewHandlers.at(n)->getStreams();
}



// copies to spUniqueNotes all the unique notes from commonData.m_vpFltHandlers, with comparison done by CmpNotePtrById;
// ignored notes (given by m_vnIgnoredNotes) are not included;
// not actual pointers from vpHandlers are stored, but the corresponding ones from given by Notes::getMaster(); so they must not be freed;
void CommonData::getUniqueNotes(const std::deque<const Mp3Handler*>& vpHandlers, set<const Note*, CmpNotePtrById>& spUniqueNotes)
{
    set<int> snIgnored (m_vnIgnoredNotes.begin(), m_vnIgnoredNotes.end());
    for (int i = 0; i < cSize(vpHandlers); ++i)
    {
        const Mp3Handler* pHndl (vpHandlers[i]);
        const vector<Note*>& vpNotes (pHndl->getNotes().getList());
        for (int j = 0, n = cSize(vpNotes); j < n; ++j)
        {
            const Note* pNote (vpNotes[j]);
            if (Note::ERR == pNote->getSeverity() || Note::WARNING == pNote->getSeverity() || Note::SUPPORT == pNote->getSeverity())
            {
                if (0 == snIgnored.count(pNote->getNoteId()))
                { // not an ignored note, so it can be added;
                    spUniqueNotes.insert(Notes::getMaster(pNote)); // most of the time the note already exists in spUniqueNotes, but that's OK: it doesn't take a lot of time and the pointers are not owned;
                }
            }
        }
    }
}




// although the current elem can be identified (so it "shouldn't" be passed) and most of the time pMp3Handler will be just that, sometimes it will deliberately be 0, so a param is actually needed;
void CommonData::setViewMode(ViewMode eViewMode, const Mp3Handler* pMp3Handler /*= 0*/)
{
    int nRes (setViewModeHlp(eViewMode, pMp3Handler));
    m_pFilesModel->selectRow(nRes);
    switch (eViewMode)
    {
    case ALL: m_pModeAllB->setChecked(true); break;
    case FOLDER: m_pModeAlbumB->setChecked(true); break;
    case FILE: m_pModeSongB->setChecked(true); break;
    default: CB_ASSERT (false);
    }
    resizeFilesGCols();
}


// returns the position of the "current" elem in m_vpViewHandlers (so it can be selected in the grid); if m_vpViewHandlers is empty, it returns -1; if pMp3Handler is 0, it returns 0 (unless m_vpViewHandlers is empty);
int CommonData::setViewModeHlp(ViewMode eViewMode, const Mp3Handler* pMp3Handler)
{
    m_eViewMode = eViewMode;

    if (m_vpFltHandlers.empty()) { m_vpViewHandlers.clear(); return -1; }

    switch (eViewMode)
    {
    case ALL: m_vpViewHandlers = m_vpFltHandlers; return 0 == pMp3Handler ? 0 : getPosInFlt(pMp3Handler);

    case FILE:
        {
            m_vpViewHandlers.clear();
            m_vpViewHandlers.push_back(0 == pMp3Handler ? m_vpFltHandlers[0] : pMp3Handler);
            return 0;
        }

    case FOLDER:
        {
            m_vpViewHandlers.clear();
            if (0 == pMp3Handler) { pMp3Handler = m_vpFltHandlers[0]; }
            string strDir (pMp3Handler->getDir());
            int nRes (-1);
            for (int i = 0, n = cSize(m_vpFltHandlers); i < n; ++i)
            {
                const Mp3Handler* p (m_vpFltHandlers[i]);
                if (p->getDir() == strDir)
                {
                    if (p == pMp3Handler)
                    {
                        nRes = cSize(m_vpViewHandlers);
                    }
                    m_vpViewHandlers.push_back(p);
                }
            }

            CB_ASSERT (nRes >= 0);
            return nRes;
        }
    }

    CB_ASSERT(false);
}



void CommonData::next()
{
    int k (nextHlp());
    m_pFilesModel->selectRow(k);
    resizeFilesGCols();
}



// returns the position of the "current" elem in m_vpViewHandlers (see setViewMode() for details)
int CommonData::nextHlp()
{
    if (m_vpViewHandlers.empty()) { return -1; }

    switch (m_eViewMode)
    {
    case ALL:
        { // move to beginning of next folder
            int i (getFilesGCrtRow());
            int n (cSize(m_vpViewHandlers));
            string strDir (m_vpViewHandlers[i]->getDir());
            for (; i < n - 1; ++i)
            {
                if (m_vpViewHandlers[i]->getDir() != strDir)
                {
                    return i;
                }
            }
            return n - 1;
        } //ttt2 the layout doesn't change for ALL, but it does for FILE and FOLDER, so perhaps a means to tell if the layout changed or not is needed

    case FILE:
        { // next file
            CB_ASSERT(1 == cSize(m_vpViewHandlers));
            int i (getPosInFlt(getCrtMp3Handler()));
            int n (cSize(m_vpFltHandlers));
            if (n - 1 == i) { return 0; }

            ++i;
            m_vpViewHandlers[0] = m_vpFltHandlers[i];
            return 0;
        }

    case FOLDER:
        { // next folder
            int i (getPosInFlt(getCrtMp3Handler()));
            int n (cSize(m_vpFltHandlers));
            if (n - 1 == i) { return cSize(m_vpViewHandlers) - 1; }

            string strDir (m_vpFltHandlers[i]->getDir());
            ++i;
            for (; i < n; ++i)
            {
                if (m_vpFltHandlers[i]->getDir() != strDir)
                {
                    break;
                }
            }
            if (n == i) { return cSize(m_vpViewHandlers) - 1; } // it was the last dir; don't change m_vpViewHandlers, just move to the end

            // new folder found
            m_vpViewHandlers.clear();
            strDir = m_vpFltHandlers[i]->getDir();
            while (strDir == m_vpFltHandlers[i]->getDir())
            {
                m_vpViewHandlers.push_back(m_vpFltHandlers[i]);
                ++i;
                if (i >= n) { break; }
            }
            return 0;
        }
    }

    CB_ASSERT(false);
}


void CommonData::previous()
{
    int k (previousHlp());
    m_pFilesModel->selectRow(k);
    resizeFilesGCols();
}


// returns the position of the "current" elem in m_vpViewHandlers (see setViewMode() for details)
int CommonData::previousHlp()
{
    if (m_vpViewHandlers.empty()) { return -1; }

    switch (m_eViewMode)
    {
    case ALL:
        { // move to beginning of previous folder
            int i (getFilesGCrtRow());
            string strDir (m_vpViewHandlers[i]->getDir());
            for (; i > 0; --i)
            {
                if (m_vpViewHandlers[i]->getDir() != strDir)
                {
                    return i;
                }
            }
            return 0;
        }

    case FILE:
        { // previous file
            CB_ASSERT(1 == cSize(m_vpViewHandlers));
            int i (getPosInFlt(getCrtMp3Handler()));
            if (0 == i) { return 0; }

            --i;
            m_vpViewHandlers[0] = m_vpFltHandlers[i];
            return 0;
        }

    case FOLDER:
        { // previous folder
            int i (getPosInFlt(getCrtMp3Handler()));
            if (0 == i) { return 0; }

            string strDir (m_vpFltHandlers[i]->getDir());
            --i;
            for (; i >= 0; --i)
            {
                if (m_vpFltHandlers[i]->getDir() != strDir)
                {
                    break;
                }
            }
            if (-1 == i) { return 0; } // it was the first dir; don't change m_vpViewHandlers, just move to the beginning

            // new folder found
            m_vpViewHandlers.clear();
            strDir = m_vpFltHandlers[i]->getDir();
            while (strDir == m_vpFltHandlers[i]->getDir())
            {
                m_vpViewHandlers.push_front(m_vpFltHandlers[i]); // push_front() is OK for a deque
                --i;
                if (i < 0) { break; }
            }
            return 0;
        }
    }

    CB_ASSERT(false);
}


// finds the position in m_vpFltHandlers; returns -1 if not found; 0 is a valid argument, in which case -1 is returned; needed by next() and previous(), to help with navigation when going into "folder" mode
int CommonData::getPosInFlt(const Mp3Handler* pMp3Handler) const
{
    if (0 == pMp3Handler) { return -1; }
    /*deque<Mp3Handler*>::const_iterator it (find(m_vpFltHandlers.begin(), m_vpFltHandlers.end(), pMp3Handler));
    if (m_vpFltHandlers.end() == it) { return -1; }*/
    deque<const Mp3Handler*>::const_iterator it (lower_bound(m_vpFltHandlers.begin(), m_vpFltHandlers.end(), pMp3Handler, CmpMp3HandlerPtrByName()));
    if (m_vpFltHandlers.end() == it || *it != pMp3Handler) { return -1; }

    int n (it - m_vpFltHandlers.begin());
    return n;
}


void CommonData::onCrtFileChanged()
{
    updateCurrentNotes();
    updateCurrentStreams();
}

// updates m_vpCrtNotes, to hold the notes corresponding to the current file and resizes the rows to fit the data
void CommonData::updateCurrentNotes()
{
    int nRow (getFilesGCrtRow());
    CB_ASSERT (nRow >= -1 && nRow < cSize(getViewHandlers()));

    m_vpCrtNotes.clear();
    if (-1 == nRow)
    { // an empty file list
        m_pNotesModel->emitLayoutChanged();
        return;
    }

    const vector<Note*> vNotes (getViewHandlers()[nRow]->getNotes().getList());

    for (int i = 0, n = cSize(vNotes); i < n; ++i)
    {
        const Note* pNote (vNotes[i]);
        if (m_bUseAllNotes || findPos(pNote) >= 0) // findPos(pNote) is -1 for TRACE as well as for ignored notes
        {
            m_vpCrtNotes.push_back(pNote);
        }
    }

    std::sort(m_vpCrtNotes.begin(), m_vpCrtNotes.end(), CmpNotePtrByPosAndId()); // the list may come sorted by some other criteria, e.g. severity first,

    //reset(); //ttt2 see what is the difference between "emit layoutChanged()" and "reset()"; here, they are similar now, but in a prior version calling "reset()" didn't show the selection (this changed after implementing "NotesModel::matchSelToMain()" but as of 2008.06.30 even that seems unncecessary
    m_pNotesModel->emitLayoutChanged();

    //m_pNotesG->resizeColumnsToContents(); //ttt2 see why uncommenting this results in column 0 (and perhaps 2) increasing their size each time NotesModel::updateCurrentNotes() gets called
    m_pNotesG->resizeRowsToContents();
}


void CommonData::updateCurrentStreams()
{
    int nRow (getFilesGCrtRow());
    CB_ASSERT (nRow >= -1 && nRow < cSize(getViewHandlers()));

    m_pStreamsModel->emitLayoutChanged();
    m_pStreamsG->resizeRowsToContents();
}


int CommonData::getFilesGCrtRow() const // returns -1 if no current element exists (e.g. because the table is empty)
{
    QModelIndex index (m_pFilesG->currentIndex()); // the documentation for QTableView isn't very clear, but in the code it just calls QItemSelectionModel::currentIndex(), for which the documentation says that it returns an ivalid index if there's no current element
    if (!index.isValid()) { return - 1; } // not sure if this is NECESSARY; for an empty table -1 is returned anyway as index.row(); but it doesn't hurt, anyway
    return index.row();
}

int CommonData::getFilesGCrtCol() const // returns -1 if no current element exists (e.g. because the table is empty)
{
    QModelIndex index (m_pFilesG->currentIndex());
    if (!index.isValid()) { return - 1; } // not sure if this is NECESSARY; for an empty table -1 is returned anyway as index.column(); but it doesn't hurt, anyway
    return index.column();
}



#if 0
struct AAA
{
    int a;
};

struct Cmp
{
    /*bool operator()(int x1, int x2) const
    {
        return x1 < x2;
    }*/

    bool operator()(AAA* a1, int x2) const // ttt2 review if the fact that this code compiles (without requiring any of the other operators) is due to the standard or to GCC's STL; probably it's standard, because we don't need an equality test for lower_bound
    {
        //return operator()(a1->a, x2);
        return true;
    }

    /*bool operator()(int x1, AAA* a2) const
    {
        return operator()(x1, a2->a);
    }

    bool operator()(AAA* a1, AAA* a2) const
    {
        return operator()(a1->a, a2->a);
    }*/
};

void opppo()
{
    vector<AAA*> v;
    lower_bound(v.begin(), v.end(), 7, Cmp());
}
#endif

// if bExactMatch is false, it finds the nearest position in m_vpViewHandlers (so even if a file was deleted, it still finds something close);
// returns -1 only if not found (either m_vpViewHandlers is empty or bExactMatch is true and the file is missing);
int CommonData::getPosInView(const std::string& strName/*, bool bUsePrevIfNotFound = true*/, bool bExactMatch /*= false*/) const
{
/*    if (strName.empty()) { return -1; }
    deque<Mp3Handler*>::const_iterator it (lower_bound(m_vpViewHandlers.begin(), m_vpViewHandlers.end(), strName, CmpMp3HandlerPtrByName()));
    if (m_vpViewHandlers.end() == it)
    {
        if (m_vpViewHandlers.empty())
        {
            return -1;
        }
        --it;
    }
    else if (bUsePrevIfNotFound && m_vpViewHandlers.begin() != it && (*it)->getName() != strName) // position on previous elem if exact match wasn't found
    {
        --it;
    }

    int n (it - m_vpViewHandlers.begin());
    return n;*/

    deque<const Mp3Handler*>::const_iterator it (lower_bound(m_vpViewHandlers.begin(), m_vpViewHandlers.end(), strName, CmpMp3HandlerPtrByName()));
    if ((m_vpViewHandlers.end() == it || (*it)->getName() != strName) && (bExactMatch || m_vpViewHandlers.empty()))
    {
        return -1;
    }

    if (m_vpViewHandlers.end() == it)
    {
        --it;
    }

    int n (it - m_vpViewHandlers.begin());
    return n;
}



void CommonData::updateSelList()
{
    QModelIndexList lSelFiles (m_pFilesG->selectionModel()->selection().indexes());

    set<int> sSel;
    for (QModelIndexList::iterator it = lSelFiles.begin(), end = lSelFiles.end(); it != end; ++it)
    {
        sSel.insert(it->row());
    }

    m_vpSelHandlers.clear();
    for (set<int>::iterator it = sSel.begin(), end = sSel.end(); it != end; ++it)
    {
        m_vpSelHandlers.push_back(m_vpViewHandlers[*it]);
    }

    sort(m_vpSelHandlers.begin(), m_vpSelHandlers.end(), CmpMp3HandlerPtrByName());
}


vector<string> CommonData::getSelNames() // calls updateSelList() and returns the names ot the selected files;
{
    updateSelList();
    vector<string> v;
    for (int i = 0, n = cSize(m_vpSelHandlers); i < n; ++i)
    {
        v.push_back(m_vpSelHandlers[i]->getName());
    }
    return v;
}


const deque<const Mp3Handler*>& CommonData::getSelHandlers()
{
    updateSelList();
    return m_vpSelHandlers;
}



// the index in m_vpAllTransf for a transformation with a given name; throws if the name doesn't exist;
int CommonData::getTransfPos(const char* szTransfName) const
{
    for (int i = 0, n = cSize(m_vpAllTransf); i < n; ++i)
    {
        //if (m_vpAllTransf[i]->getName() == szTransfName) // this works as of 2008.11.07, but it doesn't provide any performance advantage, so better without it
        if (0 == strcmp(m_vpAllTransf[i]->getActionName(), szTransfName))
        {
            return i;
        }
    }
    CB_ASSERT (false);
}


set<string> CommonData::getAllDirs() const
{
    set<string> sDirs;
    for (int i = 0, n = cSize(m_vpAllHandlers); i < n; ++i)
    {
        const Mp3Handler* p (m_vpAllHandlers[i]);
        const string& s (p->getName());
        string::size_type n (s.rfind(getPathSep()));
        CB_ASSERT(string::npos != n);
        string strDir (s.substr(0, n));
        sDirs.insert(strDir);
    }

    return sDirs;
}


// keeps existing handlers as long as they are still "included"
void CommonData::setDirectories(const std::vector<std::string>& vstrIncludeDirs, const std::vector<std::string>& vstrExcludeDirs)
{
    //const vector<string>& vstrSel (getSelNames());
    //string strCrtName (getCrtName());

    m_vstrIncludeDirs = vstrIncludeDirs;
    m_vstrExcludeDirs = vstrExcludeDirs;

    m_dirTreeEnum.reset(vstrIncludeDirs, vstrExcludeDirs);
    m_settings.saveDirs(vstrIncludeDirs, vstrExcludeDirs);

    vector<const Mp3Handler*> vpDel;
    for (int i = 0, n = cSize(m_vpAllHandlers); i < n; ++i)
    {
        const Mp3Handler* p (m_vpAllHandlers[i]);
        if (!m_dirTreeEnum.isIncluded(p->getName()))
        {
            vpDel.push_back(p);
        }
    }

    deque<const Mp3Handler*> vpAll, vpFlt, vpView;

    set_difference(m_vpAllHandlers.begin(), m_vpAllHandlers.end(), vpDel.begin(), vpDel.end(), back_inserter(vpAll), CmpMp3HandlerPtrByName());
    set_difference(m_vpFltHandlers.begin(), m_vpFltHandlers.end(), vpDel.begin(), vpDel.end(), back_inserter(vpFlt), CmpMp3HandlerPtrByName());
    set_difference(m_vpViewHandlers.begin(), m_vpViewHandlers.end(), vpDel.begin(), vpDel.end(), back_inserter(vpView), CmpMp3HandlerPtrByName());

    vpAll.swap(m_vpAllHandlers);
    vpFlt.swap(m_vpFltHandlers);
    vpView.swap(m_vpViewHandlers);

    //updateWidgets(strCrtName, vstrSel); // !!! no point in keeping crt/sel; this is only called by scan(), and after that the first file gets selected anyway
    updateWidgets();
}



/*QString CommonData::getNoteLabel(int nPosInFlt) // gets the label of a note based on its position in m_uniqueNotes.m_vpFlt
{
    return ::getNoteLabel(m_uniqueNotes.getFltVec()[nPosInFlt]);
}*/


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


// For the vertical header of a QTableView whose labels are the current row number, it determines the width necessary to accomodate any of those labels.
// Currently it uses a hard-coded value to add to the width. //ttt2 fix
// It makes 2 assumptions:
//   - the TableView uses the same FontMetrics as the ones returned by QApplication::fontMetrics() (this is intended to be called from a TableModel's headerData(), for which finding the table is not easy; and anyway, a model can be connected to several tables)
//   - digits in the font have the same size, or at least there is no digit with a size larger than that of '9'; (in many fonts all the digits do have the same size, so it should be OK most of the time)
// Only the width is calculated; the height is returned as "1". This allows the content of a cell to determine the width of a row. Returning the height actually needed to draw the label would cause the rows to be to large, because significant spacing is added to the result. This is the opposite of what happens to the width, where a big number of pixels must be added by getNumVertHdrSize() just to have everything displayed. // ttt3 is this a Qt bug? (adds spacing where it's not needed and doesn't add it where it should)
//
// If nRowCount<=0 it behaves as if nRowCount==1
//
// Returns QVariant() for horizontal headers.
//
// The real reason this is needed: Qt can easily resize the header to accomodate all the header labels, and that's enough for fixed-height rows, whose height is set with verticalHeader()->setDefaultSectionSize(). However, if resizeRowsToContents() gets called (and it seems that it must be called to get variable-height working, and some flag to enable this doesn't seem to exist) the height of each row's header becomes too big. Using getNumVertHdrSize() we force the height to be 1, thus allowing the data cells to tell the height (the final height is the maximum between data cells and the header cell for each row). //ttt2 At some point it seemed that rows would get larger even without calling getNumVertHdrSize(). This should be looked at again.
QVariant getNumVertHdrSize(int nRowCount, Qt::Orientation eOrientation) // ttt2 add optional param QTableView to take the metrics from
{
    if (eOrientation == Qt::Vertical)
    {
        QFontMetrics fm (QApplication::fontMetrics());
        if (nRowCount <= 0)
        {
            nRowCount = 1;
        }
        double d (1.01 + log10(double(nRowCount)));
        QString s (int(d), QChar('9'));
        int nWidth (fm.width(s));
        return QSize(nWidth + 10, 1); //ttt2 hard-coded "10"
    }
    return QVariant();
}



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

#if 0
static QString getNoteLabel(int nPos)
{
    if (-1 == nPos) { return ""; }
    //ttt2 perhaps have a vector where each string is added manually; then it's possible to skip "trouble" letters, like 'l' and 'I' (look the same in many fonts) or 'm' (too wide)
    //ttt2 perhaps use underline, strikeout, ... to have more single-letter labels
    QString s;
    { // currently accepts codes from 0 to 103
#if 1
        if (nPos >= 52)
        {
            s = "'";
            nPos -= 52;
        }

        if (nPos < 26)
        {
            //nPos = 0x03B1 + nPos; //  greek alpha
            nPos += 'a';
        }
        else if (nPos < 52)
        {
            nPos = 'A' + nPos - 26;
        }
        else
        {
            CB_ASSERT (false);
        } //ttt2 add more when needed

#else
        if (nPos < 26)
        {
            //nPos = 0x03B1 + nPos; //  greek alpha
            nPos += 'a';
        }
        else if (nPos < 52)
        {
            nPos = 'A' + nPos - 26;
        }
        else if (nPos < 77)
        {
            nPos = 0x03B1 + nPos - 52; //  greek alpha
        } //ttt2 add more when needed
        else
        {
            CB_ASSERT (false);
        }

        //return QString::number(nPos);
#endif
        /*int n1 (nPos % 52);
        int n2 (nPos / 52);
        return getNoteLabel(n2) + getNoteLabel(n1);*/
    }

    nPos %= 26;
    {
        //nPos = 0x03B1 + nPos; //  greek alpha
        nPos += 'a';
    }
    if ('m' == nPos || 'w' == nPos) { nPos = 'i'; }

    QChar c (nPos);
    s = c + s;
    //s += "\n";
    s = c + s;
    return s;
}
#endif

static QString getNoteLabel(int nCateg, int nPos)
{
    if (nCateg < 0 || nCateg >= Note::CUSTOM || nPos < 0 || nPos > 'z' - 'a' - 2) { return ""; }
    if (nPos >= 'm' - 'a') { ++nPos; }
    if (nPos >= 'w' - 'a') { ++nPos; }
    if (nCateg >= 'm' - 'a') { ++nCateg; }
    if (nCateg >= 'w' - 'a') { ++nCateg; }
    char cp ('a' + nPos);
    char cc ('a' + nCateg);
    QString s;
    s += cc;
    s += cp;
    return s;
}

// returns a label for a note in a given position; first 26 notes get labels "a" .. " z", next 26 get "A" .. "Z", then "aa" .. "az", "aA" .. "aZ", "ba", ...
QString getNoteLabel(const Note* pNote)
{
/*if (0 == nPos) return "l";
if (1 == nPos) return "m";
if (2 == nPos) return "wwwwww";//*/
    //CB_ASSERT (nPos >= 0);
    return getNoteLabel(pNote->getCategory(), pNote->getLabelIndex());
}

#if 0
struct TestLabel
{
    TestLabel()
    {
        for (int i = 0; i < 104; ++i)
        {
            qDebug("%d %s", i, getNoteLabel(i).toUtf8().data());
        }
    }
};

static TestLabel ppppppppp;

#endif


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

const QColor& ERROR_PEN_COLOR()
{
    static QColor col (112, 48, 0);
    return col;
}

const QColor& SUPPORT_PEN_COLOR()
{
    static QColor col (24, 64, 190);
    return col;
}




//ttt2 inconsistency: note is ref while vpNoteSet has vectors; OTOH comparison is not done by address; perhaps just document this;
// color is normally the category color, but for support notes it's a "support" color; if the note isn't found in vpNoteSet, dGradStart and dGradEnd are set to -1, but normally they get a segment obtained by dividing [0, 1] in equal parts;
void CommonData::getNoteColor(const Note& note, const vector<const Note*>& vpNoteSet, QColor& color, double& dGradStart, double& dGradEnd) const
{
    LAST_STEP("CommonData::getNoteColor()");
    dGradStart = -1;
    dGradEnd = -1;

    if (Note::TRACE == note.getSeverity()) { color = QColor(255, 255, 255); return; } //ttt2 use a system color

    /*if (Note::SUPPORT == note.getSeverity())
    {
        color = QColor(235, 235, 255);
    }
    else*/
    {
        color = m_vNoteCategColors[note.getCategory()];
    }

    vector<const Note*>::const_iterator it (lower_bound(vpNoteSet.begin(), vpNoteSet.end(), &note, CmpNotePtrById()));

    if (vpNoteSet.end() == it)
    {
        CB_ASSERT (vpNoteSet.empty());
        return;
    }

    int nPos (it - vpNoteSet.begin());
    int nFirstInGroup (nPos), nLastInGroup (nPos);

    for (; nFirstInGroup > 0 && vpNoteSet[nFirstInGroup - 1]->getCategory() == note.getCategory(); --nFirstInGroup) {}
    for (; nLastInGroup < cSize(vpNoteSet) - 1 && vpNoteSet[nLastInGroup + 1]->getCategory() == note.getCategory(); ++nLastInGroup) {}

    nLastInGroup -= nFirstInGroup;
    nPos -= nFirstInGroup;
    //nFirstInGroup = 0;

    dGradStart = double(nPos)/(nLastInGroup + 1);
    dGradEnd = double(nPos + 1)/(nLastInGroup + 1);
}


QColor getDefaultBkgCol()
{
    QColor c (QPalette().color(QPalette::Active, QPalette::Light));
    if (isWhite(c))
    {
        c = QPalette().color(QPalette::Active, QPalette::Window).lighter(110);
    }
    if (isWhite(c))
    {
        c = QPalette().color(QPalette::Active, QPalette::Window).lighter(103);
    }
    if (isWhite(c))
    {
        c = QPalette().color(QPalette::Active, QPalette::Window);
    }
    if (isWhite(c))
    {
        c = QColor(253, 250, 240);
    }

    return c;
}




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


/*void printFontInfo(const char* szLabel, const QFont& font)
{
    QFontInfo info (font);
    cout << szLabel << ": " << info.family().toStdString() << ", exactMatch:" << info.exactMatch() << ", fixedPitch:" << info.fixedPitch() << ", italic:" << info.italic() <<
        ", pixelSize:" << info.pixelSize() << ", pointSize:" << info.pointSize() << ", real pointSize on 100dpi screen:" << info.pixelSize()*72.0/100 << endl;
}
*/


#if 0
const QFont& getFixedFont()
{
    static QFont s_font; // ttt3 not multithreaded, but doesn't matter
    static bool s_bInit (false);
    if (!s_bInit)
    {
        s_bInit = true;

        //myOption.font = QFont("Courier", myOption.font.pointSize());
        //myOption.font = QFont("B&H LucidaTypewritter", myOption.font.pointSize());
        //myOption.font = QFont("LucidaTypewritter", myOption.font.pointSize());
        //myOption.font = QFont("B&H LucidaTypewritter", 9);
        //myOption.font = QFont("Lucidatypewriter");
        //myOption.font = QFont("LucidaTypewriter");
        //myOption.font = QFont();
        //myOption.font.setPixelSize(12);
        //myOption.font.setRawName("B&H LucidaTypewritter");
        //myOption.font.setRawName("LucidaTypewritter");
        //myOption.font.setRawName("Typewritter");
        //myOption.font.setRawName("-b&h-lucidatypewritter-*-*-*-*-*-*-*-*-*-*-*-*");
        //myOption.font.setRawName("-b&h-lucidatypewriter-medium-r-normal-sans-26-190-100-100-m-159-iso8859-1");
        //myOption.font.setRawName("-b&h-lucidatypewriter-medium-r-normal-sans-12-120-75-75-m-70-iso8859-1");
        //myOption.font.setStyleHint(QFont::TypeWriter);
        //myOption.font.fromString("B&H LucidaTypewriter,11,-1,5,50,0,0,0,0,0");
//        myOption.font.setSize...
//        myOption.font.setFixedPitch(true); // ttt2 is there a difference between setFixedPitch(true) and setStyleHint(QFont::TypeWriter) ?

        /*myOption.font.setFamily("B&H LucidaTypewriter");
        myOption.font.setPixelSize(12);*/

        //printFontInfo("orig font: ", s_font);
        //s_font = QFont("B&H LucidaTypewriter");
        s_font.setFamily("B&H LucidaTypewriter");
        //s_font.setFamily("B&H LucidaTypewriter12c");
        s_font.setFixedPitch(true);  // !!! needed to select some fixed font in case there's no "B&H LucidaTypewriter" installed
        s_font.setPixelSize(12); //ttt2 hard-coded "12"; //ttt1 some back-up font instead of just letting the system decide
        //s_font.setPointSize(9); //ttt2 hard-coded "9"; //ttt2 some back-up font instead of just letting the system decide

//QFont f; qDebug("default font: pix %d, point: %d, fam: %s", f.pixelSize(), f.pointSize(), f.family().toLatin1().data());

        //myOption.font.setPointSize(12);
//        printFontInfo("new font: ", s_font);

//font: B&H LucidaTypewriter,11,-1,5,50,0,0,0,0,0

//print map
// xfontsel
        // "mmmiiiWWWlll";
    }

    return s_font;
}
#endif

#ifdef OKPOJOIJWOUIh
void tstFont()
{
/*    bool b;
    QFont f (QFontDialog::getFont(&b, this));
    //cout << "font: " << QFontInfo(myOption.font).family().toStdString() << endl;
    //cout << "font from dlg: " << f.toString().toStdString() << endl;
    printFontInfo("font from dlg", f);
*/

    for (int i = 6; i < 18; ++i)
    {
        QFont f ("B&H LucidaTypewriter", i);

        char a [20];
        sprintf(a, "point size %d", i);
        printFontInfo(a, f);
    }

    cout << endl;

    for (int i = 6; i < 18; ++i)
    {
        QFont f ("B&H LucidaTypewriter");
        f.setPixelSize(i);

        char a [20];
        sprintf(a, "pixel size %d", i);
        printFontInfo(a, f);
    }

    cout << endl;

    {
        bool b;
        QFont f (QFontDialog::getFont(&b));
        printFontInfo("font from QFontDialog" , f);
        /*
        QFont f;
        if (QDialog::Accepted == KFontDialog::getFont(f, true))
        {
            printFontInfo("font from QFontDialog" , f);
        }*/
    }

    cout << endl;

    for (int i = 6; i < 18; ++i)
    {
        QFont f ("B&H LucidaTypewriter12c", i);

        char a [50];
        sprintf(a, "B&H LucidaTypewriter12c point size %d", i);
        printFontInfo(a, f);
    }

}
#endif

/*extern*/ const int CUSTOM_TRANSF_CNT (4);


void Filter::setNotes(const std::vector<const Note*>& v)
{
    m_vpNotes.clear();
    for (int i = 0; i < cSize(v); ++i)
    {
        const Note* p (Notes::getMaster(v[i])); // 2009.03.23 currently v comes with pointers from Notes, so this is pointless;
        CB_ASSERT (0 != p);
        m_vpNotes.push_back(p);
    }
    //qDebug("set v sz %d", cSize(m_vpNotes));

    m_bNoteFilter = !m_vpNotes.empty();

    emit filterChanged();
}


void Filter::setDirs(const std::vector<string>& v)
{
    //if (v == m_vDirs) { return; }
    m_vDirs = v;
    m_bDirFilter = !m_vDirs.empty();

    emit filterChanged();
}


// !!! needed because we need m_vDirs next time we press the filter button, so we don't want to delete it with a "setDirs(vector<string>())", but just ignore it
void Filter::disableDir()
{
    if (!m_bDirFilter) { return; }

    m_bDirFilter = false;

    emit filterChanged();
}


void Filter::disableNote()
{
    if (!m_bNoteFilter) { return; }

    m_bNoteFilter = false;

    emit filterChanged();
}


void Filter::disableAll() // saves m_bNoteFilter to m_bSavedNoteFilter and m_bDirFilter to m_bSavedDirFilter, then disables the filters
{
    m_bSavedNoteFilter = m_bNoteFilter;
    m_bSavedDirFilter = m_bDirFilter;
    if (m_bNoteFilter || m_bDirFilter)
    {
        m_bNoteFilter = m_bDirFilter = false;
        emit filterChanged();
    }
}


void Filter::restoreAll() // loads m_bNoteFilter from m_bSavedNoteFilter and m_bDirFilter from m_bSavedDirFilter, then enables the filters, if they are true
{
    if (m_bNoteFilter != m_bSavedNoteFilter || m_bDirFilter != m_bSavedDirFilter)
    {
        m_bNoteFilter = m_bSavedNoteFilter;
        m_bDirFilter = m_bSavedDirFilter;
        emit filterChanged();
    }
}


//========================================================================================================================
//========================================================================================================================
//========================================================================================================================



// called after config change or filter change or mergeHandlerChanges(), mainly to update unique notes (which is reflected in columns in the file grid and in lines in the unique notes list); also makes sure that something is displayed if there are any files in m_vpFltHandlers (e.g. if a transform was applied that removed all the files in an album, the next album gets loaded);
// this is needed after transforms/normalization/tag editing, but there's no need for an explicit call, because all these call mergeHandlerChanges() (directly or through MainFormDlgImpl::scan())
void CommonData::updateWidgets(const std::string& strCrtName /*= ""*/, const std::vector<std::string>& vstrSel /*= std::vector<std::string>()*/)
{
    if (m_vpViewHandlers.empty() && !m_vpFltHandlers.empty())
    {
        CommonData::ViewMode eViewMode (getViewMode());
        setViewMode(CommonData::ALL);
        int nPos (getPosInView(strCrtName)); // doesn't matter if it's empty
        const Mp3Handler* pMp3Handler (m_vpViewHandlers[nPos]);
        setViewMode(eViewMode, pMp3Handler);
    }

    int nRow (-1);
    if (!strCrtName.empty()) { nRow = getPosInView(strCrtName); }
    if (-1 == nRow) { nRow = 0; }

    vector<int> v;
    for (int i = 0, n = cSize(vstrSel); i < n; ++i)
    {
        int k (getPosInView(vstrSel[i], EXACT_MATCH));
        if (-1 != k)
        {
            v.push_back(k);
        }
    }

    updateUniqueNotes();

    m_pFilesModel->selectRow(nRow, v);
    m_pUniqueNotesModel->selectTopLeft();

    resizeFilesGCols();
    m_pFilesG->setFocus();
}



// updates m_uniqueNotes to reflect the current m_vpAllHandlers and m_vpFltHandlers;
void CommonData::updateUniqueNotes()
{
    set<const Note*, CmpNotePtrById> spAllNotes;
    getUniqueNotes(m_vpAllHandlers, spAllNotes);
    set<const Note*, CmpNotePtrById> spFltNotes;
    getUniqueNotes(m_vpFltHandlers, spFltNotes); // ttt2 not sure if it makes more sense for m_uniqueNotes to show notes for what is filtered or for what is "current"; it feels like the list would change too often if it is going to reflect "current"; anyway, if the change is made, the code must be moved;
    m_uniqueNotes.clear();
    // if there's no filtering (and sometimes even if there is a filter applied), spAllNotes and spFltNotes have the same elements; it's probably not worth the trouble to try and figure out if they should be equal or not
    m_uniqueNotes.addColl(spAllNotes);
    m_uniqueNotes.setFlt(spFltNotes);
}



// updates m_vpFltHandlers and m_vpViewHandlers; also updates the state of the filter buttons (deselecting them if the user chose empty filters)
void CommonData::onFilterChanged()
{
    const vector<string>& vstrSel (getSelNames());

    string strCrtName (getCrtName());
    m_vpFltHandlers.clear();

    const vector<string>& vSelDirs (m_filter.getDirs());
    const vector<const Note*>& vpSelNotes (m_filter.getNotes());
    int nSelNotesSize (cSize(vpSelNotes));
    CmpNotePtrById cmp; // !!! same comparator used by NoteColl::sort()


    bool bDirFilter (m_filter.isDirEnabled());
    bool bNoteFilter (m_filter.isNoteEnabled());

    for (int i = 0, n = cSize(m_vpAllHandlers); i < n; ++i)
    {
        const Mp3Handler* pHandler (m_vpAllHandlers[i]);

        if (bDirFilter)
        {
            vector<string>::const_iterator it (lower_bound(vSelDirs.begin(), vSelDirs.end(), pHandler->getName()));
            if (vSelDirs.begin() == it) { goto e1; }
            --it;
            //cout << *it << " " << pHandler->getName() << endl;
            if (!isInsideDir(pHandler->getName(), *it)) { goto e1; }
        }

        if (bNoteFilter)
        {
            const NoteColl& coll (pHandler->getNotes());
            const vector<Note*>& vpCrtNotes (coll.getList());
            int i (0);
            int j (0);
            int nCrtNotesSize (cSize(vpCrtNotes));

            for (;;)
            {
                for (; i < nSelNotesSize && j < nCrtNotesSize && cmp(vpSelNotes[i], vpCrtNotes[j]); ++i) {}
                for (; i < nSelNotesSize && j < nCrtNotesSize && cmp(vpCrtNotes[j], vpSelNotes[i]); ++j) {}
                if (i >= nSelNotesSize || j >= nCrtNotesSize) { goto e1; }
                if (CmpNotePtrById::equals(vpSelNotes[i], vpCrtNotes[j])) { break; }
            }
        }

        m_vpFltHandlers.push_back(pHandler);

e1:;
    }

    m_pNoteFilterB->setChecked(bNoteFilter); // !!! no guard needed, because the event that calls the filter is "clicked", not "checked"
    m_pDirFilterB->setChecked(bDirFilter);

    m_vpViewHandlers.clear();
    updateWidgets(strCrtName, vstrSel);

    if (m_vpViewHandlers.empty() && (bNoteFilter || bDirFilter))
    {
        m_filter.disableAll(); // it makse some sense to not disable the filter, e.g. after reload
    }
}



void CommonData::setCustomTransf(const std::vector<std::vector<int> >& vv)
{
    CB_ASSERT (cSize(vv) == CUSTOM_TRANSF_CNT);
    //ttt2 assert elements are withing range
    m_vvnCustomTransf = vv;
}


void CommonData::setCustomTransf(int nTransf, const std::vector<int>& v)
{
    CB_ASSERT (nTransf >= 0 && nTransf < CUSTOM_TRANSF_CNT);
    //ttt2 assert elements are withing range
    m_vvnCustomTransf[nTransf] = v;
}


void CommonData::setVisibleTransf(const std::vector<int>& v)
{
    //ttt2 assert elements are withing range
    m_vnVisibleTransf = v;
}



void CommonData::setIgnoredNotes(const std::vector<int>& v)
{
    const int NOTE_CNT (cSize(Notes::getAllNotes()));
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        CB_ASSERT (v[i] >= 0 && v[i] < NOTE_CNT);
    }
    set<int> s (v.begin(), v.end());
    CB_ASSERT (cSize(s) == cSize(v));
    m_vnIgnoredNotes.clear();
    m_vnIgnoredNotes.insert(m_vnIgnoredNotes.end(), s.begin(), s.end());
}


void CommonData::setQualThresholds(const QualThresholds& q)
{
    // ttt2 some checks
    m_qualThresholds = q;
}


// looks in m_vpAllHandlers; returns 0 if there's no such handler
const Mp3Handler* CommonData::getHandler(const std::string& strName) const
{
    deque<const Mp3Handler*>::const_iterator it (lower_bound(m_vpViewHandlers.begin(), m_vpViewHandlers.end(), strName, CmpMp3HandlerPtrByName()));
    if (m_vpViewHandlers.end() == it || (*it)->getName() != strName)
    {
        return 0;
    }

    return *it;
}



// elements from vpAdd that are not "included" are discarded; when a new version of a handler is in vpAdd, the old version may be in vpDel, but this is optional; takes ownership of elements from vpAdd; deletes the pointers from vpDel;
void CommonData::mergeHandlerChanges(const vector<const Mp3Handler*>& vpAdd1, const vector<const Mp3Handler*>& vpDel1, int nKeepWhenUpdate)
{
    //m_bDirty = m_bDirty || !vpAdd1.empty() || !vpDel1.empty();

    const string& strCrtName (nKeepWhenUpdate & CURRENT ? getCrtName() : "");
    const vector<string>& vstrSel (nKeepWhenUpdate & SEL ? getSelNames() : vector<string>());

    vector<const Mp3Handler*> vpAdd;
    for (int i = 0, n = cSize(vpAdd1); i < n; ++i)
    {
        if (m_dirTreeEnum.isIncluded(vpAdd1[i]->getName()))
        {
            vpAdd.push_back(vpAdd1[i]);
        }
        else
        {
            delete vpAdd1[i];
        }
    }

    sort(vpAdd.begin(), vpAdd.end(), CmpMp3HandlerPtrByName()); // !!! the vector is probably sorted, but file renaming might have an impact on the order (and if it doesn't have now, it might in the future)

    vector<const Mp3Handler*> vpDel;

    { // we want in vpDel elements from vpDel1, as well as elements from m_vpAllHandlers for which there is a newer version in vpAdd
        vector<const Mp3Handler*> vpDel2 (vpDel1);
        sort(vpDel2.begin(), vpDel2.end(), CmpMp3HandlerPtrByName());

        vector<const Mp3Handler*> vpNewVer; // existing handlers for which there is a new version

        set_intersection(m_vpAllHandlers.begin(), m_vpAllHandlers.end(), vpAdd.begin(), vpAdd.end(), back_inserter(vpNewVer), CmpMp3HandlerPtrByName()); // !!! according to the standard (25.3.5.3) copying is made from the first range (i.e. from m_vpAllHandlers)

        set_union(vpNewVer.begin(), vpNewVer.end(), vpDel2.begin(), vpDel2.end(), back_inserter(vpDel), CmpMp3HandlerPtrByName());
    }

    deque<const Mp3Handler*> vpAll, vpFlt, vpView;
/*qDebug("1a %d %d %d", cSize(m_vpAllHandlers), cSize(m_vpFltHandlers), cSize(m_vpViewHandlers));
qDebug("1b %d %d", cSize(vpDel), cSize(vpAdd));
qDebug("1c %d %d %d", cSize(vpAll), cSize(vpFlt), cSize(vpView));*/

    set_difference(m_vpAllHandlers.begin(), m_vpAllHandlers.end(), vpDel.begin(), vpDel.end(), back_inserter(vpAll), CmpMp3HandlerPtrByName());
    set_difference(m_vpFltHandlers.begin(), m_vpFltHandlers.end(), vpDel.begin(), vpDel.end(), back_inserter(vpFlt), CmpMp3HandlerPtrByName());
    set_difference(m_vpViewHandlers.begin(), m_vpViewHandlers.end(), vpDel.begin(), vpDel.end(), back_inserter(vpView), CmpMp3HandlerPtrByName());

    clearPtrContainer(vpDel);

    m_vpAllHandlers.clear(); m_vpFltHandlers.clear(); m_vpViewHandlers.clear();
    set_union(vpAll.begin(), vpAll.end(), vpAdd.begin(), vpAdd.end(), back_inserter(m_vpAllHandlers), CmpMp3HandlerPtrByName());
    set_union(vpFlt.begin(), vpFlt.end(), vpAdd.begin(), vpAdd.end(), back_inserter(m_vpFltHandlers), CmpMp3HandlerPtrByName()); // !!! perhaps for view and flt we should check the directory/filter, but it seems more important that the new handlers are seen, so they can be compared to the old ones //ttt1 review
    set_union(vpView.begin(), vpView.end(), vpAdd.begin(), vpAdd.end(), back_inserter(m_vpViewHandlers), CmpMp3HandlerPtrByName());

    updateWidgets(strCrtName, vstrSel);
}


void CommonData::setSongInCrtAlbum()
{
    m_nSongInCrtAlbum = 0;
    if (m_vpAllHandlers.empty()) { return; }

    const Mp3Handler* p (getCrtMp3Handler());
    CB_ASSERT (0 != p);

    deque<const Mp3Handler*>::iterator it (lower_bound(m_vpAllHandlers.begin(), m_vpAllHandlers.end(), p, CmpMp3HandlerPtrByName()));
    CB_ASSERT (m_vpAllHandlers.end() != it && (*it) == p);
    m_nSongInCrtAlbum = it - m_vpAllHandlers.begin();
}


deque<const Mp3Handler*> CommonData::getCrtAlbum() const //ttt2 perhaps sort by track, if there's an ID3V2; anyway, this is only used by TagWriter and FileRenamer, and TagWriter does its own sorting (there might be an issue if calling getCrtAlbum() multiple times for the same album returns songs in different order because tracks changed between the calls)
{
    deque<const Mp3Handler*> v;
    if (m_vpAllHandlers.empty()) { return v; }

    //CB_ASSERT (m_nSongInCrtAlbum >= 0 && m_nSongInCrtAlbum < cSize(m_vpAllHandlers)); // !!! incorrect assert; files migth have been deleted without updating m_nSongInCrtAlbum (well, one of the purposes of this function is to update m_nSongInCrtAlbum)
    CB_ASSERT (m_nSongInCrtAlbum >= 0);
    m_nSongInCrtAlbum = min(m_nSongInCrtAlbum, cSize(m_vpAllHandlers) - 1);
    string s (m_vpAllHandlers[m_nSongInCrtAlbum]->getDir());
    int nFirst (m_nSongInCrtAlbum), nLast (m_nSongInCrtAlbum);
    for (; nFirst >= 0 && m_vpAllHandlers[nFirst]->getDir() == s; --nFirst) {}
    ++nFirst;

    for (int n = cSize(m_vpAllHandlers); nLast < n && m_vpAllHandlers[nLast]->getDir() == s; ++nLast) {}
    --nLast;

    m_nSongInCrtAlbum = nFirst;
    v.insert(v.end(), m_vpAllHandlers.begin() + nFirst, m_vpAllHandlers.begin() + nLast + 1);
    return v;
}


// used for album navigation in tag editor and file renamer
bool CommonData::nextAlbum() const
{
    if (m_vpAllHandlers.empty()) { return false; }

    CB_ASSERT (m_nSongInCrtAlbum >= 0 && m_nSongInCrtAlbum < cSize(m_vpAllHandlers));
    string s (m_vpAllHandlers[m_nSongInCrtAlbum]->getDir());

    deque<const Mp3Handler*>::const_iterator it (lower_bound(m_vpFltHandlers.begin(), m_vpFltHandlers.end(), m_vpAllHandlers[m_nSongInCrtAlbum], CmpMp3HandlerPtrByName()));
    //CB_ASSERT (m_vpAllHandlers[m_nSongInCrtAlbum] == *it); // !!! wrong - getCrtAlbum() moves m_nSongInCrtAlbum to the first song in album regardless of filter
    CB_ASSERT (m_vpFltHandlers.end() != it);
    int n (cSize(m_vpFltHandlers));
    int nFltPos (it - m_vpFltHandlers.begin());
    for (; nFltPos < n && m_vpFltHandlers[nFltPos]->getDir() == s; ++nFltPos) {}
    if (n == nFltPos)
    {
        return false;
    }

    it = lower_bound(m_vpAllHandlers.begin(), m_vpAllHandlers.end(), m_vpFltHandlers[nFltPos], CmpMp3HandlerPtrByName());
    CB_ASSERT (m_vpFltHandlers[nFltPos] == *it);
    m_nSongInCrtAlbum = it - m_vpAllHandlers.begin();

    return true;
}





bool CommonData::prevAlbum() const
{
    if (m_vpAllHandlers.empty()) { return false; }

    CB_ASSERT (m_nSongInCrtAlbum >= 0 && m_nSongInCrtAlbum < cSize(m_vpAllHandlers));
    string s (m_vpAllHandlers[m_nSongInCrtAlbum]->getDir());

    deque<const Mp3Handler*>::const_iterator it (lower_bound(m_vpFltHandlers.begin(), m_vpFltHandlers.end(), m_vpAllHandlers[m_nSongInCrtAlbum], CmpMp3HandlerPtrByName()));
    //CB_ASSERT (m_vpAllHandlers[m_nSongInCrtAlbum] == *it); // !!! wrong - getCrtAlbum() moves m_nSongInCrtAlbum to the first song in album regardless of filter
    CB_ASSERT (m_vpFltHandlers.end() != it);
    int nFltPos (it - m_vpFltHandlers.begin());
    for (; nFltPos >= 0 && m_vpFltHandlers[nFltPos]->getDir() == s; --nFltPos) {}
    if (nFltPos < 0)
    {
        return false;
    }

    it = lower_bound(m_vpAllHandlers.begin(), m_vpAllHandlers.end(), m_vpFltHandlers[nFltPos], CmpMp3HandlerPtrByName());
    CB_ASSERT (m_vpFltHandlers[nFltPos] == *it);
    m_nSongInCrtAlbum = it - m_vpAllHandlers.begin();

    return true;
}




// resizes the first column to use all the available space; doesn't shrink it to less than 400px, though
void CommonData::resizeFilesGCols()
{
    QHeaderView* p (m_pFilesG->horizontalHeader());

    p->setResizeMode(0, QHeaderView::Stretch);
    int n (p->sectionSize(0));
    if (n < MIN_FILE_WIDTH)
    {
        //n = 600;
        n = MIN_FILE_WIDTH;
        //n = m_pFilesG->width()
    }

    p->setResizeMode(0, QHeaderView::Interactive);
    p->resizeSection(0, n);

    /*if (nSection >= 1 + 26*2) // ttt3 this is needed to handle more than 52 notes // ttt3 keep in mind that bold fonts need more space
    {
        for ...
            QFontMetrics fm (m_pCommonData->m_pFilesG->fontMetrics());
            int nWidth (fm.width(getNoteLabel(nSection - 1)));
            //return QSize(50, CELL_HEIGHT);
            resizeSection(i, nWidth);
    }*/
}


void CommonData::trace(const std::string& s)
{
    if (m_bTraceEnabled)
    {
        m_vLogs.push_back(s);
    }
}

void CommonData::clearLog()
{
    m_vLogs.clear();
}


void setupTraceToFile(bool bEnable);

void CommonData::setTraceToFile(bool bTraceToFile) // also removes the file
{
    m_bTraceToFile = bTraceToFile;
    setupTraceToFile(bTraceToFile);
}


void CommonData::setFastSave(bool bFastSave, bool bUpdateTransforms)
{
    if (m_bFastSave == bFastSave) { return; }
    m_bFastSave = bFastSave;
    if (!bUpdateTransforms) { return; }
}


/*

KDE 3.5.7 / Qt 3.3.8

point size 6: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:8, pointSize:6, real pointSize on 100dpi screen:5.76
point size 7: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:10, pointSize:7, real pointSize on 100dpi screen:7.2
point size 8: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:11, pointSize:8, real pointSize on 100dpi screen:7.92
point size 9: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
point size 10: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
point size 11: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
point size 12: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24
point size 13: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:18, pointSize:13, real pointSize on 100dpi screen:12.96
point size 14: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:20, pointSize:14, real pointSize on 100dpi screen:14.4
point size 15: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:20, pointSize:14, real pointSize on 100dpi screen:14.4
point size 16: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:24, pointSize:17, real pointSize on 100dpi screen:17.28
point size 17: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:24, pointSize:17, real pointSize on 100dpi screen:17.28

pixel size 6: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:6, real pointSize on 100dpi screen:5.76
pixel size 7: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:6, real pointSize on 100dpi screen:5.76
pixel size 8: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:8, pointSize:6, real pointSize on 100dpi screen:5.76
pixel size 9: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:6, real pointSize on 100dpi screen:5.76
pixel size 10: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:10, pointSize:7, real pointSize on 100dpi screen:7.2
pixel size 11: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:11, pointSize:8, real pointSize on 100dpi screen:7.92
pixel size 12: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:12, pointSize:9, real pointSize on 100dpi screen:8.64
pixel size 13: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
pixel size 14: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
pixel size 15: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
pixel size 16: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24
pixel size 17: B&H LucidaTypewriter [B&H], exactMatch:1, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24

font from QFontDialog: B&H LucidaTypewriter [B&H], exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08

issues:
    1) when building by point size, the 14px (10.08 points) is selected over the 12px (8.64 points) to approximate the "9 points" font
    2) the 12px font can be selected in QFontDialog as "12 points" (as shown in the preview) but when the call returns the 14px font is passed

*/


/*

Qt 4.3.1

point size 6: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
point size 7: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:10, pointSize:10, real pointSize on 100dpi screen:7.2
point size 8: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:11, pointSize:8, real pointSize on 100dpi screen:7.92
point size 9: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:12, pointSize:12, real pointSize on 100dpi screen:8.64
point size 10: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:14, real pointSize on 100dpi screen:10.08
point size 11: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:14, real pointSize on 100dpi screen:10.08
point size 12: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24
point size 13: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:18, pointSize:18, real pointSize on 100dpi screen:12.96
point size 14: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:20, pointSize:14, real pointSize on 100dpi screen:14.4
point size 15: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:20, pointSize:14, real pointSize on 100dpi screen:14.4
point size 16: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:24, pointSize:24, real pointSize on 100dpi screen:17.28
point size 17: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:24, pointSize:24, real pointSize on 100dpi screen:17.28

pixel size 6: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
pixel size 7: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
pixel size 8: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
pixel size 9: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
pixel size 10: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:10, pointSize:10, real pointSize on 100dpi screen:7.2
pixel size 11: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:11, pointSize:8, real pointSize on 100dpi screen:7.92
pixel size 12: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:12, pointSize:12, real pointSize on 100dpi screen:8.64
pixel size 13: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:12, pointSize:12, real pointSize on 100dpi screen:8.64
pixel size 14: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:14, pointSize:14, real pointSize on 100dpi screen:10.08
pixel size 15: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:14, real pointSize on 100dpi screen:10.08
pixel size 16: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24
pixel size 17: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24

font from QFontDialog: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:14, real pointSize on 100dpi screen:10.08

issues:
    1) while building a font with a particular point size works OK, the value returned by pointSize() is wrong for fonts with 75dpi resolution
    2) the smallest "B&H LucidaTypewriter" font that can be selected in the dialog is 14px

*/


/*

Qt 4.4.0 (KDE 4.1):

point size 6: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
point size 7: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:10, pointSize:10, real pointSize on 100dpi screen:7.2
point size 8: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:11, pointSize:8, real pointSize on 100dpi screen:7.92
point size 9: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
point size 10: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
point size 11: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
point size 12: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24
point size 13: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:18, pointSize:18, real pointSize on 100dpi screen:12.96
point size 14: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:20, pointSize:14, real pointSize on 100dpi screen:14.4
point size 15: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:20, pointSize:14, real pointSize on 100dpi screen:14.4
point size 16: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:20, pointSize:14, real pointSize on 100dpi screen:14.4
point size 17: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:24, pointSize:24, real pointSize on 100dpi screen:17.28

pixel size 6: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
pixel size 7: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
pixel size 8: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
pixel size 9: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:8, pointSize:8, real pointSize on 100dpi screen:5.76
pixel size 10: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:10, pointSize:10, real pointSize on 100dpi screen:7.2
pixel size 11: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:11, pointSize:8, real pointSize on 100dpi screen:7.92
pixel size 12: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:12, pointSize:12, real pointSize on 100dpi screen:8.64
pixel size 13: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
pixel size 14: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
pixel size 15: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08
pixel size 16: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24
pixel size 17: B&H LucidaTypewriter, exactMatch:1, fixedPitch:1, italic:0, pixelSize:17, pointSize:12, real pointSize on 100dpi screen:12.24

font from QFontDialog: B&H LucidaTypewriter, exactMatch:0, fixedPitch:1, italic:0, pixelSize:14, pointSize:10, real pointSize on 100dpi screen:10.08

issues:
    1) when building by point size, the 14px (10.08 points) is selected over the 12px (8.64 points) to approximate the "9 points" font; this used to work in 4.3.1
    2) the 12px font can't be selected at all in QFontDialog
    3) in the dialog both smaller and larger fonts can be selected, but the 12px never shows up

*/





