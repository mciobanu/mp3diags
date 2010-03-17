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


#include  "Id3V240Stream.h"

#include  "Helpers.h"


using namespace std;
using namespace pearl;


//============================================================================================================
//============================================================================================================
//============================================================================================================

// !!! Note that the exceptions thrown by Id3V240Frame::Id3V240Frame() pass through Id3V240Stream, so they should have a type suited for this situation.
// Also note that by the time Id3V240Frame::Id3V240Frame() gets executed, it was already decided that we deal with an Id3V240Stream. Now we should see if it is valid or broken.



// since broken applications may use all 8 bits for size, although only 7 should be used, this tries to figure out if the size is correct
bool Id3V240Frame::checkSize(istream& in, streampos posNext)
{
    streampos pos (m_pos);
    pos += m_nDiskDataSize + m_nDiskHdrSize;
    if (pos > posNext) { return false; }
    if (pos == posNext) { return true; }

    in.seekg(pos);
    char bfr [ID3_FRAME_HDR_SIZE];
    int nHdrBytesSkipped (0);
    int nRead (readID3V2(m_bHasUnsynch, in, bfr, ID3_FRAME_HDR_SIZE, posNext, nHdrBytesSkipped));
    in.clear();
    if (nRead < ID3_FRAME_HDR_SIZE)
    { // should be padding, which is always 0
        for (int i = 0; i < nRead; ++i)
        {
            if (0 != bfr[i]) { return false; }
        }
        return true;
    }

    // check for frames: uppercase letters and digits
    for (int i = 0; i < 4; ++i)
    {
        char c (bfr[i]);
        if ((c < 'A' || c > 'Z') && (c < '0' || c > '9')) //ttt3 ASCII-specific
        {
            if (0 != c) { return false; }
            goto tryPadding;
        }
    }
    return true; // another frame seems to follow

tryPadding:
    in.seekg(pos);
    int nSize (posNext - pos);
    char* a (new char[nSize]);

    ArrayPtrRelease<char> rel (a);

    nRead = readID3V2(m_bHasUnsynch, in, a, nSize, posNext, nHdrBytesSkipped);
    in.clear();
    if (nRead != nSize) { return false; }
    for (int i = 0; i < nRead; ++i)
    {
        if (0 != a[i]) { return false; }
    }
    return true;
}


/*override*/ int Id3V240Frame::getOffset() const
{
    return 0 != (m_cFlag2 & 0x01) ? 4 : 0; // not quite OK if the flag is set but the frame is only several bytes, but that was invalid to begin with; anyway, it doesn't mess up loading from disk, since none is needed for such short frames
}


void Id3V240Frame::load(NoteColl& notes, istream& in, streampos posNext, bool bHasUnsynch)
{
    StreamStateRestorer rst (in);
    if (m_nDiskDataSize < 0) { return; }
    if (m_nDiskDataSize > 5000000) { return; } // there are probably no frames over 5MB

    vector<char> v (m_nDiskDataSize);

    m_vcData.clear();
    //int nContentBytesSkipped (0);

    int nRead (read(in, &v[0], m_nDiskDataSize));

    //readID3V2(bHasUnsynch, in, &v[0], m_nMemDataSize1 w, posNext, nContentBytesSkipped));
    //m_nDiskDataSize1 = q m_nMemDataSize1 + nContentBytesSkipped;
    if (m_nDiskDataSize != nRead || !checkSize(in, posNext))
    {
        return;
    }

    rst.setOk();
    if (bHasUnsynch && !v.empty())
    {
        for (int i = 0; i < cSize(v); ++i)
        {
            m_vcData.push_back(v[i]);
            if (char(0xff) == v[i] && i < cSize(v) - 1 && 0 == v[i + 1]) // !!! it's OK to run this on the Data length indicator, because it doesn't contain any 0xff
            {
                ++i;
            }
        }
    }
    else
    {
        v.swap(m_vcData);
    }

    m_nMemDataSize = cSize(m_vcData);

    if (getOffset() > 0 && m_nMemDataSize > 3)
    { // Data length indicator
        unsigned char* p (reinterpret_cast<unsigned char*> (&m_vcData[0]));
        //inspect(p, 50);

        int nDli ((p[0] << 21) + (p[1] << 14) + (p[2] << 7) + (p[3] << 0));

        if (nDli != m_nMemDataSize - 4)
        {
            MP3_NOTE (m_pos, id3v240IncorrectDli); //ttt2 probably adjust m_nMemDataSize anyway; after all, the Data length indicator flag is still there
        }
        else
        {
            m_nMemDataSize -= 4;
            m_vcData.erase(m_vcData.begin(), m_vcData.begin() + 4);
        }
    }
}

