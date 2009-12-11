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


#include  <QFile>
#include  <QFileInfo>
#include  <QDir>

#include  "LyricsStream.h"

#include  "Helpers.h"
#include  "Widgets.h"


using namespace std;

namespace
{
    const char* LYR_BEGIN ("LYRICSBEGIN");
    const int LYR_BEGIN_SIZE (strlen(LYR_BEGIN));
    const char* LYR_END ("LYRICS200");
    const int LYR_END_SIZE (strlen(LYR_END));
}


LyricsStream::LyricsStream()
{
    m_bHasTitle = m_bHasArtist = m_bHasGenre = m_bHasImage = m_bHasAlbum = false;
}


LyricsStream::LyricsStream(int nIndex, NoteColl& notes, std::istream& in, const std::string& strFileName) : DataStream(nIndex), m_pos(in.tellg()), m_strFileName(strFileName)
{
    StreamStateRestorer rst (in);
    m_bHasTitle = m_bHasArtist = m_bHasGenre = m_bHasImage = m_bHasAlbum = false;

    //const int BFR_SIZE (256);
    //char bfr [BFR_SIZE];
    vector<char> vcBfr (256);

    streamoff nRead (read(in, &vcBfr[0], LYR_BEGIN_SIZE + LYR_END_SIZE));

    MP3_CHECK_T (nRead >= LYR_BEGIN_SIZE + LYR_END_SIZE && 0 == strncmp(LYR_BEGIN, &vcBfr[0], LYR_BEGIN_SIZE), m_pos, "Invalid Lyrics stream tag. Header not found.", NotLyricsStream());

    streampos pos (m_pos);
    int nTotalSize (LYR_BEGIN_SIZE);
    pos += LYR_BEGIN_SIZE;
    for (;;)
    {
        in.seekg(pos);
        vcBfr.resize(8 + 1);
        streamoff nRead (read(in, &vcBfr[0], 8));
        MP3_CHECK (8 == nRead, pos, lyrTooShort, NotLyricsStream());

        char* pLast;
        int nSize;

        if (isdigit(vcBfr[0]))
        { // the size, folowed by the end
            vcBfr.resize(6 + LYR_END_SIZE);
            nRead = read(in, &vcBfr[8], 6 + LYR_END_SIZE - 8);
            MP3_CHECK (6 + LYR_END_SIZE - 8 == nRead, pos, lyrTooShort, NotLyricsStream());
            char c (vcBfr[6]);
            vcBfr[6] = 0;
            nSize = int(strtol(&vcBfr[0], &pLast, 10));
            vcBfr[6] = c;
            MP3_CHECK (pLast == &vcBfr[6], pos, invalidLyr, NotLyricsStream());

            MP3_CHECK (nTotalSize == nSize, pos, invalidLyr, NotLyricsStream());
            MP3_CHECK (0 == strncmp(LYR_END, &vcBfr[6], LYR_END_SIZE), pos, invalidLyr, NotLyricsStream());

            pos += 6 + LYR_END_SIZE;
            break;
        }

        string strField (&vcBfr[0], &vcBfr[0] + 3);
        vcBfr[8] = 0;
        nSize = int(strtol(&vcBfr[3], &pLast, 10));
        MP3_CHECK (pLast == &vcBfr[8], pos, invalidLyr, NotLyricsStream());
        vcBfr.resize(nSize);
        nRead = read(in, &vcBfr[0], nSize);
        MP3_CHECK (nSize == nRead, pos, lyrTooShort, NotLyricsStream());
        vcBfr.push_back(0);

        string strVal (convStr(QString::fromLatin1(&vcBfr[0])));

        //qDebug("field %s, size %d, pos %x, val %s", strField.c_str(), nSize, int(pos), strVal.c_str());

        if ("LYR" == strField)
        {
            if (m_strLyrics.empty())
            {
                m_strLyrics = strVal;
                //QQQ = true;
            }
            else
            {
                MP3_NOTE (m_pos, duplicateFields); //ttt2 perhaps differentiate between various dup fields
                m_strOther += "Additional LYR field: " + strVal + "\n\n";
            }
        }
        else if ("INF" == strField)
        {
            MP3_NOTE (m_pos, infInLyrics);
            m_strOther += "INF field: " + strVal + "\n\n";
        }
        else if ("AUT" == strField)
        {
            if (m_strAuthor.empty())
            {
                m_strAuthor = strVal;
                //QQQ = true;
            }
            else
            {
                MP3_NOTE (m_pos, duplicateFields);
                m_strOther += "Additional AUT field: " + strVal + "\n\n";
            }
        }
        else if ("EAL" == strField)
        {
            if (m_strAlbum.empty())
            {
                m_strAlbum = strVal;
                m_bHasAlbum = true;
            }
            else
            {
                MP3_NOTE (m_pos, duplicateFields);
                m_strOther += "Additional EAL field: " + strVal + "\n\n";
            }
        }
        else if ("EAR" == strField)
        {
            if (m_strArtist.empty())
            {
                m_strArtist = strVal;
                m_bHasArtist = true;
            }
            else
            {
                MP3_NOTE (m_pos, duplicateFields);
                m_strOther += "Additional EAR field: " + strVal + "\n\n";
            }
        }
        else if ("ETT" == strField)
        {
            if (m_strTitle.empty())
            {
                m_strTitle = strVal;
                m_bHasTitle = true;
            }
            else
            {
                MP3_NOTE (m_pos, duplicateFields);
                m_strOther += "Additional ETT field: " + strVal + "\n\n";
            }
        }
        else if ("IMG" == strField)
        {
            if (m_strImageFiles.empty())
            {
                m_strImageFiles = strVal;
                m_bHasImage = true;
                //MP3_NOTE (m_pos, imgInLyrics);
            }
            else
            {
                MP3_NOTE (m_pos, duplicateFields);
                m_strOther += "Additional IMG field: " + strVal + "\n\n";
            }
        }
        else if ("GRE" == strField)
        {
            if (m_strGenre.empty())
            {
                m_strGenre = strVal;
                m_bHasGenre = true;
            }
            else
            {
                MP3_NOTE (m_pos, duplicateFields);
                m_strOther += "Additional GRE field: " + strVal + "\n\n";
            }
        }
        else if ("IND" == strField)
        {
            if (m_strInd.empty())
            {
                m_strInd = strVal;
            }
            else
            {
                MP3_NOTE (m_pos, duplicateFields);
                m_strOther += "Additional IND field: " + strVal + "\n\n";
            }
        }
        else
        {
            m_strOther += strField + " field: " + strVal + "\n\n";
        }

        nTotalSize += 8 + nSize;
        pos += 8 + nSize;
    }

    in.seekg(pos);
    m_nSize = pos - m_pos;

    //m_strOther=string(512, 'a'); m_strOther[391] = 10; // err
    //m_strOther=string(512, 'a'); m_strOther[303] = 10; // err
    //m_strOther=string(512, 'a'); m_strOther[302] = 10; // ok
    //m_strOther=string(511, 'a'); m_strOther[303] = 10; // ok

    MP3_TRACE (m_pos, "LyricsStream built.");

    rst.setOk();
}


