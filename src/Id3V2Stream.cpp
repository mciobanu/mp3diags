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
#include  "fstream_unicode.h"

#include  "Id3V2Stream.h"

#include  "MpegStream.h"
#include  "Helpers.h"
#include  "CommonData.h"
#include  "Widgets.h"  // for GlobalTranslHlp


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
    STRM_ASSERT (posNext >= posCrt); // not always right
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
        m_nDiskDataSize(-1),
        m_pos(pos),
        m_pFileName(pFileName),
        m_bHasUnsynch(bHasUnsynch),
        m_bHasLatin1NonAscii(false),
        m_eApicStatus(NO_APIC),
        m_nPictureType(-1),
        m_nImgOffset(-1),
        m_nImgSize(-1),
        m_eCompr(ImageInfo::INVALID),
        m_nWidth(-1),
        m_nHeight(-1)
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


static const char* findAfter(const char* p, unsigned char cEnc, const char* pLast)
{
//inspect(p, pLast - p);
    if (0 == p) { return 0; }
    switch (cEnc)
    {
    case 0:
    case 3:
        {
            while (p < pLast - 1 && 0 != *p) { ++p; }
            if (p + 1 < pLast) { return p + 1; }
            return 0;
        }

    case 1:
    case 2:
        {
            while (p < pLast - 2 && (0 != *p || 0 != *(p + 1)))
            {
                p += 2;
                //inspect(p, pLast - p);
            }
            if (p + 2 < pLast) { return p + 2; }
            return 0;
        }

    default:
        CB_ASSERT(false);
    }
}