void Id3V240Frame::load(NoteColl& notes, istream& in, streampos posNext)
{
    load(notes, in, posNext, m_bHasUnsynch);
    if (-1 == m_nMemDataSize)
    {
        load(notes, in, posNext, !m_bHasUnsynch);
        if (-1 != m_nMemDataSize)
        {
            m_bHasUnsynch = !m_bHasUnsynch;
            MP3_NOTE (m_pos, id3v240IncorrectFrameSynch);
        }
    }
}



Id3V240Frame::Id3V240Frame(NoteColl& notes, istream& in, streampos pos, bool bHasUnsynch, streampos posNext, StringWrp* pFileName) : Id3V2Frame(pos, bHasUnsynch, pFileName)
{
    in.seekg(pos);
    char bfr [ID3_FRAME_HDR_SIZE];
    int nHdrBytesSkipped (0);
    int nRead (readID3V2(bHasUnsynch/*false*/, in, bfr, ID3_FRAME_HDR_SIZE, posNext, nHdrBytesSkipped));

    MP3_CHECK (ID3_FRAME_HDR_SIZE == nRead || (nRead >= 1 && 0 == bfr[0]), pos, id3v2FrameTooShort, StreamIsBroken(Id3V240Stream::getClassDisplayName(), "Truncated ID3V2.4.0 tag.")); // in 2.4.0 bHasUnsynch shouldn't matter, because the frame header doesn't need unsynch; but unconforming apps use 8 bits for size, as opposed to 7, as they are supposed to
    unsigned char* p (reinterpret_cast<unsigned char*> (bfr));
    if (0 == bfr[0])
    { // padding
        //m_nMemDataSize = -1; // !!! not needed; the constructor makes it -1
        //m_nDiskDataSize = -1; // !!! not needed; the constructor makes it -1
        m_szName[0] = 0;
        return;
    }
    m_nDiskHdrSize = ID3_FRAME_HDR_SIZE + nHdrBytesSkipped;
//inspect(bfr, ID3_FRAME_HDR_SIZE);
    strncpy(m_szName, bfr, 4);
    m_szName[4] = 0;

    {
        char c (m_szName[0]);
        MP3_CHECK (c >= 'A' && c <= 'Z', pos, id3v2InvalidName, StreamIsBroken(Id3V240Stream::getClassDisplayName(), "ID3V2.4.0 tag containing a frame with an invalid name: " + getReadableName() + "."));
        for (int i = 1; i < 4; ++i)
        {
            char c (m_szName[i]);
            MP3_CHECK ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'), pos, id3v2InvalidName, StreamIsBroken(Id3V240Stream::getClassDisplayName(), "ID3V2.4.0 tag containing a frame with an invalid name: " + getReadableName() + "."));  //ttt3 ASCII-specific
        }
    }


    m_cFlag1 = bfr[8];
    m_cFlag2 = bfr[9];
//inspect(bfr);

    MP3_CHECK (0 == (m_cFlag1 & ~0x60), pos, id3v2UnsuppFlags1, StreamIsUnsupported(Id3V240Stream::getClassDisplayName(), "ID3V2.4.0 tag containing a frame with an unsupported flag.")); // ignores "Tag alter preservation" and "File alter preservation" // ttt2 use them
    MP3_CHECK (0 == (m_cFlag2 & ~0x03), pos, id3v2UnsuppFlags2, StreamIsUnsupported(Id3V240Stream::getClassDisplayName(), "ID3V2.4.0 tag containing a frame with an unsupported flag."));

    m_bHasUnsynch = (0 != (m_cFlag2 & ~0x02));
//m_bHasUnsynch = false;


    if (0 != (m_cFlag1 & 0x8f)) { MP3_NOTE (pos, id3v2IncorrectFlg1); }
    if (0 != (m_cFlag2 & 0xb0)) { MP3_NOTE (pos, id3v2IncorrectFlg2); }


    if (0 == (p[4] & 0x80) && 0 == (p[5] & 0x80) && 0 == (p[6] & 0x80) && 0 == (p[7] & 0x80))
    { // by the specs it should be 7 bit unsynch
        m_nDiskDataSize = (p[4] << 21) + (p[5] << 14) + (p[6] << 7) + (p[7] << 0);
        load(notes, in, posNext);
    }

    if (-1 == m_nMemDataSize)
    { // failed to load as 7bit unsynch, so try 8bit
        m_nDiskDataSize = (p[4] << 24) + (p[5] << 16) + (p[6] << 8) + (p[7] << 0);
        load(notes, in, posNext);

        if (-1 == m_nMemDataSize)
        {
            MP3_THROW (pos, id3v240CantReadFrame, StreamIsBroken(Id3V240Stream::getClassDisplayName(), "Broken ID3V2.4.0 tag."));
        }
        else
        {
            MP3_NOTE (pos, id3v240IncorrectSynch);
        }
    }

    try
    {
        try
        {
            getUtf8String();

            if ('T' == m_szName[0])
            {
                if (!isTxxx() && 0 == m_nMemDataSize)
                { // this is really invalid; text frames must have at least a byte; however, by doing these we make sure that an empty (i.e. having a single byte, for text encoding) frame gets copied if the tag is edited;
                    m_vcData.clear();
                    m_vcData.push_back(0);
                    m_nMemDataSize = cSize(m_vcData);
                    MP3_NOTE_D (pos, id3v2EmptyTextFrame, Notes::id3v2EmptyTextFrame().getDescription() + string(" (Frame:") + m_szName + ")");
                }
                else if (isTxxx() && m_nMemDataSize <= 1)
                { // this is really invalid; text frames must have at least a byte; however, by doing these we make sure that an empty (i.e. having a single byte, for text encoding) frame gets copied if the tag is edited;
                    m_vcData.clear();
                    static const char* szInvalid ("INVALID");
                    m_vcData.push_back(0); // latin1
                    m_vcData.insert(m_vcData.end(), szInvalid, szInvalid + strlen(szInvalid)); // description
                    m_vcData.push_back(0); // terminator
                    m_vcData.insert(m_vcData.end(), szInvalid, szInvalid + strlen(szInvalid)); // value
                    m_nMemDataSize = cSize(m_vcData);
                    MP3_NOTE_D (pos, id3v2EmptyTextFrame, Notes::id3v2EmptyTextFrame().getDescription() + string(" (Frame:") + m_szName + ")"); // ttt2 actually TXXX is not text
                }
                //ttt2 add check for terminating 0 for TXXX, to split the value into descr and val
            }
        }
        catch (const NotId3V2Frame&)
        {
            MP3_THROW (pos, id3v2TextError, StreamIsBroken(Id3V240Stream::getClassDisplayName(), "ID3V2.4.0 tag containing a broken text frame named " + getReadableName() + "."));
        }
        catch (const UnsupportedId3V2Frame&)
        {
            MP3_THROW (pos, id3v240UnsuppText, StreamIsUnsupported(Id3V240Stream::getClassDisplayName(), "ID3V2.4.0 tag containing a text frame named " + getReadableName() + " using unsupported characters or unsupported text encoding.")); //ttt2 not specific enough; this might need reviewing in the future
        }
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...)
    {
        throw;
    }

    if (m_nMemDataSize > 150) // ttt2 perhaps make configurable
    { // if the frame needs a lot of space, erase the data from memory; it will be retrieved from the disk when needed;
        vector<char>().swap(m_vcData);
    }
}