/*override*/ void LyricsStream::copy(std::istream& in, std::ostream& out)
{
    appendFilePart(in, out, m_pos, m_nSize);
}


/*override*/ std::string LyricsStream::getInfo() const
{
    return "";
}





/*override*/ std::string LyricsStream::getTitle(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = m_bHasTitle; }
    return m_strTitle;
}

/*override*/ std::string LyricsStream::getArtist(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = m_bHasArtist; }
    return m_strArtist;
}

/*override*/ std::string LyricsStream::getGenre(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = m_bHasGenre; }
    return m_strGenre;
}


ImageInfo LyricsStream::readImage(const QString& strRelName) const //ttt2 perhaps move to Helpers and use for ID3V2 links as well; keep in mind that in many places link is considered invalid, so they would need updating;
{
    QString qs (QFileInfo(convStr(m_strFileName)).dir().filePath(strRelName));

    //qs = convStr(m_strFileName) + getPathSep() + qs;

    QFile f (qs);
    bool bRes (false);

    if (f.open(QIODevice::ReadOnly))
    {
        CursorOverrider crsOv;

        int nSize ((int)f.size());
        QByteArray comprImg (f.read(nSize));
        QImage pic; //ttt1 rename QImage variables as "img"

        if (pic.loadFromData(comprImg))
        {
            bRes = true;

            ImageInfo::Compr eCompr;
            int nWidth, nHeight;

            if (nSize <= ImageInfo::MAX_IMAGE_SIZE)
            {
                nWidth = pic.width(); nHeight = pic.height();
                eCompr = qs.endsWith(".png", Qt::CaseInsensitive) ? ImageInfo::PNG : (qs.endsWith(".jpg", Qt::CaseInsensitive) || qs.endsWith(".jpeg", Qt::CaseInsensitive) ? ImageInfo::JPG : ImageInfo::INVALID); //ttt2 add more cases if supporting more image types
            }
            else
            {
                QImage scaledImg;
                ImageInfo::compress(pic, scaledImg, comprImg);
                nWidth = scaledImg.width(); nHeight = scaledImg.height();
                eCompr = ImageInfo::JPG;
            }

            ImageInfo img (-1, ImageInfo::OK, eCompr, comprImg, nWidth, nHeight);

            return img;
        }
    }

    return ImageInfo(-1, ImageInfo::ERROR_LOADING);
}