void Id3V2Frame::print(ostream& out, bool bFullInfo) const
{
    out << m_szName;
    if ('T' == m_szName[0])
    {
        string s (getUtf8String());
        if (!bFullInfo && cSize(s) > 100)
        {
            s.erase(90);
            s += " ...";
        }

        out << "=\"" << s << "\""; //ttt2 probably specific to particular versions of Linux and GCC
//cout << " value=\"" << getUtf8String() << "\""; //ttt2 probably specific to particular versions of Linux and GCC
        //out << " value=\"" << "RRRRRRRR" << "\"";
    }
    else if (bFullInfo && string("USLT") == m_szName)
    {
        Id3V2FrameDataLoader wrp (*this);
        const char* pData (wrp.getData());
        //const char* q (pData + 1);
        out << ": ";
        int nBeg (1);

        { // skip description
            if (0 == pData[0] || 3 == pData[0])
            {
                for (; nBeg < m_nMemDataSize && 0 != pData[nBeg]; ++nBeg) {}
                ++nBeg;
            }
            else
            {
                for (; nBeg < m_nMemDataSize - 1 && (0 != pData[nBeg] || 0 != pData[nBeg + 1]); ++nBeg) {}
                nBeg += 2;
            }
        }

        QString qs;
        switch (pData[0])
        {
        case 0: // Latin-1
            qs = QString::fromLatin1(pData + nBeg, m_nMemDataSize - nBeg);
            break;

        case 1:
            try
            {
                qs = QString::fromUtf8(utf8FromBomUtf16(pData + nBeg, m_nMemDataSize - nBeg).c_str());
            }
            catch (const NotId3V2Frame&)
            {
                qs = tr("<< error decoding string >>");
            }
            break;

        case 2:
            qs = tr("<< unsupported encoding >>");
            break;

        case 3:
            qs = QString::fromUtf8(pData + nBeg, m_nMemDataSize - nBeg); // ttt3 not quite OK for ID3V2.3.0, but it's probably better this way
            break;
        }

        //qs.replace('\n', " / "); qs.replace('\r', "");
        qs = "\n" + qs + "\n";

        out << qs.toUtf8().constData();
    }
    else
    {
        out << " " << convStr(tr("size=")) << m_nMemDataSize;
    }


    if (bFullInfo && string("GEOB") == m_szName)
    { // !!! "size" is already written
        //ttt2  perhaps try and guess the data type
        Id3V2FrameDataLoader wrp (*this);
        const char* pData (wrp.getData());
        unsigned char cEnc (*pData);
        const char* pMime (0);
        const char* pFile (0);
        const char* pDescr (0);
        const char* pBinData (0);

        QString qstrMime, qstrFile, qstrDescr;

        if (cEnc > 3)
        {
            out << " " << convStr(tr("invalid text encoding"));
        }
        else if (2 == cEnc)
        {
            out << " " << convStr(tr("unsupported text encoding"));
        }
        else
        {
            const char* pLast (pData + m_nMemDataSize); // actually first after last

            pMime = pData + 1;

            pFile = findAfter(pMime, 0, pLast); // !!! mime is always UTF-8
            pDescr = findAfter(pFile, cEnc, pLast);
            pBinData = findAfter(pDescr, cEnc, pLast);

            if (0 != pBinData)
            {
                qstrMime = QString::fromLatin1(pMime);
                switch (cEnc)
                {
                case 0: // Latin-1
                    qstrFile = QString::fromLatin1(pFile);
                    qstrDescr = QString::fromLatin1(pDescr);
                    break;

                case 1:
                    try
                    {
                        qstrFile = QString::fromUtf8(utf8FromBomUtf16(pFile, pDescr - pFile).c_str());
                    }
                    catch (const NotId3V2Frame&)
                    { // invalid encoding
                        qstrFile = tr("<encoding error>");
                    }

                    try
                    {
                        qstrDescr = QString::fromUtf8(utf8FromBomUtf16(pDescr, pBinData - pDescr).c_str());
                    }
                    catch (const NotId3V2Frame&)
                    { // invalid encoding
                        qstrDescr = tr("<encoding error>");
                    }
                    break;

                /*case 2:
                    qs = tr("<< unsupported encoding >>");
                    break;*/

                case 3:  // ttt3 not quite OK for ID3V2.3.0, but it's probably better this way
                    qstrFile = QString::fromUtf8(pFile);
                    qstrDescr = QString::fromUtf8(pDescr);
                    break;

                default: CB_ASSERT1 (false, m_pFileName->s);
                }
            }

            if (0 == pBinData)
            {
                out << " " << convStr(tr("invalid data"));
            }
            else
            {
                int nBinSize (pLast - pBinData);
                int nPrintedBinSize (min(1024, nBinSize));
                //out << " MIME=\"" << convStr(qstrMime) << "\" File=\"" << convStr(qstrFile) << "\" Descr=\"" << convStr(qstrDescr) << "\" Binary data size=" << pLast - pBinData << (nPrintedBinSize != nBinSize ? " Begins with: " : " Content: ") << asHex(pBinData, nPrintedBinSize);
                out << " " << convStr(tr("MIME=\"%1\" File=\"%2\" Descr=\"%3\" Binary data size=%4").arg(qstrMime).arg(qstrFile).arg(qstrDescr).arg(pLast - pBinData));
                out << " " << convStr((nPrintedBinSize != nBinSize ? tr("Begins with: %1") : tr("Content: %1")).arg(convStr(asHex(pBinData, nPrintedBinSize))));
            }
        }
    }


    if (Id3V2Frame::NO_APIC != m_eApicStatus)
    {
        out << " " << convStr(tr("status=%1").arg(tr(getImageStatus())));
    }

}


const char* Id3V2Frame::getImageType() const
{
    return ImageInfo::getImageType(m_nPictureType);
}


const char* Id3V2Frame::getImageStatus() const
{
    switch(m_eApicStatus)
    {
    case USES_LINK: return QT_TR_NOOP("link");
    case NON_COVER: return QT_TR_NOOP("non-cover");
    case ERR: return QT_TR_NOOP("error");
    case COVER: return QT_TR_NOOP("cover");
    default: CB_ASSERT1 (false, m_pFileName->s);
    }
}


double Id3V2Frame::getRating() const // asserts it's POPM
{
    CB_ASSERT (0 == strcmp(KnownFrames::LBL_RATING(), m_szName));

    Id3V2FrameDataLoader wrp (*this);
    const char* pData (wrp.getData());
    int k (0);
    while (k < m_nMemDataSize && 0 != pData[k]) { ++k; } // skip email addr
    ++k;
    if (k >= m_nMemDataSize)
    { // error //ttt2 add warning on constructor
        return -1;
    }
    unsigned char c (pData[k]);
    return 5*(double(c) - 1)/254;  // ttt2 not sure this is the best mapping
}



