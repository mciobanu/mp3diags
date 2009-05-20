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


#include  <sstream>
#include  <fstream>

#include  "Id3V2Stream.h"

#include  "MpegStream.h"
#include  "Helpers.h"


using namespace std;
using namespace pearl;









/*
     ID3V2.3

     +-----------------------------+
     |      Header (10 bytes)      |
     +-----------------------------+
     |       Extended Header       |
     | (variable length, OPTIONAL) |
     +-----------------------------+
     |   Frames (variable length)  |
     +-----------------------------+
     |           Padding           |
     | (variable length, OPTIONAL) |
     +-----------------------------+
     | Footer (10 bytes, OPTIONAL) |
     +-----------------------------+

http://osdir.com/ml/multimedia.id3v2/2007-08/msg00008.html :

For v2.3 tags, you would read the first ten bytes (tag header) and then, if there is an extended header, read 4 bytes to get its size and then skip that many bytes. (The padding that is given in the extended header comes AFTER the frames.)

For v2.4 tags, you would read the first ten bytes (tag header) and then, if there is an extended header, read 4 bytes to get its size and then skip (size - 4) bytes ahead. (I believe in v2.4 tags, the size given in  the extended header includes the 4 bytes you would have already read.)  And don't forget that in v2.4 tags, the extended header size is stored as a syncsafe integer.




*/

// the total size, including the 10-byte header
int getId3V2Size(char* pId3Header)
{
    unsigned char* p (reinterpret_cast<unsigned char*>(pId3Header));
    return
        (p[6] << 21) +
        (p[7] << 14) +
        (p[8] << 7) +
        (p[9] << 0) +
        10;
}






// reads nCount bytes into pDest;
// if bHasUnsynch is true, it actually reads more bytes, applying the unsynchronization algorithm, so pDest gets nCount bytes;
// returns the number of bytes it could read;
// posNext is the position where the next block begins (might be EOF); nothing starting at that position should be read; needed to take care of an ID3V2 tag whose last frame ends with 0xff and has no padding;
// asserts that posNext is not before the current position in the stream
streamsize readID3V2(bool bHasUnsynch, istream& in, char* pDest, streamsize nCount, streampos posNext, int& nBytesSkipped)
{
    nBytesSkipped = 0;
    if (0 == nCount) { return 0; }
    streampos posCrt (in.tellg());
    CB_ASSERT (posNext >= posCrt);
    if (nCount > posNext - posCrt)
    {
        nCount = posNext - posCrt;
    }

    if (!bHasUnsynch)
    {
        return read(in, pDest, nCount);
    }

    const int BFR_SIZE (256);
    char bfr [BFR_SIZE];
    char cPrev (0);
    char* q (pDest);
    for (;;)
    {
        streamsize nTarget (min(streamsize(BFR_SIZE), streamsize(posNext - posCrt)));
        streamsize nRead (read(in, bfr, nTarget));
        if (0 == nRead)
        { // doesn't matter what was read before; EOF reached
            goto e1;
        }
        char* p (bfr);
        for (;;)
        {
            if (posCrt >= posNext) { goto e1; }
            if (p >= bfr + nRead) { break; }
            if (0 == *p && char(0xff) == cPrev)
            {
                ++p;
                posCrt += 1;
                ++nBytesSkipped;
                cPrev = 0;
                if (posCrt >= posNext) { goto e1; }
                if (p >= bfr + nRead) { break; }
            }

            if (q >= pDest + nCount) { goto e1; }

            cPrev = *q++ = *p++;
            posCrt += 1;
        }
    }

e1:
    in.clear();
    in.seekg(posCrt);
    return streamsize(q - pDest);
}



//============================================================================================================
//============================================================================================================
//============================================================================================================


Id3V2Frame::Id3V2Frame(streampos pos, bool bHasUnsynch, StringWrp* pFileName) :
        m_nMemDataSize(-1),
        m_pos(pos),
        m_pFileName(pFileName),
        m_bHasUnsynch(bHasUnsynch),
        m_bHasLatin1NonAscii(false),
        m_eApicStatus(NO_APIC),
        m_nPictureType(-1),
        m_nImgOffset(-1),
        m_nImgSize(-1),
        m_eCompr(ImageInfo::INVALID)
{
}