// may return multiple null characters; it's the job of getUtf8String() to deal with them;
// chars after the first null are considered comments (or after the second null, for TXXX), so the nulls are replaced with commas, except for those at the end of the string (which are removed) and the first null in TXXX;
string Id3V240Frame::getUtf8StringImpl() const
{
    if ('T' != m_szName[0])
    {
        return "<non-text value>";
    }

    // have a "pos" to pass here and ability to log; could also be used for large tags, to not store them in memory yet have them available
    // also use pos below
    // or perhaps forget these and throw exceptions that have error messages, catch them and log/show dialogs
    // 2008.07.12 - actually all these proposals don't seem to work well: the callers don't catch the exceptions thrown here and wouldn't know what to do with them; there's no good place to display the errors (the end user isn't supposed to look at logs); it seems better to not throw, but return a string describing the problem;
    // 2008.07.12 - on a second thought - throw but call this on the constructor, where it can be logged properly; if it worked on the constructor it should work later too
    CB_CHECK1 (m_nMemDataSize > 0, NotId3V2Frame());
    Id3V2FrameDataLoader wrp (*this);
    const char* pData (wrp.getData()); //ttt2 from http://www.id3.org/id3v2.4.0-frames - All text information frames supports multiple strings, stored as a null separated list

    string s;

    if (0 == pData[0])
    { // Latin-1

        for (int i = 1; i < m_nMemDataSize; ++i)
        {
            unsigned char c (pData[i]);
            if (c < 128)
            { // !!! 0 is OK
                s += char(c);
            }
            else
            {
                m_bHasLatin1NonAscii = true;
                unsigned char c1 (0xc0 | (c >> 6));
                unsigned char c2 (0x80 | (c & 0x3f));
                s += char(c1);
                s += char(c2);
            }
        }
    }
    else if (1 == pData[0])
    {
        s = utf8FromBomUtf16(pData + 1, m_nMemDataSize - 1);
    }
    else if (3 == pData[0])
    {
        s = string(pData + 1, m_nMemDataSize - 1);
    }
    else
    {
        if (2 == pData[0])
        {
            CB_THROW1 (UnsupportedId3V2Frame()); //ttt2 add support for UTF-16BE (2 = "UTF-16BE [UTF-16] encoded Unicode [UNICODE] without BOM");
        }

        CB_THROW1 (NotId3V2Frame());
    }

    { // deal with nulls
        int k (0);
        if (isTxxx())
        {
            for (; k < cSize(s) && 0 != s[k]; ++k) {}
            ++k;
        }

        int i (cSize(s) - 1);

        for (; i >= k && 0 == s[i]; --i)
        {
            s.erase(i);
        }

        for (; i >= k; --i)
        {
            if (0 == s[i])
            {
                //s.replace(i, 1, i > k ? ", " : "");
                s.replace(i, 1, ", ");
            }
        }
    }

    return s;
}