/*static*/ string Id3V2Frame::utf8FromBomUtf16(const char* pData, int nSize)
{
    CB_CHECK (nSize > 1, NotId3V2Frame); // UNICODE string entries must have a size of 3 or more."
    if (2 == nSize && 0 == *pData && 0 == *(pData + 1)) { return ""; } // not quite correct, but seems to happen; even if a string is null, it must begin with BOM //ttt1 add a note
    const unsigned char* p (reinterpret_cast<const unsigned char*> (pData));
    CB_CHECK ((0xff == p[0] && 0xfe == p[1]) || (0xff == p[1] && 0xfe == p[0]), NotId3V2Frame); //ttt2 perhaps use other exception

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

    return s;
}



// for display / export; for frames in KnownFrames::getKnownFrames only the data up to the first 0 is used (effectively removing comments in 2.3.0), while for the others, including TXXX, nulls and all other characters with codes below 32 are replaced with spaces (as a result, both description and value are shown for TXXX), so in either case the return value doesn't contain nulls;
// whitespaces at the end of the string are removed;
string Id3V2Frame::getUtf8String() const
{
    try
    {
        string s (getUtf8StringImpl());
        if (0 == KnownFrames::getKnownFrames().count(m_szName) || isTxxx())
        { // text frames may contain null characters, and what's after a null char isn't supposed to be displayed; however, for frames that aren't used we may want to see what's after the null
            // ttt2 null characters mean different things inside ID3V2.3.0 (comment follows) vs. ID3V2.4.0 (multiple values separated by 0); however, this doesn't matter in current implementation, because Id3V240Frame::getUtf8StringImpl() replaces nulls with commas
            for (int i = 0; i < cSize(s); ++i)
            {
                unsigned char c (s[i]);
                if (c < 32) // ttt3 ASCII only
                {
                    s[i] = 32;
                }
            }
        }

        s = s.c_str(); // !!! so now s doesn't contain null chars
        rtrim(s);

        return s;
    }
    catch (const Id3V2FrameDataLoader::LoadFailure&)
    {
        return convStr(TagReader::tr("<error loading frame>")); //ttt2 not sure if this is the best thing to do, but at least avoids crashes;
    }
    catch (const Id3V2Frame::NotId3V2Frame&)
    {
        return convStr(TagReader::tr("<error decoding frame>")); //ttt2 not sure if this is the best thing to do, but at least avoids crashes;
    }
}


// for internal processing; similar to getUtf8String() but doesn't replace internal null characters with spaces;
// removes trailing nulls and whitespaces, as well as 2.3.0 comments;
string Id3V2Frame::getRawUtf8String() const
{
    try
    {
        string s (getUtf8StringImpl());

        int i (cSize(s) - 1);
        for (; i >= 0; --i)
        {
            unsigned char c (s[i]);
            if (0 != c && (c >= ' ' || (c < ' ' && QChar(c).isSpace())))
            {
                break;
            }
        }

        ++i;
        if (i < cSize(s))
        {
            s.erase(i);
        }

        return s;
    }
    catch (const Id3V2FrameDataLoader::LoadFailure&)
    {
        return convStr(TagReader::tr("<error loading frame>")); //ttt2 not sure if this is the best thing to do, but at least avoids crashes;
    }
    catch (const Id3V2Frame::NotId3V2Frame&)
    {
        return convStr(TagReader::tr("<error decoding frame>")); //ttt2 not sure if this is the best thing to do, but at least avoids crashes;
    }
}


bool Id3V2Frame::isTxxx() const
{
    return 0 == strcmp(KnownFrames::LBL_TXXX(), m_szName);
}



//============================================================================================================
//============================================================================================================
//============================================================================================================


Id3V2FrameDataLoader::Id3V2FrameDataLoader(const Id3V2Frame& frame) : m_frame(frame)
{
    if (cSize(frame.m_vcData) < m_frame.m_nMemDataSize)
    {
        int nDiscard (frame.getOffset()); // for DataLengthIndicator
        //m_bOwnsData = true;
        CB_ASSERT1 (frame.m_vcData.empty(), m_frame.m_pFileName->s);
        CB_ASSERT1 (0 != frame.m_pFileName, m_frame.m_pFileName->s);

        m_vcOwnData.resize(m_frame.m_nMemDataSize);
        ifstream_utf8 in (m_frame.m_pFileName->s.c_str(), ios::binary);
        in.seekg(m_frame.m_pos);
        in.seekg(m_frame.m_nDiskHdrSize, ios_base::cur);
        streampos posNext (m_frame.m_pos);
        posNext += m_frame.m_nDiskDataSize + m_frame.m_nDiskHdrSize;
        int nContentBytesSkipped (0);
        int nRead (0);
        in.seekg(nDiscard, ios_base::cur);
        nRead = readID3V2(m_frame.m_bHasUnsynch, in, &m_vcOwnData[0], cSize(m_vcOwnData), posNext, nContentBytesSkipped);
        //qDebug("nRead %d ; m_frame.m_nMemDataSize %d ; nContentBytesSkipped %d ", nRead, m_frame.m_nMemDataSize, nContentBytesSkipped);
        if (cSize(m_vcOwnData) != nRead)
        {
            CB_THROW(LoadFailure);
        }

        m_pData = &m_vcOwnData[0];
    }
    else
    {
        if (frame.m_vcData.empty())
        {
            static char c (0);
            m_pData = &c;
        }
        else
        {
            m_pData = &frame.m_vcData[0];
        }
    }
}