/*virtual*/ Id3V2Frame::~Id3V2Frame()
{
    //qDebug("Id3V2Frame::~Id3V2Frame(%p)", this);
}

static bool isReadable(char c)
{
    return c > 32 && c < 127;
}


// normally returns m_szName, but if it has invalid characters (<=32 or >=127), returns a hex representation
string Id3V2Frame::getReadableName() const
{
    if (isReadable(m_szName[0]) && isReadable(m_szName[1]) && isReadable(m_szName[2]) && isReadable(m_szName[3]))
    {
        return m_szName;
    }
    char a [32];
    sprintf(a, "0x%02x 0x%02x 0x%02x 0x%02x",
        (unsigned)(unsigned char)m_szName[0],
        (unsigned)(unsigned char)m_szName[1],
        (unsigned)(unsigned char)m_szName[2],
        (unsigned)(unsigned char)m_szName[3]);
    return a;
}


void Id3V2Frame::print(ostream& out) const
{
    out << m_szName;
    if ('T' == m_szName[0])
    {
        out << "=\"" << getUtf8String() << "\""; //ttt2 probably specific to particular versions of Linux and GCC
//cout << " value=\"" << getUtf8String() << "\""; //ttt2 probably specific to particular versions of Linux and GCC
        //out << " value=\"" << "RRRRRRRR" << "\"";
    }
    else
    {
        out << " size=" << m_nMemDataSize;
    }

    if (0 == strcmp(KnownFrames::LBL_IMAGE(), m_szName))
    {
        out << " status=";
        switch(m_eApicStatus)
        {
        case USES_LINK: out << "link"; break;
        case NOT_SUPPORTED: out << "not supported"; break;
        case ERROR: out << "error"; break;
        case OK: out << "OK"; break;
        default: CB_ASSERT(false);
        }
    }

}


/*static*/ string Id3V2Frame::utf8FromBomUtf16(const char* pData, int nSize)
{
    CB_CHECK1 (nSize > 1, NotId3V2Frame()); // UNICODE string entries must have a size of 3 or more."
    const unsigned char* p (reinterpret_cast<const unsigned char*> (pData));
    CB_CHECK1 ((0xff == p[0] && 0xfe == p[1]) || (0xff == p[1] && 0xfe == p[0]), NotId3V2Frame());

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    bool bIsFffeOk (true); // x86
#else
    bool bIsFffeOk (false);
#endif

    pData += 2;

    vector<char> v;
    int nUSize ((nSize - 2)/2); // ttt3 maybe check that nSize is an even number, but not sure what to do if it isn't
    if ((0xff == *p && !bIsFffeOk) || (0xff != *p && bIsFffeOk))
    { // swap bytes so QString would understand them; it might seem useful for QString to understand BOM, but it doesn't; so ...
        v.resize(nUSize*2);
        for (int i = 0; i < nUSize; ++i)
        {
            v[i*2] = pData[i*2 + 1];
            v[i*2 + 1] = pData[i*2];
        }

        pData = &v[0];
    }
    const ushort* pUs (reinterpret_cast<const ushort*>(pData));
    QString qs (QString::fromUtf16(pUs, nUSize));
    string s (convStr(qs));

    rtrim(s);
    return s;
}




//============================================================================================================
//============================================================================================================
//============================================================================================================


Id3V2FrameDataLoader::Id3V2FrameDataLoader(const Id3V2Frame& frame) : m_frame(frame)
{
    if (cSize(frame.m_vcData) < m_frame.m_nMemDataSize)
    {
        m_bOwnsData = true;
        CB_ASSERT (frame.m_vcData.empty());
        CB_ASSERT (0 != frame.m_pFileName);
        char* pData (new char[m_frame.m_nMemDataSize]);
        m_pData = pData;
        ifstream in (m_frame.m_pFileName->s.c_str(), ios::binary);
        in.seekg(m_frame.m_pos);
        in.seekg(m_frame.m_nDiskHdrSize, ios_base::cur);
        streampos posNext (m_frame.m_pos);
        posNext += m_frame.m_nDiskDataSize + m_frame.m_nDiskHdrSize;
        int nContentBytesSkipped (0);
        int nRead (0);
        nRead = readID3V2(m_frame.m_bHasUnsynch, in, pData, m_frame.m_nMemDataSize, posNext, nContentBytesSkipped);
        //qDebug("nRead %d ; m_frame.m_nMemDataSize %d ; nContentBytesSkipped %d ", nRead, m_frame.m_nMemDataSize, nContentBytesSkipped);
        if (m_frame.m_nMemDataSize != nRead)
        {
            delete[] m_pData;
            throw LoadFailure();
        }
    }
    else
    {
        m_bOwnsData = false;
        m_pData = &frame.m_vcData[0];
    }
}


