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
#include  <map>
#include  <set>
#include  <sstream>

#include  <QBuffer>
#include  <QPainter>

#include  "CommonTypes.h"

#include  "Helpers.h"
#include  "FullSizeImgDlg.h"

#include  "DataStream.h" // for translations
#include  "Widgets.h" // for translation

using namespace std;


ImageInfo::ImageInfo(int nImageType, Status eStatus, Compr eCompr, QByteArray compressedImg, int nWidth, int nHeight) :
        m_eCompr(eCompr), m_eStatus(eStatus), m_nWidth(nWidth), m_nHeight(nHeight), m_compressedImg(compressedImg), m_nImageType(nImageType)
{
}


// the picture is scaled down, keeping the aspect ratio, if the limits are exceeded; 0 and negative limits are ignored;
// if nMaxWidth>0 and nMaxHeight<=0, nMaxHeight has the same value as nMaxWidth;
QImage ImageInfo::getImage(int nMaxWidth /* = -1*/, int nMaxHeight /* = -1*/) const
{
    CB_ASSERT (NO_PICTURE_FOUND != m_eStatus);

    if (nMaxHeight <= 0) { nMaxHeight = nMaxWidth; }
    if (nMaxWidth <= 0) { nMaxWidth = nMaxHeight; }

    QImage pic;

    if (USES_LINK == m_eStatus || ERROR_LOADING == m_eStatus || !pic.loadFromData(m_compressedImg)) //ttt2 not sure how loadFromData() handles huge images;
    {
        if (nMaxWidth < 0) { nMaxWidth = m_nWidth; }
        if (nMaxHeight < 0) { nMaxHeight = m_nHeight; }

        if (nMaxWidth <= 0) { nMaxWidth = 200; }
        if (nMaxHeight <= 0) { nMaxHeight = 200; }

        QImage errImg (nMaxWidth, nMaxHeight, QImage::Format_ARGB32);
        QPainter pntr (&errImg);
        pntr.fillRect(0, 0, nMaxWidth, nMaxHeight, QColor(255, 128, 128));
        pntr.drawRect(0, 0, nMaxWidth - 1, nMaxHeight - 1);
        //pntr.drawText(5, nMaxHeight/2 + 10, USES_LINK == m_eStatus ? "Link" : (ERROR_LOADING == m_eStatus ?  "Error" : "Uncompr error"));
        pntr.drawText(QRectF(0, 0, nMaxWidth, nMaxHeight), Qt::AlignCenter | Qt::TextWordWrap, USES_LINK == m_eStatus ? "Link" : (ERROR_LOADING == m_eStatus ?  "Error" : "Uncompr error"));
        return errImg;
    }

    if (nMaxWidth <= 0 || (pic.width() <= nMaxWidth && pic.height() <= nMaxHeight))
    {
        return pic;
    }

    return pic.scaled(nMaxWidth, nMaxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


bool ImageInfo::operator==(const ImageInfo& other) const
{
    return m_eCompr == other.m_eCompr /*&& m_eStatus == other.m_eStatus*/ && m_nWidth == other.m_nWidth && m_compressedImg == other.m_compressedImg; // !!! related to Id3V230StreamWriter::addImage() status is ignored in both places //ttt2 review decision to ignore status
}


/*static*/ int ImageInfo::MAX_IMAGE_SIZE (100000);

ImageInfo::ImageInfo(int nImageType, Status eStatus, const QImage& pic) : m_eCompr(JPG), m_eStatus(eStatus), m_nImageType(nImageType)
{
    QImage scaledPic;
    compress(pic, scaledPic, m_compressedImg);

    m_nWidth = scaledPic.width();
    m_nHeight = scaledPic.height();
}



/*static*/ const char* ImageInfo::getImageType(int nImageType)
{
    switch (nImageType)
    {
    case 0x00: return QT_TRANSLATE_NOOP("TagReader", "other");
    case 0x01: return QT_TRANSLATE_NOOP("TagReader", "32x32 icon");
    case 0x02: return QT_TRANSLATE_NOOP("TagReader", "other file icon");
    case 0x03: return QT_TRANSLATE_NOOP("TagReader", "front cover");
    case 0x04: return QT_TRANSLATE_NOOP("TagReader", "back cover");
    case 0x05: return QT_TRANSLATE_NOOP("TagReader", "leaflet page");
    case 0x06: return QT_TRANSLATE_NOOP("TagReader", "media");
    case 0x07: return QT_TRANSLATE_NOOP("TagReader", "lead artist");
    case 0x08: return QT_TRANSLATE_NOOP("TagReader", "artist");
    case 0x09: return QT_TRANSLATE_NOOP("TagReader", "conductor");
    case 0x0a: return QT_TRANSLATE_NOOP("TagReader", "band");
    case 0x0b: return QT_TRANSLATE_NOOP("TagReader", "composer");
    case 0x0c: return QT_TRANSLATE_NOOP("TagReader", "lyricist");
    case 0x0d: return QT_TRANSLATE_NOOP("TagReader", "recording location");
    case 0x0e: return QT_TRANSLATE_NOOP("TagReader", "during recording");
    case 0x0f: return QT_TRANSLATE_NOOP("TagReader", "during performance");
    case 0x10: return QT_TRANSLATE_NOOP("TagReader", "screen capture");
    //case 0x11: return QT_TRANSLATE_NOOP("TagReader", "a bright coloured fish");
    case 0x12: return QT_TRANSLATE_NOOP("TagReader", "illustration");
    case 0x13: return QT_TRANSLATE_NOOP("TagReader", "band/artist logotype");
    case 0x14: return QT_TRANSLATE_NOOP("TagReader", "publisher/studio logotype");
    default:
        return QT_TRANSLATE_NOOP("TagReader", "unknown");
    }
}


const char* ImageInfo::getImageType() const
{
    return getImageType(m_nImageType);
}


/*static*/ const char* ImageInfo::getComprStr(Compr eCompr)
{
    switch (eCompr)
    {
    case INVALID: return "invalid"; // no translation needed as long as this is only used in XML export
    case JPG: return "JPEG";
    case PNG: return "PNG";
    }
    CB_ASSERT (false);
}


// scales down origPic and stores the pixmap in scaledPic, as well as a compressed version in comprImg; the algorithm coninues until comprImg becomes smaller than MAX_IMAGE_SIZE or until the width and the height of scaledPic get smaller than 150; no scaling is done if comprImg turns out to be small enough for the original image;
/*static*/ void ImageInfo::compress(const QImage& origPic, QImage& scaledPic, QByteArray& comprImg)
{
    const int QUAL (-1); //ttt2 hard-coded

    //QPixmap scaledImg;
    int n (max(origPic.width(), origPic.height()));
//qDebug("-------------");
    for (int i = n;;)
    {
        scaledPic = origPic.scaled(i, i, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        comprImg.clear();
        QBuffer bfr (&comprImg);
        //nWidth = scaledPic.width(); nHeight = scaledPic.height();

        scaledPic.save(&bfr, "jpg", QUAL);

        int nSize (comprImg.size());
//qDebug("width=%d, size=%d", i, nSize);
        if (nSize <= MAX_IMAGE_SIZE) { break; }

        //double d (min(4.0/5, 1/sqrt(nSize*1.0/MAX_IMAGE_SIZE)));
        double d (min(4.0/5, sqrt(MAX_IMAGE_SIZE*1.0/nSize))); // ttt2 review this; the "1.0" is quite wrong in some cases

        i = int(i*d);

        if (i <= 150) { break; }
    }
}




QString ImageInfo::getTextDescr(const QString& qstrSep /* = "\n"*/) const
{
    QString s;
    s.sprintf("%dx%d", getWidth(), getHeight());
    s += qstrSep + TagReader::tr(getImageType());
    return s;
}


void ImageInfo::showFull(QWidget* pParent) const
{
    FullSizeImgDlg dlg (pParent, *this);

    dlg.exec();
}


ostream& operator<<(ostream& out, const AlbumInfo& inf)
{
    out << "title: \"" << inf.m_strTitle << /*"\", artist: \"" << inf.m_strArtist << "\", composer: \"" << inf.m_strComposer <<*/ /*"\", format: \"" << inf.m_strFormat <<*/ "\", genre: \"" << inf.m_strGenre << "\", released: \"" << inf.m_strReleased << "\", var artists: \"" << int(inf.m_eVarArtists) << "\"\n\nnotes: " << inf.m_strNotes << endl;

    /*for (int i = 0, n = cSize(inf.m_vstrImageNames); i < n; ++i)
    {
        out << inf.m_vstrImageNames[i] << endl;
    }*/

    out << "\ntracks:" << endl;
    for (int i = 0, n = cSize(inf.m_vTracks); i < n; ++i)
    {
        out << "pos: \"" << inf.m_vTracks[i].m_strPos << "\", artist: \"" << inf.m_vTracks[i].m_strArtist << "\", title: \"" << inf.m_vTracks[i].m_strTitle << "\", composer: \"" << inf.m_vTracks[i].m_strComposer << "\"" << endl;
    }

    return out;
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


static const set<QString>& getLowerCaseSet()
{
    static set<QString> sqstrLowCase;
    static bool bInit (false);
    if (!bInit)
    {
        bInit = true;
        const char* aszList[] = { "a","an","the", // from http://avalon-internet.com/Capitalize_an_English_Title/en

                "about","above","across","after","against","along", //ttt2 English-only //ttt0 see if there's a need for this in other languages; (this is needed for "title case"); also, this should be tied to a different locale than the one the UI is in
                "amid","among","around","at","before","behind","below", "beneath",
                "beside","besides","between","beyond","but","by","concerning","despite",
                "down","during","except","from","in","including", "inside","into","like",
                "minus","near","notwithstanding","of","off","on", "onto","opposite","out",
                "outside","over","past","per","plus","regarding","since","through",
                "throughout","till","to","toward","towards","under","underneath","unless",
                "unlike","until","up","upon","versus","via","with","within","without",

                "and","but","for","nor","or","so","yet",

                "after","although","as","because","if",
                "lest","than","that","though","when","whereas","while",

                "also","both","each","either","neither","whether",

                0 };

        for (const char** p = &aszList[0]; 0 != *p; ++p)
        {
            sqstrLowCase.insert(*p);
        }

        /* alternative lists/opinions:
            http://aitech.ac.jp/~ckelly/midi/help/caps.html

            http://www.cumbrowski.com/CarstenC/articles/20070623_Title_Capitalization_in_the_English_Language.asp
            http://www.searchenginejournal.com/title-capitalization-in-the-english-language/4882/
            http://ezinearticles.com/?Title-Capitalization-In-The-English-Language&id=658201
        */
    }

    return sqstrLowCase;
}


static const map<QString, QString>& getFixedCaseMap()
{
    static map<QString, QString> mqstrFixedCase;
    static bool bInit (false);
    if (!bInit)
    {
        bInit = true;
        const char* aszList[] = { "I","MTV","L.A.", 0 };

        for (const char** p = &aszList[0]; 0 != *p; ++p)
        {
            mqstrFixedCase[QString(*p).toLower()] = *p;
        }
    }

    return mqstrFixedCase;
}



static void dropPunct(const QString& s, int& i, int& j)
{
    int n (s.size());
    i = 0; j = n;
    for (; i < n && s[i].isPunct(); ++i) {}
    for (; i < n && s[n - 1].isPunct(); --n) {}
}



static QString singleWordFirstLast(const QString& s)
{
    int i, j;
    dropPunct(s, i, j);
    if (i == j) { return s; }

    QString s1 (s.mid(i, j - i).toLower());
    if (getFixedCaseMap().count(s1) > 0)
    {
        s1 = (*getFixedCaseMap().lower_bound(s1)).second;
    }
    else
    {
        s1 = s1.toLower();
        s1[0] = s1[0].toUpper();
    }

    QString s2 (s);
    s2.replace(i, j - i, s1);

    return s2;
}


static QString singleWordMiddleTitle(const QString& s)
{
    int i, j;
    dropPunct(s, i, j);
    if (i == j) { return s; }

    QString s1 (s.mid(i, j - i).toLower());
    if (getFixedCaseMap().count(s1) > 0)
    {
        s1 = (*getFixedCaseMap().lower_bound(s1)).second;
    }
    else if (getLowerCaseSet().count(s1) > 0)
    { // !!! nothing, keep lower
    }
    else
    {
        s1 = s1.toLower();
        s1[0] = s1[0].toUpper();
    }

    QString s2 (s);
    s2.replace(i, j - i, s1);

    return s2;
}



static QString singleWordMiddleSentence(const QString& s)
{
    int i, j;
    dropPunct(s, i, j);
    if (i == j) { return s; }

    QString s1 (s.mid(i, j - i).toLower());
    if (getFixedCaseMap().count(s1) > 0)
    {
        s1 = (*getFixedCaseMap().lower_bound(s1)).second;
    }

    QString s2 (s);
    s2.replace(i, j - i, s1);

    return s2;
}



QString getCaseConv(const QString& s, TextCaseOptions eCase)
{
    switch(eCase)
    {
    //case TC_NONE: CB_ASSERT (false);

    case TC_LOWER: return s.toLower();

    case TC_UPPER: return s.toUpper();

    case TC_TITLE:
        {
            QStringList l (s.split(" ", QString::SkipEmptyParts));
            int n (l.size());
            if (n > 0)
            {
                l[0] = singleWordFirstLast(l[0]);
            }
            if (n > 1)
            {
                l[n - 1] = singleWordFirstLast(l[n - 1]);
            }
            for (int i = 1; i < n - 1; ++i)
            {
                l[i] = singleWordMiddleTitle(l[i]);
            }
            return l.join(" ");
        }

    case TC_SENTENCE:
        {
            QStringList l (s.split(" ", QString::SkipEmptyParts));
            int n (l.size());
            if (n > 0)
            {
                l[0] = singleWordFirstLast(l[0]);
            }
            for (int i = 1; i < n; ++i)
            {
                l[i] = singleWordMiddleSentence(l[i]);
            }
            return l.join(" ");
        }

    default:
        CB_ASSERT (false);
    }

}


const char* getCaseAsStr(TextCaseOptions e)
{
    switch (e)
    {
    case TC_NONE: return QT_TRANSLATE_NOOP("TagReader", "<no change>");
    case TC_LOWER: return QT_TRANSLATE_NOOP("TagReader", "lower case");
    case TC_UPPER: return QT_TRANSLATE_NOOP("TagReader", "UPPER CASE");
    case TC_TITLE: return QT_TRANSLATE_NOOP("TagReader", "Title Case");
    case TC_SENTENCE: return QT_TRANSLATE_NOOP("TagReader", "Sentence case");
    default:
        CB_ASSERT (false);
    }
}

/*static*/ char ExternalToolInfo::s_cSeparator = '|';

ExternalToolInfo::ExternalToolInfo(const string& strSerValue)
{
    string::size_type k1 (strSerValue.find(s_cSeparator));
    string::size_type k2 (strSerValue.find(s_cSeparator, k1 + 1));
    string::size_type k3 (strSerValue.find(s_cSeparator, k2 + 1));
    string::size_type k4 (strSerValue.find(s_cSeparator, k3 + 1));
    CB_CHECK1(k1 != string::npos && k2 != string::npos && k3 != string::npos && k4 == string::npos, InvalidExternalToolInfo());
    m_strName = strSerValue.substr(0, k1);
    m_strCommand = strSerValue.substr(k1 + 1, k2 - k1 -1);
    string s (strSerValue.substr(k2 + 1, k3 - k2 - 1));
    CB_CHECK1(s.size() == 1 && s >= "0" && s <= "2", InvalidExternalToolInfo());
    m_eLaunchOption = (LaunchOption)atoi(s.c_str());
    s = strSerValue.substr(k3 + 1);
    CB_CHECK1(s.size() == 1 && s >= "0" && s <= "1", InvalidExternalToolInfo());
    m_bConfirmLaunch = (bool)atoi(s.c_str());
}


string ExternalToolInfo::asString()
{
    ostringstream out;
    out << m_strName << s_cSeparator << m_strCommand << s_cSeparator << (int)m_eLaunchOption << s_cSeparator << (int)m_bConfirmLaunch;
    return out.str();
}


/*static*/ QString ExternalToolInfo::launchOptionAsTranslatedString(LaunchOption eLaunchOption)
{
    switch (eLaunchOption)
    {
    case DONT_WAIT: return GlobalTranslHlp::tr("Don't wait");
    case WAIT_THEN_CLOSE_WINDOW: return GlobalTranslHlp::tr("Wait for external tool to finish, then close launch window");
    case WAIT_AND_KEEP_WINDOW_OPEN: return GlobalTranslHlp::tr("Wait for external tool to finish, then keep launch window open");
    default: CB_ASSERT(false);
    }
}