Id3V2FrameDataLoader::~Id3V2FrameDataLoader()
{
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

/*static*/ const char* KnownFrames::LBL_WMP_VAR_ART() { return "TPE2"; }
/*static*/ const char* KnownFrames::LBL_ITUNES_VAR_ART() { return "TCMP"; }

/*static*/ const char* KnownFrames::LBL_TXXX() { return "TXXX"; }


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
        (*it)->print(out, Id3V2Frame::FULL_INFO);
//(*it)->print(cout);
    }
}



/*override*/ void Id3V2StreamBase::copy(std::istream& in, std::ostream& out)
{
    appendFilePart(in, out, m_pos, m_nTotalSize); //ttt2
}



/*override*/ std::string Id3V2StreamBase::getInfo() const
{
    ostringstream out;
    out << convStr(DataStream::tr("padding=")) << m_nPaddingSize << ", " << convStr(DataStream::tr("unsynch=")) << convStr(GlobalTranslHlp::tr(boolAsYesNo(hasUnsynch()))) << "; " << convStr(DataStream::tr("frames")) << ": ";
    bool bFirst (true);
    for (vector<Id3V2Frame*>::const_iterator it = m_vpFrames.begin(), end = m_vpFrames.end(); it != end; ++it)
    {
        if (!bFirst) { out << ", "; }
        bFirst = false;
        (*it)->print(out, Id3V2Frame::SHORT_INFO);
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





/*override*/ std::string Id3V2StreamBase::getTitle(bool* pbFrameExists /* = 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_TITLE()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}



/*override*/ std::string Id3V2StreamBase::getArtist(bool* pbFrameExists /* = 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_ARTIST()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}


/*override*/ std::string Id3V2StreamBase::getTrackNumber(bool* pbFrameExists /* = 0*/) const
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


/*override*/ std::string Id3V2StreamBase::getGenre(bool* pbFrameExists /* = 0*/) const
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
            MP3_NOTE (p->m_pos, id3v2EmptyTcon); //ttt2 perhaps replace this with something more generic
        }
    }

    //ttt2 add other checks
}
//ttt2 perhaps use links to pictures in crt dir


/*override*/ ImageInfo Id3V2StreamBase::getImage(bool* pbFrameExists /* = 0*/) const
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
        CB_ASSERT1 (0 == m_pPicFrame, m_pFileName->s);
        return ImageInfo(-1, m_eImageStatus);
    }

    CB_ASSERT1 (0 != m_pPicFrame, m_pFileName->s);
    try
    {
        Id3V2FrameDataLoader wrp (*m_pPicFrame);
        const char* pCrtData (wrp.getData());
        const char* pBinData (pCrtData + m_pPicFrame->m_nImgOffset);
        //CB_OLD_CHECK (pixmap.loadFromData(pBinData, m_nImgSize));

        // make sure the data is still available and correct (the file might have been modified externally)
        if (-1 == m_pPicFrame->m_nWidth)
        {
            QPixmap pic;
            if (!pic.loadFromData(reinterpret_cast<const unsigned char*>(pBinData), m_pPicFrame->m_nImgSize)) // this takes a lot of time
            {
                goto e1;
            }
            m_pPicFrame->m_nWidth = short(pic.width());
            m_pPicFrame->m_nHeight = short(pic.height());
        }

        QByteArray b (QByteArray::fromRawData(pBinData, m_pPicFrame->m_nImgSize));
        b.append('x'); b.resize(b.size() - 1); // !!! these are needed because fromRawData() doesn't create copies of the memory used for the byte array
        return ImageInfo(m_pPicFrame->m_nPictureType, m_eImageStatus, m_pPicFrame->m_eCompr, b, m_pPicFrame->m_nWidth, m_pPicFrame->m_nHeight);

        //QBuffer bfr (&res.m_compressedImg);
        //bfr.
        //res.m_compressedImg = QByteArray(fromRawData
        //delete pPictureInfo;
    }
    catch (const Id3V2FrameDataLoader::LoadFailure&)
    {
        //eImageStatus = ImageInfo::ERROR_LOADING;
    }