Id3V2FrameDataLoader::~Id3V2FrameDataLoader()
{
    if (m_bOwnsData)
    {
        delete[] m_pData;
    }
}








//============================================================================================================
//============================================================================================================
//============================================================================================================

/*static*/ const char* KnownFrames::LBL_TITLE() { return "TIT2"; }
/*static*/ const char* KnownFrames::LBL_ARTIST() { return "TPE1"; }
/*static*/ const char* KnownFrames::LBL_TRACK_NUMBER() { return "TRCK"; }
/*static*/ const char* KnownFrames::LBL_TIME_YEAR_230() { return "TYER"; }
/*static*/ const char* KnownFrames::LBL_TIME_DATE_230() { return "TDAT"; }
/*static*/ const char* KnownFrames::LBL_TIME_240() { return "TDRC"; }
/*static*/ const char* KnownFrames::LBL_GENRE() { return "TCON"; }
/*static*/ const char* KnownFrames::LBL_IMAGE() { return "APIC"; }
/*static*/ const char* KnownFrames::LBL_ALBUM() { return "TALB"; }
/*static*/ const char* KnownFrames::LBL_RATING() { return "POPM"; }
/*static*/ const char* KnownFrames::LBL_COMPOSER() { return "TCOM"; }



//============================================================================================================
//============================================================================================================
//============================================================================================================



Id3V2StreamBase::Id3V2StreamBase(int nIndex, istream& in, StringWrp* pFileName) :
        DataStream(nIndex),

        m_nPaddingSize(0),
        m_pos(in.tellg()),
        m_pFileName(pFileName),

        m_eImageStatus(ImageInfo::NO_PICTURE_FOUND),
        m_pPicFrame(0)
{
}



/*override*/ Id3V2StreamBase::~Id3V2StreamBase()
{
    clearPtrContainer(m_vpFrames);
}





bool Id3V2StreamBase::hasUnsynch() const
{
    return 0 != (m_cFlags & 0x80);
}


void Id3V2StreamBase::printFrames(ostream& out) const
{
    for (vector<Id3V2Frame*>::const_iterator it = m_vpFrames.begin(), end = m_vpFrames.end(); it != end; ++it)
    {
        (*it)->print(out);
//(*it)->print(cout);
    }
}



/*override*/ void Id3V2StreamBase::copy(std::istream& in, std::ostream& out)
{
    appendFilePart(in, out, m_pos, m_nTotalSize); //ttt1
}



/*override*/ std::string Id3V2StreamBase::getInfo() const
{
    ostringstream out;
    out << "padding=" << m_nPaddingSize << ", unsynch=" << (hasUnsynch() ? "YES" : "NO") << "; frames: ";
    bool bFirst (true);
    for (vector<Id3V2Frame*>::const_iterator it = m_vpFrames.begin(), end = m_vpFrames.end(); it != end; ++it)
    {
        if (!bFirst) { out << ", "; }
        bFirst = false;
        (*it)->print(out);
    }
    string s (out.str());
//cout << s << endl;
//printHex(s, cout, false);
    return s;
}



Id3V2Frame* Id3V2StreamBase::findFrame(const char* szFrameName) //ttt2 finds the first frame, but doesn't care about duplicates
{
    for (int i = 0, n = cSize(m_vpFrames); i < n; ++i)
    {
        Id3V2Frame* p = m_vpFrames[i];
        if (0 == strcmp(szFrameName, p->m_szName)) { return p; }
    }
    return 0;
}