/*override*/ bool Id3V240Frame::discardOnChange() const
{
    return 0 != (m_cFlag1 & 0x60);
}





//============================================================================================================
//============================================================================================================
//============================================================================================================




Id3V240Stream::Id3V240Stream(int nIndex, NoteColl& notes, istream& in, StringWrp* pFileName, bool bAcceptBroken /*= false*/) : Id3V2StreamBase(nIndex, in, pFileName)
{
    StreamStateRestorer rst (in);

    streampos pos (m_pos);
    char bfr [ID3_HDR_SIZE];
    MP3_CHECK_T (ID3_HDR_SIZE == read(in, bfr, ID3_HDR_SIZE), pos, "Invalid ID3V2.4.0 tag. File too small.", NotId3V2());
    MP3_CHECK_T ('I' == bfr[0] && 'D' == bfr[1] && '3' == bfr[2], pos, "Invalid ID3V2.4.0 tag. Invalid ID3V2 header.", NotId3V2());
    MP3_CHECK_T (4 == bfr[3] && 0 == bfr[4], pos, "Invalid ID3V2.4.0 tag. Invalid ID3V2.4.0 header.", NotId3V2());
    m_nTotalSize = getId3V2Size (bfr);
    m_cFlags = bfr[5];
    MP3_CHECK (0 == (m_cFlags & 0x7f), pos, id3v2UnsuppFlag, StreamIsUnsupported(Id3V240Stream::getClassDisplayName(), "ID3V2 tag with unsupported flag.")); //ttt2 review, support

    streampos posNext (pos);
    posNext += m_nTotalSize;
    pos += ID3_HDR_SIZE;
    bool bHasLatin1NonAscii (false); // if it has a text frame that uses Latin1 encoding and has chars between 128 and 255
    try
    {
        for (;;)
        {
            long long nDiff (pos - posNext);
            if (nDiff >= 0) { break; }
            Id3V240Frame* p (new Id3V240Frame(notes, in, pos, hasUnsynch(), posNext, m_pFileName));
            bHasLatin1NonAscii = bHasLatin1NonAscii || p->m_bHasLatin1NonAscii;
            if (-1 == p->m_nMemDataSize)
            { // it encountered zeroes, which signals the beginning of padding //ttt2 should check that there's no garbage after the first zero
                m_nPaddingSize = posNext - pos;
                delete p;
                break;
            }
            m_vpFrames.push_back(p);
            pos += p->m_nDiskHdrSize + p->m_nDiskDataSize;
        }
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...)
    {
        if (bAcceptBroken)
        {
            return;
        }

        clearPtrContainer(m_vpFrames);
        throw;
    }

    checkDuplicates(notes);
    checkFrames(notes);

    if (bHasLatin1NonAscii)
    {
        MP3_NOTE (m_pos, id3v2HasLatin1NonAscii);
    }

    if (0 != findFrame(KnownFrames::LBL_TIME_YEAR_230()))
    {
        if (0 != findFrame(KnownFrames::LBL_TIME_240()))
        {
            MP3_NOTE (m_pos, id3v240DeprTyerAndTdrc); // ttt2 check consistency among TYER and TDRC
        }
        else
        {
            MP3_NOTE (m_pos, id3v240DeprTyer);
        }
    }

    if (0 != findFrame(KnownFrames::LBL_TIME_DATE_230()))
    {
        if (0 != findFrame(KnownFrames::LBL_TIME_240()))
        {
            MP3_NOTE (m_pos, id3v240DeprTdatAndTdrc); // ttt2 check consistency among TDAT and TDRC
        }
        else
        {
            MP3_NOTE (m_pos, id3v240DeprTdat);
        }
    }


    preparePicture(notes);
    switch (m_eImageStatus)
    {
    case ImageInfo::NO_PICTURE_FOUND: MP3_NOTE (m_pos, id3v2NoApic); break;
    case ImageInfo::ERROR_LOADING: MP3_NOTE (m_pos, id3v2CouldntLoadPic); break;
    //case ImageInfo::USES_LINK: MP3_NOTE (m_pos, id3v2LinkNotSupported); break; // !!! already reported by id3v2LinkInApic, so no need for another note;
    case ImageInfo::LOADED_NOT_COVER: MP3_NOTE (m_pos, id3v2NotCoverPicture); break;
    default:;
    }

    if (m_vpFrames.empty())
    {
        MP3_NOTE (m_pos, id3v2EmptyTag);
    }

    MP3_TRACE (m_pos, "Id3V240Stream built.");

    {
        pos = m_pos;
        pos += m_nTotalSize - 1;
        in.seekg(pos);
        char c;

        MP3_CHECK (1 == read(in, &c, 1), m_pos, id3v240CantReadFrame, NotId3V2());
    }

    rst.setOk();
}