e1:
    trace("The picture could be loaded before but now this is no longer possible. The most likely reason is that the file was moved or changed by an external application."); //ttt0 is there a macro?

    return ImageInfo(-1, ImageInfo::ERROR_LOADING);
}


/*override*/ vector<ImageInfo> Id3V2StreamBase::getImages() const
{
    vector<ImageInfo> v;

    for (int i = 0; i < cSize(m_vpFrames); ++i)
    {
        Id3V2Frame* pFrame (m_vpFrames[i]);
        if (Id3V2Frame::NON_COVER == pFrame->m_eApicStatus || Id3V2Frame::COVER == pFrame->m_eApicStatus)
        {
            try
            {
                Id3V2FrameDataLoader wrp (*pFrame);
                const char* pCrtData (wrp.getData());
                const char* pBinData (pCrtData + pFrame->m_nImgOffset);
                //CB_OLD_CHECK (pixmap.loadFromData(pBinData, m_nImgSize));

                // make sure the data is still available and correct (the file might have been modified externally)
                if (-1 == pFrame->m_nWidth)
                {
                    QPixmap pic;
                    if (!pic.loadFromData(reinterpret_cast<const unsigned char*>(pBinData), pFrame->m_nImgSize)) // this takes a lot of time
                    {
                        v.push_back(ImageInfo(pFrame->m_nPictureType, ImageInfo::ERROR_LOADING));
                        continue;
                    }
                    pFrame->m_nWidth = short(pic.width());
                    pFrame->m_nHeight = short(pic.height());
                }

                QByteArray b (QByteArray::fromRawData(pBinData, pFrame->m_nImgSize));
                b.append('x'); b.resize(b.size() - 1); // !!! these are needed because fromRawData() doesn't create copies of the memory used for the byte array
                v.push_back(ImageInfo(pFrame->m_nPictureType, Id3V2Frame::COVER == pFrame->m_eApicStatus ? ImageInfo::OK : ImageInfo::LOADED_NOT_COVER, pFrame->m_eCompr, b, pFrame->m_nWidth, pFrame->m_nHeight));
            }
            catch (const Id3V2FrameDataLoader::LoadFailure&)
            {
                v.push_back(ImageInfo(pFrame->m_nPictureType, ImageInfo::ERROR_LOADING));
            }

            continue;
        }

        if (Id3V2Frame::ERR == pFrame->m_eApicStatus)
        {
            v.push_back(ImageInfo(pFrame->m_nPictureType, ImageInfo::ERROR_LOADING));
        }

        if (Id3V2Frame::USES_LINK == pFrame->m_eApicStatus)
        {
            v.push_back(ImageInfo(pFrame->m_nPictureType, ImageInfo::USES_LINK));
        }
    }

    return v;
}


/*override*/ std::string Id3V2StreamBase::getImageData(bool* pbFrameExists /* = 0*/) const
{
    if (0 != pbFrameExists)
    {
        *pbFrameExists = 0 != m_pPicFrame;
    }

    if (0 == m_pPicFrame) { return ""; }

    ostringstream out; // !!! don't translate, as this is used by XML export only //ttt1 review exporting as XML
    out << "type:" << m_pPicFrame->getImageType() << ", status:" << m_pPicFrame->getImageStatus() << ", size:" << m_pPicFrame->m_nWidth << "x" << m_pPicFrame->m_nHeight << ", compr:" << ImageInfo::getComprStr(m_pPicFrame->m_eCompr);

    return out.str();
}