const Id3V2Frame* Id3V2StreamBase::findFrame(const char* szFrameName) const //ttt2 finds the first frame, but doesn't care about duplicates
{
    for (int i = 0, n = cSize(m_vpFrames); i < n; ++i)
    {
        const Id3V2Frame* p = m_vpFrames[i];
        if (0 == strcmp(szFrameName, p->m_szName)) { return p; }
    }
    return 0;
}





/*override*/ std::string Id3V2StreamBase::getTitle(bool* pbFrameExists /*= 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_TITLE()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}



/*override*/ std::string Id3V2StreamBase::getArtist(bool* pbFrameExists /*= 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_ARTIST()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}


/*override*/ std::string Id3V2StreamBase::getTrackNumber(bool* pbFrameExists /*= 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_TRACK_NUMBER()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}

/*static bool isNum(const string& s)
{
    if (s.empty()) { return false; }
    for (int i = 0, n = cSize(s); i < n; ++i)
    {
        if (!isdigit(s[i])) { return false; }
    }
    return true;
}*/

static string decodeGenre(const string& s)
{
    string strRes;
    const char* q (s.c_str());
    while ('(' == *q && '(' != *(q + 1))
    {
        const char* q1 (q + 1);
        if (!isdigit(*q1)) { return s; } // error
        for (; isdigit(*q1); ++q1) {}
        if (')' != *q1) { return s; } // error

        if (!strRes.empty()) { strRes += " / "; }
        strRes += getId3V1Genre(atoi(q + 1));
        q = q1 + 1;
    }

    if ('(' == *q && '(' == *(q + 1)) { ++q; }
    if (0 != *q)
    {
        if (!strRes.empty()) { strRes += " / "; }
        strRes += q;
    }

    return strRes;
}


/*override*/ std::string Id3V2StreamBase::getGenre(bool* pbFrameExists /*= 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_GENRE())); // for valid formats see tstGenre() and http://www.id3.org/id3v2.3.0#head-42b02d20fb8bf48e38ec5415e34909945dd849dc
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }

    string s (p->getUtf8String());
    /*int n (cSize(s));
    if (n > 2 && '(' == s[0] && ')' == s[n - 1] && isNum(s.substr(1, n - 2)))
    {
        return getId3V1Genre(atoi(s.c_str() + 1));
    }

    if (isNum(s))
    {
        return getId3V1Genre(atoi(s.c_str()));
    }*/

    return decodeGenre(s);
}

/*
void tstGenre() //ttt2 remove
{
    cout << "\nGenre test\n";
    { string s ("gaga"); cout << "*" << s << "*" << decodeGenre(s) << "*\n"; }
    { string s ("(10)gaga"); cout << "*" << s << "*" << decodeGenre(s) << "*\n"; }
    { string s ("(10)(83)gaga"); cout << "*" << s << "*" << decodeGenre(s) << "*\n"; }
    { string s ("(10)(83)((gaga)"); cout << "*" << s << "*" << decodeGenre(s) << "*\n"; }
    { string s ("(10a)gaga"); cout << "*" << s << "*" << decodeGenre(s) << "*\n"; }
    { string s ("(b10)gaga"); cout << "*" << s << "*" << decodeGenre(s) << "*\n"; }
}
*/


void Id3V2StreamBase::checkFrames(NoteColl& notes) // various checks to be called from derived class' constructor
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_GENRE()));
    if (0 != p)
    {
        string s (p->getUtf8String());
        /*int n (cSize(s));
        if (n > 2 && '(' == s[0] && ')' == s[n - 1] && isNum(s.substr(1, n - 2)))
        {
            MP3_NOTE (p->m_pos, "Numerical value between parantheses found as track genre. The standard specifies a numerical value, but most applications use a descriptive name instead.");
        }
        else if (isNum(s))
        {
            MP3_NOTE (p->m_pos, "Numerical value found as track genre. While this is consistent with the standard, most applications use a descriptive name instead.");
        }
        else */if (s.empty())
        {
            MP3_NOTE (p->m_pos, id3v2EmptyTcon);
        }
    }

    //ttt2 add other checks
}
//ttt1 perhaps use links to pictures in crt dir