/*override*/ ImageInfo LyricsStream::getImage(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = m_bHasImage; }

    if (m_bHasImage)
    {
        // http://www.id3.org/Lyrics3v2
        // http://www.mpx.cz/mp3manager/tags.htm

        QString qs (convStr(m_strImageFiles));
        {
            int k (qs.indexOf("\r\n"));
            if (-1 != k)
            {
                qs.remove(k, qs.size());
            }
        }

        return readImage(qs);
    }
    return ImageInfo();
}


/*override*/ std::vector<ImageInfo> LyricsStream::getImages() const
{
    vector<ImageInfo> v;

    if (m_strImageFiles.empty()) { return v; }

    QString qs (convStr(m_strImageFiles) + "\r\n");

    for (;;)
    {
        int k (qs.indexOf("\r\n"));
        if (-1 == k) { return v; }

        v.push_back(readImage(qs.left(k)));
        qs.remove(0, k + 2);
    }
}


/*override*/ std::string LyricsStream::getAlbumName(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = m_bHasAlbum; }
    return m_strAlbum;
}

/*override*/ std::string LyricsStream::getOtherInfo() const
{
    string s;
    if (!m_strInd.empty()) { s += "IND field: " + m_strInd + "\n\n"; }
    if (!m_strImageFiles.empty()) { s += "IMG field: " + m_strImageFiles + "\n\n"; }
    if (!m_strAuthor.empty()) { s += "AUT field: " + m_strAuthor + "\n\n"; }
    if (!m_strLyrics.empty()) { s += "LYR field: " + m_strLyrics + "\n\n"; }

    if (!m_strOther.empty()) { s += m_strOther + "\n\n"; }
    return s;
}

#if 1 //...
/*override*/ TagReader::SuportLevel LyricsStream::getSupport(Feature eFeature) const
{
    switch (eFeature)
    {
    case TITLE:
    case ARTIST:
    case GENRE:
    case IMAGE:
    case ALBUM:
        return READ_ONLY;

    default:
        return NOT_SUPPORTED;
    }
}

#endif


//ttt2 perhaps remove \r