/*static*/ const char* Id3V2StreamBase::decodeApic(NoteColl& notes, int nDataSize, streampos pos, const char* const pData, const char*& szMimeType, int& nPictureType, const char*& szDescription)
{
    MP3_CHECK (0 == pData[0] || 3 == pData[0], pos, id3v2UnsupApicTextEnc, CB_EXCP(NotSupTextEnc)); // !!! there's no need for StreamIsUnsupported here, because this error is not fatal, and it isn't allowed to propagate, therefore doesn't cause a stream to be Unsupported; //ttt2 review, support
    szMimeType = pData + 1; // ttt3 type 0 is Latin1, while type 3 is UTF8, so this isn't quite right; however, MIME types should probably be plain ASCII, so it's the same; and anyway, we only recognize JPEG and PNG, which are ASCII
    //int nMimeSize (strnlen(pData, nDataSize));

    const char* p (pData + 1);
    for (; p < pData + nDataSize && 0 != *p; ++p) {}
    MP3_CHECK (p < pData + nDataSize - 1, pos, id3v2UnsupApicTextEnc, CB_EXCP(ErrorDecodingApic)); // "-1" to account for szDescription
    CB_ASSERT (0 == *p);
    ++p;

    nPictureType = *p++;

    szDescription = p;
    for (; p < pData + nDataSize; ++p)
    {
        if (0 == *p)
        {
            return p + 1;
        }
    }

    CB_THROW(ErrorDecodingApic);
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
        pFrame->m_eApicStatus = pFrame->m_nPictureType == Id3V2Frame::PT_COVER ? Id3V2Frame::COVER : Id3V2Frame::NON_COVER;
        pFrame->m_nWidth = short(img.width());
        pFrame->m_nHeight = short(img.height());
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
        } //ttt2 perhaps support GIF or other formats; (well, GIFs can be loaded, but are recompressed when saving)
        return;
    }

    pFrame->m_eApicStatus = Id3V2Frame::ERR;

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
    //Id3V2Frame* pFirstLoadableCover (0);
    Id3V2Frame* pFirstLoadableNonCover (0);
    Id3V2Frame* pFirstApic (0); // might have errors, might be link, ...

    for (int i = 0, n = cSize(m_vpFrames); i < n; ++i)
    { // go through the frame list and set m_eApicStatus
        Id3V2Frame* p = m_vpFrames[i];
        if (0 == strcmp(KnownFrames::LBL_IMAGE(), p->m_szName))
        {
            if (0 == pFirstApic) { pFirstApic = p; } // !!! regardless of what exceptions might get thrown, p will have an m_eApicStatus other than NO_APIC; that happens either in preparePictureHlp() or with explicit assignments
            try
            {
                Id3V2FrameDataLoader wrp (*p);
                const char* pData (wrp.getData());
                const char* pCrtData (0);

                try
                {
                    pCrtData = decodeApic(notes, p->m_nMemDataSize, p->m_pos, pData, szMimeType, p->m_nPictureType, szDescription);
                }
                catch (const NotSupTextEnc&)
                {
                    p->m_eApicStatus = Id3V2Frame::ERR;
                    continue;
                }
                catch (const ErrorDecodingApic&)
                {
                    p->m_eApicStatus = Id3V2Frame::ERR;
                    continue;
                }

                if (0 != *szDescription) { MP3_NOTE (p->m_pos, id3v2PictDescrIgnored); }

                preparePictureHlp(notes, p, pData, pCrtData, szMimeType);

                if (Id3V2Frame::COVER == p->m_eApicStatus)
                {
                    if (0 == m_pPicFrame)
                    {
                        m_eImageStatus = ImageInfo::OK;
                        m_pPicFrame = p;
                    }
                }

                if (0 == pFirstLoadableNonCover && Id3V2Frame::NON_COVER == p->m_eApicStatus)
                {
                    pFirstLoadableNonCover = p;
                }
            }
            catch (const Id3V2FrameDataLoader::LoadFailure&)
            {
                MP3_NOTE (p->m_pos, fileWasChanged);
                p->m_eApicStatus = Id3V2Frame::ERR;
            }
        }
    }


    if (ImageInfo::OK == m_eImageStatus)
    {
        return;
    }

    // no cover frame was found; just pick the first loadable APIC frame and use it
    CB_ASSERT (ImageInfo::NO_PICTURE_FOUND == m_eImageStatus);

    if (0 != pFirstLoadableNonCover)
    {
        m_eImageStatus = ImageInfo::LOADED_NOT_COVER;
        m_pPicFrame = pFirstLoadableNonCover;
        return;
    }

    if (0 == pFirstApic)
    {
        return;
    }

    switch (pFirstApic->m_eApicStatus)
    {
    case Id3V2Frame::USES_LINK: m_eImageStatus = ImageInfo::USES_LINK; return;
    case Id3V2Frame::ERR: m_eImageStatus = ImageInfo::ERROR_LOADING; return;
    default: CB_ASSERT1 (false, m_pFileName->s); // all cases should have been covered
    }

}