/*override*/ ImageInfo Id3V2StreamBase::getImage(bool* pbFrameExists /*= 0*/) const
{
//if (0 != pbFrameExists) { *pbFrameExists = false; } return ImageInfo(ImageInfo::NO_PICTURE_FOUND);

    if (0 != pbFrameExists)
    {
        const Id3V2Frame* p (findFrame(KnownFrames::LBL_IMAGE()));
        *pbFrameExists = 0 != p;
    }

    //ImageInfo res;

    //res.m_eStatus = m_eImageStatus;

    if (ImageInfo::OK != m_eImageStatus && ImageInfo::LOADED_NOT_COVER != m_eImageStatus)
    {
        CB_ASSERT (0 == m_pPicFrame);
        return ImageInfo(-1, m_eImageStatus);
    }

    CB_ASSERT (0 != m_pPicFrame);
    try
    {
        Id3V2FrameDataLoader wrp (*m_pPicFrame);
        const char* pCrtData (wrp.getData());
        const char* pBinData (pCrtData + m_pPicFrame->m_nImgOffset);
        //CB_CHECK (pixmap.loadFromData(pBinData, m_nImgSize));

        // make sure the data is still available and correct (the file might have been modified externally)
        QPixmap pic;
        if (pic.loadFromData(reinterpret_cast<const unsigned char*>(pBinData), m_pPicFrame->m_nImgSize))
        {
            QByteArray b (QByteArray::fromRawData(pBinData, m_pPicFrame->m_nImgSize));
            b.append('x'); b.resize(b.size() - 1); // !!! these are needed because fromRawData() doesn't create copies of the memory used for the byte array
            return ImageInfo(m_pPicFrame->m_nPictureType, m_eImageStatus, m_pPicFrame->m_eCompr, b, pic.width(), pic.height());
        }
        //QBuffer bfr (&res.m_compressedImg);
        //bfr.
        //res.m_compressedImg = QByteArray(fromRawData
        //delete pPictureInfo;
    }
    catch (const Id3V2FrameDataLoader::LoadFailure&)
    {
        //eImageStatus = ImageInfo::ERROR_LOADING;
    }

    trace("The picture could be loaded before but now this is no longer possible. The most likely reason is that the file was moved or changed by an external application.");

    return ImageInfo(-1, ImageInfo::ERROR_LOADING);
}



/*static*/ const char* Id3V2StreamBase::decodeApic(NoteColl& notes, streampos pos, const char* pData, const char*& szMimeType, int& nPictureType, const char*& szDescription)
{
    MP3_CHECK (0 == pData[0], pos, id3v2UnsupApicTextEnc, NotSupTextEnc()); // !!! there's no need for StreamIsUnsupported here, because this error is not fatal, and it isn't allowed to propagate, therefore doesn't cause a stream to be Unsupported; //ttt1 review, support
    ++pData;
    szMimeType = pData;
    int nMimeSize (strlen(pData));

    pData += 1 + nMimeSize;
    nPictureType = *pData++;

    szDescription = pData;

    return pData + strlen(szDescription) + 1;
}


static bool isTypeSupported(int nType)
{
    //return Id3V2Frame::OTHER == nType || Id3V2Frame::ICON == nType || Id3V2Frame::COVER == nType; //ttt1 review decision to have all these map to "cover"; see also Mp3HandlerTagData::reload(), where saving of a "cover" image is influenced by what this returned
    return Id3V2Frame::COVER == nType; // 2009.04.05 - for a while it seemed a good idea to report OTHER and ICON as "supported", but there's the issue of what to do when deleting an image in the tag editor; seems better to just use the cover; //ttt1 OTOH this creates more duplicates
}