/*override*/ TagReader::SuportLevel Id3V240Stream::getSupport(Feature eFeature) const
{
    switch (eFeature)
    {
    case TITLE:
    case ARTIST:
    case TRACK_NUMBER:
    case TIME:
    case GENRE:
    case IMAGE:
    case ALBUM:
    case RATING:
    case COMPOSER:
    case VARIOUS_ARTISTS:
        return READ_ONLY;

    default:
        return NOT_SUPPORTED;
    }
/*{ , , , , , , ,  };
    enum SuportLevel { NOT_SUPPORTED, , READ_WRITE };*/
}


/*override*/ TagTimestamp Id3V240Stream::getTime(bool* pbFrameExists /*= 0*/) const
{
    //const Id3V240Frame* p (findFrame(KnownFrames::LBL_TIME_YEAR_230()));
    const Id3V2Frame* p (findFrame(KnownFrames::LBL_TIME_240()));
    if (0 != p)
    {
        try
        {
            TagTimestamp t (p->getUtf8String());
            if (0 != pbFrameExists)
            {
                *pbFrameExists = true;
            }
            return t;
        }
        catch (const TagTimestamp::InvalidTime&)
        { // !!! nothing
        }
    }

    return get230TrackTime(pbFrameExists);
}


/*

http://www.id3.org/id3v2.4.0-structure :

The timestamp fields are based on a subset of ISO 8601. When being as
   precise as possible the format of a time string is
   yyyy-MM-ddTHH:mm:ss (year, "-", month, "-", day, "T", hour (out of
   24), ":", minutes, ":", seconds), but the precision may be reduced by
   removing as many time indicators as wanted. Hence valid timestamps
   are
   yyyy, yyyy-MM, yyyy-MM-dd, yyyy-MM-ddTHH, yyyy-MM-ddTHH:mm and
   yyyy-MM-ddTHH:mm:ss. All time stamps are UTC. For durations, use
   the slash character as described in 8601, and for multiple non-
   contiguous dates, use multiple strings, if allowed by the frame
   definition.
*/



//============================================================================================================
//============================================================================================================
//============================================================================================================