/*override*/ std::string Id3V2StreamBase::getAlbumName(bool* pbFrameExists /* = 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_ALBUM()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}


/*override*/ std::string Id3V2StreamBase::getComposer(bool* pbFrameExists /* = 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_COMPOSER()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}


// *pbFrameExists gets set if at least one frame exists
/*override*/ int Id3V2StreamBase::getVariousArtists(bool* pbFrameExists /* = 0*/) const
{
    int nRes (0);
    if (0 != pbFrameExists) { *pbFrameExists = false; }

    try
    {
        {
            const Id3V2Frame* p (findFrame(KnownFrames::LBL_WMP_VAR_ART()));
            if (0 != p)
            {
                if (0 != pbFrameExists)
                {
                    *pbFrameExists = true;
                }
                if (0 == convStr(p->getUtf8String()).compare("VaRiOuS Artists", Qt::CaseInsensitive))
                {
                    nRes += TagReader::VA_WMP;
                }
            }
        }

        {
            const Id3V2Frame* p (findFrame(KnownFrames::LBL_ITUNES_VAR_ART()));
            if (0 != p)
            {
                if (0 != pbFrameExists)
                {
                    *pbFrameExists = true;
                }
                if ("1" == p->getUtf8String())
                {
                    nRes += TagReader::VA_ITUNES;
                }
            }
        }
    }
    catch (const Id3V2Frame::NotId3V2Frame&)
    { // !!! nothing
    }
    catch (const Id3V2Frame::UnsupportedId3V2Frame&)
    { // !!! nothing
    }

    return nRes;
}


/*override*/ double Id3V2StreamBase::getRating(bool* pbFrameExists /* = 0*/) const
{
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_RATING()));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return -1; }

    return p->getRating();
}


static const char* REPLAY_GAIN_ROOT ("replaygain");