void Id3V2StreamBase::preparePictureHlp(NoteColl& notes, Id3V2Frame* pFrame, const char* pFrameData, const char* pImgData, const char* szMimeType)
{
    if (0 == strcmp("-->", szMimeType))
    {
        MP3_NOTE (pFrame->m_pos, id3v2LinkInApic);
        pFrame->m_eApicStatus = Id3V2Frame::USES_LINK;
        return;
    }

    //QPixmap img; // !!! QPixmap can only be used in GUI threads, so QImage must be used instead: http://lists.trolltech.com/qt-interest/2005-02/thread00008-0.html or http://lists.trolltech.com/qt-interest/2006-11/thread00045-0.html
    QImage img;
    const unsigned char* pBinData (reinterpret_cast<const unsigned char*>(pImgData));
    int nSize (pFrame->m_nMemDataSize - (pImgData - pFrameData));
    if (img.loadFromData(pBinData, nSize))
    {
        pFrame->m_nImgSize = nSize;
        pFrame->m_nImgOffset = pImgData - pFrameData;
        pFrame->m_eApicStatus = Id3V2Frame::OK;
        if (0 == strcmp("image/jpeg", szMimeType) || 0 == strcmp("image/jpg", szMimeType))
        {
            pFrame->m_eCompr = ImageInfo::JPG;
        }
        else if (0 == strcmp("image/png", szMimeType))
        {
            pFrame->m_eCompr = ImageInfo::PNG;
        }
        else
        {
            pFrame->m_eCompr = ImageInfo::INVALID;
        } //ttt1 perhaps support GIF or other formats
        return;
    }

    pFrame->m_eApicStatus = Id3V2Frame::ERROR;

    if (pFrame->m_nMemDataSize > 100)
    {
        MP3_NOTE (pFrame->m_pos, id3v2ErrorLoadingApic);
    }
    else
    {
        MP3_NOTE (pFrame->m_pos, id3v2ErrorLoadingApicTooShort);
    }
}



void Id3V2StreamBase::preparePicture(NoteColl& notes) // initializes fields used by the APIC frame
{
    const char* szMimeType;
    const char* szDescription;
    Id3V2Frame* pFirstValidApicFrame (0);
    Id3V2Frame* pFirstApicFrame (0);

    for (int i = 0, n = cSize(m_vpFrames); i < n; ++i)
    {
        Id3V2Frame* p = m_vpFrames[i];
        if (0 == strcmp(KnownFrames::LBL_IMAGE(), p->m_szName))
        {
            if (0 == pFirstApicFrame) { pFirstApicFrame = p; }
            try
            {
                Id3V2FrameDataLoader wrp (*p);
                const char* pData (wrp.getData());
                const char* pCrtData (0);

                try
                {
                    pCrtData = decodeApic(notes, p->m_pos, pData, szMimeType, p->m_nPictureType, szDescription);
                }
                catch (const NotSupTextEnc&)
                {
                    p->m_eApicStatus = Id3V2Frame::NOT_SUPPORTED;
                    continue;
                }
                if (0 != *szDescription) { MP3_NOTE (p->m_pos, id3v2PictDescrIgnored); }

                preparePictureHlp(notes, p, pData, pCrtData, szMimeType);

                if (Id3V2Frame::OK == p->m_eApicStatus)
                {
                    if (isTypeSupported(p->m_nPictureType))
                    {
                        m_eImageStatus = ImageInfo::OK;
                        m_pPicFrame = p;
                    }
                    else if (0 == pFirstValidApicFrame)
                    {
                        pFirstValidApicFrame = p;
                    }
                }

            }
            catch (const Id3V2FrameDataLoader::LoadFailure&)
            {
                MP3_NOTE (p->m_pos, fileWasChanged);
            }
        }
    }


    if (ImageInfo::OK == m_eImageStatus)
    {
        return;
    }

    // no frame with supported type (0, 1, or 3) was found; just pick the first APIC frame and use it

    if (ImageInfo::NO_PICTURE_FOUND == m_eImageStatus && 0 != pFirstValidApicFrame)
    {
        m_eImageStatus = ImageInfo::LOADED_NOT_COVER;
        m_pPicFrame = pFirstValidApicFrame;
        return;
    }

    if (0 == pFirstApicFrame)
    {
        return;
    }

    switch (pFirstApicFrame->m_eApicStatus)
    {
    case Id3V2Frame::USES_LINK: m_eImageStatus = ImageInfo::USES_LINK; return;
    case Id3V2Frame::ERROR: m_eImageStatus = ImageInfo::ERROR_LOADING; return;
    default: CB_ASSERT(false); // all cases should have been covered
    }

}