//ttt1 mp3gain 1.5.1 can use ID3V2 instead of APE, so that should be tested as well; see TXXX="MP3GAIN_MINMAX[...]" in "Arctic Monkeys - 01 - My Propeller.mp3" (which also has TXXX="replaygain_track_gain[...]") - also review Id3V2Cleaner::processId3V2Stream
bool Id3V2StreamBase::hasReplayGain() const
{
    for (int i = 0; i < cSize(m_vpFrames); ++i)
    {
        const Id3V2Frame* pFrame (m_vpFrames[i]);
        if (pFrame->isTxxx())
        {
            QString qs (convStr(pFrame->getUtf8String()));

            if (qs.startsWith(REPLAY_GAIN_ROOT, Qt::CaseInsensitive))
            {
                return true;
            }
        }
    }

    return false;
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


/*static*/ const set<string>& KnownFrames::getExcludeFromInfoFrames() // frames that shouldn't be part of "other info"; doesn't include TXXX and "Various Artists" frames
{
    static bool bFirstTime (true);
    static set<string> s;
    if (bFirstTime)
    {
        s.insert(KnownFrames::LBL_TITLE());
        s.insert(KnownFrames::LBL_ARTIST());
        s.insert(KnownFrames::LBL_TRACK_NUMBER());
        s.insert(KnownFrames::LBL_TIME_YEAR_230());
        s.insert(KnownFrames::LBL_TIME_DATE_230());
        s.insert(KnownFrames::LBL_TIME_240()); //ttt2 perhaps this shouldn't be used for 2.3.0, but it covers cases like reading 2.4.0 and writing 2.3.0 or bugs by some tools
        s.insert(KnownFrames::LBL_GENRE());
        s.insert(KnownFrames::LBL_IMAGE());
        s.insert(KnownFrames::LBL_ALBUM());
        s.insert(KnownFrames::LBL_RATING());
        s.insert(KnownFrames::LBL_COMPOSER());

        bFirstTime = false;
    }

    return s;
}

// includes "Various Artists" frames; doesn't include TXXX
/*static*/ const set<string>& KnownFrames::getKnownFrames()
{
    static bool bFirstTime (true);
    static set<string> sKnownFrames (getExcludeFromInfoFrames());
    if (bFirstTime)
    {
        sKnownFrames.insert(KnownFrames::LBL_WMP_VAR_ART());
        sKnownFrames.insert(KnownFrames::LBL_ITUNES_VAR_ART());

        bFirstTime = false;
    }

    return sKnownFrames;
}



/*override*/ std::string Id3V2StreamBase::getOtherInfo() const
{
    set<string> sUsedFrames;

    //string strRes;
    ostringstream out;
    bool b (false);

    for (int i = 0, n = cSize(m_vpFrames); i < n; ++i)
    {
        Id3V2Frame* p = m_vpFrames[i];
        if (KnownFrames::getExcludeFromInfoFrames().count(p->m_szName) > 0 && sUsedFrames.count(p->m_szName) == 0)
        {
            sUsedFrames.insert(p->m_szName);
        }
        else
        {
            if (Id3V2Frame::NON_COVER != p->m_eApicStatus && Id3V2Frame::COVER != p->m_eApicStatus) // images that can be loaded are shown, so there shouldn't be another entry for them
            {
                if (b) { out << ", "; }
                b = true;
                p->print(out, Id3V2Frame::FULL_INFO);
            }
        }
    }
    return out.str();
}



void Id3V2StreamBase::checkDuplicates(NoteColl& notes) const
{
    // for some it's OK to be duplicated, e.g. for APIC and various "Picture type" pictures;

    set<pair<string, int> > sUsedFrames;
    int nImgCnt (0);
    streampos secondImgPos (-1);

    for (int i = 0, n = cSize(m_vpFrames); i < n; ++i)
    {
        Id3V2Frame* p = m_vpFrames[i];
//if (0 == strcmp("TDOR", p->m_szName)) { MP3_NOTE (p->m_pos, "TDOR found. See if it should be processed."); } //ttt remove
//if (0 == strcmp("TDRC", p->m_szName)) { MP3_NOTE (p->m_pos, "TDRC found. See if it should be processed."); }
//if (0 == strcmp("TDRL", p->m_szName)) { MP3_NOTE (p->m_pos, "TDRL found. See if it should be processed."); }
        if (KnownFrames::getKnownFrames().count(p->m_szName) > 0)
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
                    MP3_NOTE_D (p->m_pos, id3v2MultipleFramesWithSameName, tr("%1 (Frame: %2)").arg(noteTr(id3v2MultipleFramesWithSameName)).arg(p->m_szName)); //ttt2 m_pos should be replaced with the position of the second frame with this ID
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

        if (KnownFrames::getKnownFrames().count(p->m_szName) > 0 || p->isTxxx())
        {
            // add if known except if a picture with the same type was already added; don't add duplicates even if Id3V230StreamWriter could take care of them, because it keeps the last value and we want the first one
            bool bAdd (true);
            for (int j = 0; j < cSize(v); ++j)
            {
                if (p->m_nPictureType == v[j]->m_nPictureType && 0 == strcmp(v[j]->m_szName, p->m_szName))
                {
                    // !!! important for both image and non-image frames; while Id3V230StreamWriter would remove duplicates as configured, we want to get rid of them now, so the first value is kept; Id3V230StreamWriter removes old values as new ones are added, which would keep the last value
                    //ttt2 not always right for multiple pictures if "keep one" is checked;

                    bAdd = false;
                    break;
                }
            }

            if (p->isTxxx())
            {
                string s (p->getUtf8String());
                QString qs (convStr(s));

                if (qs.startsWith(REPLAY_GAIN_ROOT, Qt::CaseInsensitive))
                {
                    bAdd = true; //ttt2 perhaps check it's no duplicate; not sure about case
                }
            }

            if (bAdd && 0 == strcmp(KnownFrames::LBL_IMAGE(), p->m_szName) && Id3V2Frame::COVER != p->m_eApicStatus && Id3V2Frame::NON_COVER != p->m_eApicStatus)
            { // !!! get rid of broken pictures and links
                bAdd = false;
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

    CB_TRACE_AND_THROW (InvalidIndex);
}


/*static*/ bool KnownFrames::canHaveDuplicates(const char* szName)
{ //ttt2 make this more sophisticated; maybe allow multiple LBL_RATING, each with its own email;
    //if (0 == strcmp(szName, LBL_IMAGE())) { return true; }

    if (1 == getKnownFrames().count(szName)) { return false; } // !!! OK for LBL_IMAGE, because when actually using this the image type should be compared as well

    return true;
}

//ttt2 note for mismatch between file name and rating