/*override*/ std::string Id3V2StreamBase::getAlbumName(bool* pbFrameExists /*= 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_ALBUM()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}


/*override*/ std::string Id3V2StreamBase::getComposer(bool* pbFrameExists /*= 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_COMPOSER()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}


/*override*/ double Id3V2StreamBase::getRating(bool* pbFrameExists /*= 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_RATING()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return -1; }
    Id3V2FrameDataLoader wrp (*p);
    const char* pData (wrp.getData());
    int n (p->m_nMemDataSize);
    int k (0);
    while (k < n && 0 != pData[k]) { ++k; } // skip email addr
    ++k;
    if (k >= n)
    { // error //ttt2 add warning on constructor
        return -1;
    }
    unsigned char c (pData[k]);
    return 5*(double(c) - 1)/254;  // ttt1 not sure this is the best mapping
}


static const char* getId3V2ClassDisplayName() // needed so pointer comparison can be performed for Id3V2StreamBase::getClassDisplayName() regardless of the template param
{
    return "ID3V2";
}



/*static*/ const char* Id3V2StreamBase::getClassDisplayName()
{
    return getId3V2ClassDisplayName();
}



// returns a frame with the given name; normally it returns the first such frame, but it may return another if there's a good reason; returns 0 if no frame was found;
const Id3V2Frame* Id3V2StreamBase::getFrame(const char* szName) const
{
    if (0 == strcmp(szName, KnownFrames::LBL_IMAGE()))
    {
        return m_pPicFrame;
    }

    return findFrame(szName);
}


/*static*/ const set<string>& KnownFrames::getKnownFrames()
{
    static bool bFirstTime (true);
    static set<string> sKnownFrames;
    if (bFirstTime)
    {
        sKnownFrames.insert(KnownFrames::LBL_TITLE());
        sKnownFrames.insert(KnownFrames::LBL_ARTIST());
        sKnownFrames.insert(KnownFrames::LBL_TRACK_NUMBER());
        sKnownFrames.insert(KnownFrames::LBL_TIME_YEAR_230());
        sKnownFrames.insert(KnownFrames::LBL_TIME_DATE_230());
        sKnownFrames.insert(KnownFrames::LBL_TIME_240()); //ttt2 perhaps this shouldn't be used for 2.3.0, but it covers cases like reading 2.4.0 and writing 2.3.0 or bugs by some tools
        sKnownFrames.insert(KnownFrames::LBL_GENRE());
        sKnownFrames.insert(KnownFrames::LBL_IMAGE());
        sKnownFrames.insert(KnownFrames::LBL_ALBUM());
        sKnownFrames.insert(KnownFrames::LBL_RATING());
        sKnownFrames.insert(KnownFrames::LBL_COMPOSER());

        bFirstTime = false;
    }

    return sKnownFrames;
}



/*override*/ std::string Id3V2StreamBase::getOtherInfo() const
{
    const set<string>& sKnownFrames (KnownFrames::getKnownFrames());

    set<string> sUsedFrames;

    //string strRes;
    ostringstream out;
    bool b (false);

    for (int i = 0, n = cSize(m_vpFrames); i < n; ++i)
    {
        Id3V2Frame* p = m_vpFrames[i];
        if (sKnownFrames.count(p->m_szName) > 0 && sUsedFrames.count(p->m_szName) == 0)
        {
            sUsedFrames.insert(p->m_szName);
        }
        else
        {
            if (b) { out << ", "; }
            b = true;
            p->print(out);
        }
    }
    return out.str();
}



void Id3V2StreamBase::checkDuplicates(NoteColl& notes) const
{
    // for some it's OK to be duplicated, e.g. for APIC and various "Picture type" pictures;
    const set<string>& sKnownFrames (KnownFrames::getKnownFrames());
    set<pair<string, int> > sUsedFrames;
    int nImgCnt (0);
    streampos secondImgPos (-1);

    for (int i = 0, n = cSize(m_vpFrames); i < n; ++i)
    {
        Id3V2Frame* p = m_vpFrames[i];
//if (0 == strcmp("TDOR", p->m_szName)) { MP3_NOTE (p->m_pos, "TDOR found. See if it should be processed."); } //ttt remove
//if (0 == strcmp("TDRC", p->m_szName)) { MP3_NOTE (p->m_pos, "TDRC found. See if it should be processed."); }
//if (0 == strcmp("TDRL", p->m_szName)) { MP3_NOTE (p->m_pos, "TDRL found. See if it should be processed."); }
        if (sKnownFrames.count(p->m_szName) > 0)
        {
            if (0 == strcmp(KnownFrames::LBL_IMAGE(), p->m_szName))
            {
                ++nImgCnt;
                if (2 == nImgCnt)
                {
                    secondImgPos = p->m_pos;
                }
            }
            if (sUsedFrames.count(make_pair(p->m_szName, p->m_nPictureType)) == 0)
            {
                sUsedFrames.insert(make_pair(p->m_szName, p->m_nPictureType));
            }
            else
            {
                if (0 == strcmp(KnownFrames::LBL_IMAGE(), p->m_szName))
                {
                    MP3_NOTE (p->m_pos, id3v2DuplicatePic);
                }
                else if (0 == strcmp(KnownFrames::LBL_RATING(), p->m_szName))
                {
                    MP3_NOTE (p->m_pos, id3v2DuplicatePopm);
                }
                else
                {
                    MP3_NOTE_D (p->m_pos, id3v2MultipleFramesWithSameName, Notes::id3v2MultipleFramesWithSameName().getDescription() + string(" (Frame:") + p->m_szName + ")"); //ttt2 m_pos should be replaced with the position of the second frame with this ID
                }
            }
        }
    }

    if (nImgCnt > 1)
    {
        MP3_NOTE (secondImgPos, id3v2MultipleApic);
    }
}


TagTimestamp Id3V2StreamBase::get230TrackTime(bool* pbFrameExists) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_TIME_YEAR_230()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return TagTimestamp(""); }
    string strYear (p->getUtf8String());
    if (4 != cSize(strYear)) { return TagTimestamp(""); }

    p = findFrame(KnownFrames::LBL_TIME_DATE_230());
    try
    {
        if (0 == p) { return TagTimestamp(strYear); }
        string strDate (p->getUtf8String());
        if (4 != cSize(strDate)) { return TagTimestamp(strYear); }
        return TagTimestamp(strYear + "-" + strDate.substr(2, 2) + "-" + strDate.substr(0, 2));
    }

    catch (const TagTimestamp::InvalidTime&)
    {
        return TagTimestamp("");
    }
}


vector<const Id3V2Frame*> Id3V2StreamBase::getKnownFrames() const // to be used by Id3V2Cleaner;
{
    vector<const Id3V2Frame*> v;

    for (int i = 0; i < cSize(m_vpFrames); ++i)
    {
        const Id3V2Frame* p (m_vpFrames[i]);

        if (KnownFrames::getKnownFrames().count(p->m_szName) > 0)
        {
            bool bAdd (true);
            for (int j = 0; j < cSize(v); ++j)
            {
                if (p->m_nPictureType == v[j]->m_nPictureType && 0 == strcmp(v[j]->m_szName, p->m_szName))
                {
                    bAdd = false;
                    break;
                }
            }

            if (bAdd)
            {
                v.push_back(p);
            }
        }
    }

    return v;
}


//============================================================================================================
//============================================================================================================
//============================================================================================================



// explicit instantiation
//template class Id3V2Stream<Id3V230Frame>;
//template class Id3V2Stream<Id3V240Frame>;



//============================================================================================================
//============================================================================================================
//============================================================================================================



/*static*/ const char* KnownFrames::getFrameName (int n)
{
    switch (n)
    {
    case 0: return LBL_TITLE();
    case 1: return LBL_ARTIST();
    case 2: return LBL_TRACK_NUMBER();
    case 3: return LBL_TIME_YEAR_230();
    case 4: return LBL_TIME_DATE_230();
    case 5: return LBL_TIME_240();
    case 6: return LBL_GENRE();
    case 7: return LBL_IMAGE();
    case 8: return LBL_ALBUM();
    case 9: return LBL_RATING();
    case 10: return LBL_COMPOSER();
    }

    CB_THROW1 (InvalidIndex());
}


/*static*/ bool KnownFrames::canHaveDuplicates(const char* szName)
{ //ttt2 make this more sophisticated; maybe allow multiple LBL_RATING, each with its own email;
    //if (0 == strcmp(szName, LBL_IMAGE())) { return true; }

    if (1 == getKnownFrames().count(szName)) { return false; } // !!! OK for LBL_IMAGE, because when actually using this the image type should be compared as well

    return true;
}

