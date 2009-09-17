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

#include  "Id3V230Stream.h"

#include  "Id3V240Stream.h"
#include  "Helpers.h"
#include  "Id3Transf.h"



using namespace std;
using namespace pearl;


//============================================================================================================
//============================================================================================================
//============================================================================================================


// !!! Note that the exceptions thrown by Id3V230Frame::Id3V230Frame() pass through Id3V230Stream, so they should have a type suited for this situation.
// Also note that by the time Id3V230Frame::Id3V230Frame() gets executed, it was already decided that we deal with an Id3V230Stream. Now we should see if it is valid or broken.


Id3V230Frame::Id3V230Frame(NoteColl& notes, istream& in, streampos pos, bool bHasUnsynch, streampos posNext, StringWrp* pFileName) : Id3V2Frame(pos, bHasUnsynch, pFileName)
{
    in.seekg(pos);
    char bfr [ID3_FRAME_HDR_SIZE];
    int nHdrBytesSkipped (0);
    int nRead (readID3V2(bHasUnsynch, in, bfr, ID3_FRAME_HDR_SIZE, posNext, nHdrBytesSkipped));

    MP3_CHECK (ID3_FRAME_HDR_SIZE == nRead || (nRead >= 1 && 0 == bfr[0]), pos, id3v2FrameTooShort, StreamIsBroken(Id3V230Stream::getClassDisplayName(), "Truncated ID3V2.3.0 tag."));
    unsigned char* p (reinterpret_cast<unsigned char*> (bfr));
    if (0 == bfr[0])
    { // padding
        //m_nMemDataSize = -1; // !!! not needed; the constructor makes it -1
        m_nDiskDataSize = -1;
        m_szName[0] = 0;
        return;
    }
//inspect(bfr, ID3_FRAME_HDR_SIZE);
    strncpy(m_szName, bfr, 4);
    m_szName[4] = 0;
    m_nMemDataSize = (p[4] << 24) + (p[5] << 16) + (p[6] << 8) + (p[7] << 0);

    {
        char c (m_szName[0]);
        MP3_CHECK (c >= 'A' && c <= 'Z', pos, id3v2InvalidName, StreamIsBroken(Id3V230Stream::getClassDisplayName(), "ID3V2.3.0 tag containing a frame with an invalid name: " + getReadableName() + "."));
        for (int i = 1; i < 4; ++i)
        {
            char c (m_szName[i]);
            MP3_CHECK ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'), pos, id3v2InvalidName, StreamIsBroken(Id3V230Stream::getClassDisplayName(), "ID3V2.3.0 tag containing a frame with an invalid name: " + getReadableName() + "."));  //ttt3 ASCII-specific
        }
    }

    m_cFlag1 = bfr[8];
    m_cFlag2 = bfr[9];

    //MP3_CHECK (0 == (m_cFlag1 & ~(0x40 | 0x1f)), pos, "Invalid ID3V2.3.0 frame. Flags1 not supported.", Id3V230Stream::NotId3V2()); // !!! 0x1f is for flags that are supposed to be always 0 but aren't; the best way to deal with them seems to be to ignore them // 2008.08.25 - seems best to just ignore these flags ("Tag alter preservation", "File alter preservation" and "Read only") //ttt2 revisit this decision
    MP3_CHECK (0 == (m_cFlag2 & ~(0x00 | 0x1f)), pos, id3v2UnsuppFlags2, StreamIsUnsupported(Id3V230Stream::getClassDisplayName(), "ID3V2.3.0 tag containing a frame with an unsupported flag."));

    if (0 != (m_cFlag1 & 0x1f)) { MP3_NOTE (pos, id3v2IncorrectFlg1); }
    if (0 != (m_cFlag2 & 0x1f)) { MP3_NOTE (pos, id3v2IncorrectFlg2); }
    m_vcData.resize(m_nMemDataSize);
    int nContentBytesSkipped (0);
    nRead = 0;
    nRead = readID3V2(bHasUnsynch, in, &m_vcData[0], m_nMemDataSize, posNext, nContentBytesSkipped);
    if (m_nMemDataSize != nRead)
    {
        vector<char>().swap(m_vcData);;
        MP3_THROW (pos, id3v2FrameTooShort, StreamIsBroken(Id3V230Stream::getClassDisplayName(), "Truncated ID3V2.3.0 tag."));
    }

    m_nDiskHdrSize = ID3_FRAME_HDR_SIZE + nHdrBytesSkipped;
    m_nDiskDataSize = m_nMemDataSize + nContentBytesSkipped;

    try
    {
        try
        {
            getUtf8String();

            Id3V2FrameDataLoader wrp (*this);
            const char* pData (wrp.getData());
            if (3 == pData[0])
            {
                //MP3_NOTE (pos, id3v230UsesUtf8);
                MP3_NOTE_D (pos, id3v230UsesUtf8, Notes::id3v230UsesUtf8().getDescription() + string(" (Frame:") + m_szName + ")"); // perhaps drop the frame name if too many such notes get generated
            }
        }
        catch (const NotId3V2Frame&)
        {
            MP3_THROW (pos, id3v2TextError, StreamIsBroken(Id3V230Stream::getClassDisplayName(), "ID3V2.3.0 tag containing a broken text frame named " + getReadableName() + "."));
        }
        catch (const UnsupportedId3V2Frame&)
        {
            MP3_THROW (pos, id3v230UnsuppText, StreamIsUnsupported(Id3V230Stream::getClassDisplayName(), "ID3V2.3.0 tag containing a text frame named " + getReadableName() + " using unsupported characters.")); //ttt2 NotSupported is not specific enough; this might need reviewing in the future
        }
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...)
    {
        vector<char>().swap(m_vcData);;
        throw;
    }

    if (m_nMemDataSize > 150) // ttt2 perhaps make configurable
    { // if the frame needs a lot of space, erase the data from memory; it will be retrieved from the disk when needed;
        vector<char>().swap(m_vcData);
    }
}


// needed by Id3V230StreamWriter::addBinaryFrame(), so objects created with this constructor don't get serialized; destroys vcData by doing a swap for its own representation
Id3V230Frame::Id3V230Frame(const std::string& strName, vector<char>& vcData) : Id3V2Frame(0, Id3V2Frame::NO_UNSYNCH, 0)
{
    CB_CHECK1 (4 == cSize(strName), NotId3V2Frame());
    strcpy(m_szName, strName.c_str());

    m_nMemDataSize = cSize(vcData);
    m_nDiskDataSize = -1;
    m_nDiskHdrSize = -1;
    m_cFlag1 = 0;
    m_cFlag2 = 0;
    // m_pos = ...;
    //m_szFileName = 0;
    //m_bHasUnsynch = false;

    vcData.swap(m_vcData);
}



/*override*/ Id3V230Frame::~Id3V230Frame()
{
    //qDebug("Id3V230Frame destr %p", this);
    //qDebug("  Id3V230Frame destr %p", m_pData);
}



// assumes Latin1, converts to UTF8; returns "<non-text value>" for non-text strings; replaces '\0' with ' ' inside tags; //ttt2 see about implications; "Momma Cried" has a tag called TXXX whose value contains a '\0': "RATING\01";
// whitespaces at the end of the string are removed; not sure if this is how it should be, though
string Id3V230Frame::getUtf8String() const
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
    const char* pData (wrp.getData());
    if (0 == pData[0])
    { // Latin-1
        string s;
        for (int i = 1; i < m_nMemDataSize; ++i)
        {
            unsigned char c (pData[i]);
            if (0 == c)
            {
                s += ' ';
            }
            else if (c < 128)
            {
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
        rtrim(s);
        return s;
    }

    if (1 == pData[0])
    {
        return utf8FromBomUtf16(pData + 1, m_nMemDataSize - 1);
    }

    if (3 == pData[0]) // not valid, but used by some tools; a note will get generated in this case
    {
        string s (pData + 1, m_nMemDataSize - 1);
        if (!s.empty())
        {
            char c (s[s.size() - 1]);
            if (0 == c) // this string is supposed to be 0-terminated; silently remove the ending null, if it's there; // ttt2 perhaps have warning, but only if somebody actually cares
            {
                s.erase(s.size() - 1);
            }
        }
        return s;
    }


    // pData[0] has an invalid value
    CB_THROW1 (NotId3V2Frame());
}



/*override*/ bool Id3V230Frame::discardOnChange() const
{
    return 0 != (m_cFlag1 & 0xc0);
}


//============================================================================================================
//============================================================================================================
//============================================================================================================




Id3V230Stream::Id3V230Stream(int nIndex, NoteColl& notes, istream& in, StringWrp* pFileName, bool bAcceptBroken /*= false*/) : Id3V2StreamBase(nIndex, in, pFileName)
{
    StreamStateRestorer rst (in);

    streampos pos (m_pos);
    char bfr [ID3_HDR_SIZE];
    MP3_CHECK_T (ID3_HDR_SIZE == read(in, bfr, ID3_HDR_SIZE), pos, "Invalid ID3V2.3.0 tag. File too small.", NotId3V2());
    MP3_CHECK_T ('I' == bfr[0] && 'D' == bfr[1] && '3' == bfr[2], pos, "Invalid ID3V2.3.0 tag. Invalid ID3V2 header.", NotId3V2());
    MP3_CHECK ((3 == bfr[3] || 4 == bfr[3]) && 0 == bfr[4], pos, id3v2UnsuppVer, StreamIsUnsupported(Id3V2StreamBase::getClassDisplayName(), "Unsupported version of ID3V2 tag" + ((bfr[3] >= 0 && bfr[3] <=9 && bfr[4] >= 0 && bfr[4] <=9) ? string(": ID3V2.") + char('0' + bfr[3]) + '.'  + char('0' + bfr[4]) : "."))); // !!! tests for both 2.3.0 and 2.4.0 to make sure a SUPPORT message is generated (as opposed to TRACE); this test could be done either here or in ID3V240 (or in a separate function);
    MP3_CHECK_T (3 == bfr[3] && 0 == bfr[4], pos, "Invalid ID3V2.3.0 tag. Invalid ID3V2.3.0 header.", NotId3V2());
    m_nTotalSize = getId3V2Size (bfr);
    m_cFlags = bfr[5];
    MP3_CHECK (0 == (m_cFlags & 0x7f), pos, id3v2UnsuppFlag, StreamIsUnsupported(Id3V230Stream::getClassDisplayName(), "ID3V2 tag with unsupported flag.")); //ttt1 review, support

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
            Id3V230Frame* p (new Id3V230Frame(notes, in, pos, hasUnsynch(), posNext, m_pFileName));
            bHasLatin1NonAscii = p->m_bHasLatin1NonAscii || bHasLatin1NonAscii;
            if (-1 == p->m_nMemDataSize)
            { // it encountered zeroes, which signals the beginning of padding
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

    preparePicture(notes);
    checkDuplicates(notes);
    checkFrames(notes);

    if (bHasLatin1NonAscii)
    {
        MP3_NOTE (m_pos, id3v2HasLatin1NonAscii);
    }

    switch (m_eImageStatus)
    {
    case ImageInfo::NO_PICTURE_FOUND: MP3_NOTE (m_pos, id3v2NoApic); break;
    case ImageInfo::ERROR_LOADING: MP3_NOTE (m_pos, id3v2CouldntLoadPic); break;
    //case ImageInfo::USES_LINK: MP3_NOTE (m_pos, id3v2LinkNotSupported); break; // !!! already reported by id3v2LinkInApic, so no need for another note;
    case ImageInfo::LOADED_NOT_COVER: MP3_NOTE (m_pos, id3v2NotCoverPicture); break;
    default:;
    }

    if (m_nPaddingSize > Id3V230StreamWriter::DEFAULT_EXTRA_SPACE + 4096) //ttt2 hard-coded
    {
        MP3_NOTE (m_pos + (getSize() - m_nPaddingSize), id3v2PaddingTooLarge);
    }

    MP3_TRACE (m_pos, "Id3V230Stream built.");

    rst.setOk();
}


/*override*/ TagReader::SuportLevel Id3V230Stream::getSupport(Feature eFeature) const
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
        return READ_ONLY;

    default:
        return NOT_SUPPORTED;
    }
/*{ , , , , , , ,  };
    enum SuportLevel { NOT_SUPPORTED, , READ_WRITE };*/
}


/*override*/ TagTimestamp Id3V230Stream::getTime(bool* pbFrameExists /*= 0*/) const
{
    return get230TrackTime(pbFrameExists);
}



//============================================================================================================
//============================================================================================================
//============================================================================================================


Id3V230StreamWriter::~Id3V230StreamWriter()
{
    /*for (int i = 0; i < cSize(m_vpOwnFrames); ++i)
    {
        qDebug("remove %p", m_vpOwnFrames[i]);
        //qDebug("    remove %p", m_vpOwnFrames[i]->m_pData);
    }*/
    clearPtrContainer(m_vpOwnFrames);
}



Id3V230StreamWriter::Id3V230StreamWriter(bool bKeepOneValidImg, bool bFastSave, Id3V2StreamBase* p, const std::string& strDebugFileName) : m_bKeepOneValidImg(bKeepOneValidImg), m_bFastSave(bFastSave), m_strDebugFileName(strDebugFileName)
{
    if (0 != p)
    {
        if (m_bKeepOneValidImg)
        {
            const Id3V2Frame* pPic (0);

            for (int i = 0; i < cSize(p->getFrames()); ++i)
            {
                const Id3V2Frame* q (p->getFrames()[i]);
                CB_ASSERT1 ((0 == strcmp(q->m_szName, KnownFrames::LBL_IMAGE())) ^ (Id3V2Frame::NO_APIC == q->m_eApicStatus), m_strDebugFileName);
                if (Id3V2Frame::NO_APIC == q->m_eApicStatus)
                {
                    bool bCopyFrame (true);

                    if ('T' == q->m_szName[0] && q->m_nMemDataSize > 0 && 0 != dynamic_cast<const Id3V240Frame*>(q))
                    { // check for UTF-8 encoding
                        Id3V2FrameDataLoader ldr (*q);
                        const char* pData (ldr.getData());
                        if (3 == pData[0])
                        { // UTF-8
                            bCopyFrame = false;
                            addTextFrame(q->m_szName, q->getUtf8String());
                        }
                    }

                    if (bCopyFrame)
                    {
                        m_vpAllFrames.push_back(q);
                    }
                }
                else
                {
                    if ((0 == pPic && q->m_eApicStatus > Id3V2Frame::ERR) ||
                        (0 != pPic && pPic->m_eApicStatus < q->m_eApicStatus))
                    {
                        pPic = q;
                    }
                }
            }

            if (0 != pPic)
            {
                m_vpAllFrames.push_back(pPic);
            }
        }
        else
        {
            m_vpAllFrames.insert(m_vpAllFrames.end(), p->getFrames().begin(), p->getFrames().end());
        }

        TagTimestamp time;
        try
        {
            time = p->getTime();
        }
        catch (const TagTimestamp::InvalidTime&)
        {
        }
        setRecTime(time);
    }
}




void Id3V230StreamWriter::setRecTime(const TagTimestamp& time)
{
    removeFrames(KnownFrames::LBL_TIME_YEAR_230());
    removeFrames(KnownFrames::LBL_TIME_DATE_230());
    removeFrames(KnownFrames::LBL_TIME_240());

    if (0 == time.getYear()[0]) { return; }
    addTextFrame(KnownFrames::LBL_TIME_YEAR_230(), time.getYear());
    if (0 == time.getDayMonth()[0]) { return; }
    addTextFrame(KnownFrames::LBL_TIME_DATE_230(), time.getDayMonth());
}



// strVal is UTF8; the frame will use ASCII if possible and UTF16 otherwise (so if there's a char with a code above 127, UTF16 gets used, to avoid codepage issues for codes between 128 and 255); nothing is added if strVal is empty;
void Id3V230StreamWriter::addTextFrame(const std::string& strName, const std::string& strVal)
{
//http://www.id3.org/id3v2.3.0#head-1a37d4a15deafc294208ccfde950f77e47000bca
    bool bIsAscii (true);
    int n (cSize(strVal));
    for (int i = 0; i < n; ++i)
    {
        unsigned char c (strVal[i]);
        if (c >= 128)
        {
            bIsAscii = false;
            break;
        }
    }

    vector<char> vcData;
    int nSize (0);
    if (bIsAscii)
    {
        nSize = n + 1; // strings are not null-terminated, but there's the "text encoding" byte
        vcData.resize(nSize);
//qDebug("add %p", p);
        vcData[0] = 0;
        copy(strVal.c_str(), strVal.c_str() + nSize - 1, &vcData[0] + 1);
    }
    else
    {
        QString s (convStr(strVal));
        const ushort* pUtf16 (s.utf16()); //ttt3 assumes short is 16bit
        int nUtf16Size (0);
        while (0 != pUtf16[nUtf16Size++]) {} // there is probably some strlen equivalent, but wcslen isn't that, because it uses wchar_t instead of ushort, which on Unix is usually 4-byte
        --nUtf16Size;
        nSize = 2*nUtf16Size + 3; // "text encoding" byte, BOM, no null terminator
        vcData.resize(nSize);
        vcData[0] = 1;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        int nOrder (1); // x86
#else
        int nOrder (0);
#endif

        vcData[2 - nOrder] = char(0xff);
        vcData[1 + nOrder] = char(0xfe);

        const char* pcUtf16 (reinterpret_cast<const char*>(pUtf16));
        copy(pcUtf16, pcUtf16 + nSize - 3, &vcData[0] + 3);
    }

    //inspect(p, nSize);

    addBinaryFrame(strName, vcData);
}


// destroys vcData by doing a swap for its own representation; asserts that strName is not APIC
void Id3V230StreamWriter::addBinaryFrame(const std::string& strName, vector<char>& vcData)
{
    Id3V230Frame* p (new Id3V230Frame(strName, vcData));
    CB_ASSERT1 (0 != strcmp(KnownFrames::LBL_IMAGE(), p->m_szName), m_strDebugFileName);
    addNonOwnedFrame(p);
    m_vpOwnFrames.push_back(p);
}


//ttt2 perhaps something like this: if there's 1 "Other" pic and a "Cover" is added; based on config: 1) delete "Other"; 2) delete "cover"; 3) keep both; //ttt2 the option should be used to mark an "Other" pic as "Cover" in the tag editor, so it doesn't trigger saving.


// the image type is ignored; images are always added as cover;
// if there is an APIC frame with the same image, it is removed (it doesn't matter if it has different type, description ...); // ttt2 description should be counted too, if used
// if cover image already exists it is removed;
void Id3V230StreamWriter::addImg(std::vector<char>& vcData)
{
    int n (cSize(vcData));
    CB_ASSERT1 (n > 100, m_strDebugFileName);
    char* p (&vcData[0]);
    CB_ASSERT1 (0 == *p || 3 == *p, m_strDebugFileName); // text encoding // this should be kept in synch with Id3V2StreamBase::decodeApic() //ttt0 triggered according to mail; might have been caused by SmallerImageRemover::apply() incorrectly assuming that an invalid APIC frame is a large picture; the test using the frame name was replaced after the assert with a test using m_eApicStatus; will have to wait until some MP3 is received that triggered this to be sure

    char* q (p + 1);
    for (; q < p + 90 && 0 != *q; ++q) {}
    CB_ASSERT1 (0 == *q, m_strDebugFileName);
    ++q;
    *q = Id3V2Frame::COVER;
    ++q;
    for (; q < p + n && 0 != *q; ++q) {}
    CB_ASSERT1 (0 == *q, m_strDebugFileName);
    ++q; // now q points to the beginning of the actual image
    int nOffs (q - p);
    int nImgSize (n - nOffs);

    removeFrames(KnownFrames::LBL_IMAGE(), Id3V2Frame::COVER);

    {
e1:
        for (int i = 0; i < cSize(m_vpAllFrames); ++i)
        {
            const Id3V2Frame* pFrm (m_vpAllFrames[i]);

            if (0 == strcmp(KnownFrames::LBL_IMAGE(), pFrm->m_szName))
            {
                if (pFrm->m_nImgSize == nImgSize)
                {
                    Id3V2FrameDataLoader ldr (*pFrm);
                    if (0 == memcmp(&vcData[nOffs], ldr.getData() + pFrm->m_nImgOffset, nImgSize)) // !!! related to ImageInfo::operator==() status is ignored in both places //ttt1 review decision to ignore status
                    {
                        removeFrames(KnownFrames::LBL_IMAGE(), pFrm->m_nPictureType);
                        goto e1;
                    }
                }
            }
        }
    }

    {
        Id3V230Frame* p (new Id3V230Frame(KnownFrames::LBL_IMAGE(), vcData));
        p->m_nPictureType = Id3V2Frame::COVER; // probably pointless, because the frame is only used internally and what gets written is vcData, regardless of p->m_nPictureType
        addNonOwnedFrame(p);
        m_vpOwnFrames.push_back(p);
    }
}


// for UTF8 text frames, a new, owned, frame will be added instead
void Id3V230StreamWriter::addNonOwnedFrame(const Id3V2Frame* p)
{
//qDebug("add %p", p);
//qDebug("  add %p", pData);
    if (0 == strcmp(KnownFrames::LBL_IMAGE(), p->m_szName) || !KnownFrames::canHaveDuplicates(p->m_szName))
    {
        removeFrames(p->m_szName, p->m_nPictureType);
        // m_vpAllFrames.insert(m_vpAllFrames.begin(), p); //ttt1 putting a front cover image after a back cover might not be the best idea; see if it makes sense to sort the frames; (perhaps have something to sort all frames before saving)
    }

    if ('T' == p->m_szName[0] && p->m_nMemDataSize > 0 && 0 != dynamic_cast<const Id3V240Frame*>(p))
    { // check for UTF-8 encoding; for UTF8 an owned frame will be added instead
        Id3V2FrameDataLoader ldr (*p);
        const char* pData (ldr.getData());
        if (3 == pData[0])
        { // UTF-8
            addTextFrame(p->m_szName, p->getUtf8String());
            return;
        }
    }

    m_vpAllFrames.push_back(p);
}

//ttt1 perhaps transform to change image types to Cover

// if multiple frames with the same name exist, they are all removed; asserts that nPictureType is -1 for non-APIC frames
void Id3V230StreamWriter::removeFrames(const std::string& strName, int nPictureType /*= -1*/)
{
    const Id3V2Frame* p;

    for (int i = cSize(m_vpAllFrames) - 1; i >= 0; --i)
    {
        p = m_vpAllFrames[i];
        CB_ASSERT1 (-1 == nPictureType || KnownFrames::LBL_IMAGE() == strName, m_strDebugFileName);
        if (p->m_szName == strName && (p->m_nPictureType == nPictureType || m_bKeepOneValidImg))
        {
            m_vpAllFrames.erase(m_vpAllFrames.begin() + i);
        }
    }

    for (int i = cSize(m_vpOwnFrames) - 1; i >= 0; --i)
    {
        p = m_vpOwnFrames[i];
        CB_ASSERT1 (-1 == nPictureType || KnownFrames::LBL_IMAGE() == strName, m_strDebugFileName);
        if (p->m_szName == strName && (p->m_nPictureType == nPictureType || m_bKeepOneValidImg))
        {
            delete p;
            m_vpOwnFrames.erase(m_vpOwnFrames.begin() + i);
        }
    }
}

//ttt1 after rescanning when note text changed, the refound error was not visible once (couldn't replicate)


static int getUnsynchVal(int x)
{
    return (x & 0x0000007f) | ((x & 0x00003f80) << 1) | ((x & 0x001fc000) << 2) | ((x & 0x0fe00000) << 3);
}


/*static*/ const int Id3V230StreamWriter::DEFAULT_EXTRA_SPACE (1024);



// throws WriteError if it cannot write, including the case when nTotalSize is too small;
// if nTotalSize is >0, the padding will be be whatever is left;
// if nTotalSize is <0 and m_bFastSave is true, there will be a padding of around ImageInfo::MAX_IMAGE_SIZE+Id3V2Expander::EXTRA_SPACE;
// if (nTotalSize is <0 and m_bFastSave is false) or if nTotalSize is 0 (regardless of m_bFastSave), there will be a padding of between DEFAULT_EXTRA_SPACE and DEFAULT_EXTRA_SPACE + 511;
// (0 discards extra padding regardless of m_bFastSave)
void Id3V230StreamWriter::write(ostream& out, int nTotalSize /*= -1*/) const
{
    int n (cSize(m_vpAllFrames));
    //if (0 == n) { return; }

    char bfr [Id3V230Stream::ID3_HDR_SIZE] = "ID3\3\0\0"; // no unsynch, no extended header, no experimental
    int nSize (0);
    for (int i = 0; i < n; ++i)
    {
        const Id3V2Frame* p (m_vpAllFrames[i]);
        if (!p->discardOnChange())
        {
            nSize += Id3V2Frame::ID3_FRAME_HDR_SIZE + m_vpAllFrames[i]->m_nMemDataSize;
        }
    }

    //if (0 == nSize) { return; } // all frames have discardOnChange

    int nPaddedSize;

    if (nTotalSize > 0)
    {
        nPaddedSize = nTotalSize;
    }
    else if (0 == nTotalSize || !m_bFastSave)
    {
        nPaddedSize = ((nSize + (512 - 1) + DEFAULT_EXTRA_SPACE)/512)*512;
    }
    else
    { // <0 and m_bFastSave set
        nPaddedSize = nSize + ImageInfo::MAX_IMAGE_SIZE + Id3V2Expander::EXTRA_SPACE;
    }

    //if (nExtraSpace < 0) { nExtraSpace = ; }
    //int nPaddedSize (((nSize + (512 - 1) + nExtraSpace)/512)*512);

    nPaddedSize -= Id3V230Stream::ID3_HDR_SIZE;
    CB_CHECK1 (nPaddedSize >= nSize, WriteError());

    //put32BitBigEndian(1, &bfr[6]);
    put32BitBigEndian(getUnsynchVal(nPaddedSize), &bfr[6]);
    out.write(bfr, 10);

    struct LdrPtrList
    {
        LdrPtrList(int n) : m_vpLdr(n) {}
        ~LdrPtrList() { clearPtrContainer(m_vpLdr); }
        vector<Id3V2FrameDataLoader*> m_vpLdr;
    };

    LdrPtrList ldrList (n);
    for (int i = 0; i < n; ++i)
    {
        const Id3V2Frame* p (m_vpAllFrames[i]);
        if (!p->discardOnChange())
        {
            ldrList.m_vpLdr[i] = new Id3V2FrameDataLoader (*p); // this loads the data, if it's not already loaded
        }
    }

    for (int i = 0; i < n; ++i)
    {
        const Id3V2Frame* p (m_vpAllFrames[i]);
        if (!p->discardOnChange())
        {
            out.write(p->m_szName, 4);
            put32BitBigEndian(p->m_nMemDataSize, &bfr[0]);
            bfr[4] = 0; //bfr[4] = p->m_cFlag1; //ttt2 use flags if any makes sense
            bfr[5] = 0; //bfr[5] = p->m_cFlag2;
            out.write(bfr, 6);

            //Id3V2FrameDataLoader wrp (*p); // this loads the data, if it's not already loaded
            out.write(ldrList.m_vpLdr[i]->getData(), p->m_nMemDataSize);
        }
    }

    writeZeros(out, nPaddedSize - nSize);

    CB_CHECK1 (out, WriteError());
}



namespace {

struct SortFrm
{
    bool operator()(const Id3V2Frame* p1, const Id3V2Frame* p2)
    {
        int n (strcmp(p1->m_szName, p2->m_szName));
        if (0 != n) { return n < 0; }
        if (p1->m_nMemDataSize != p2->m_nMemDataSize) { return p1->m_nMemDataSize < p2->m_nMemDataSize; }

        Id3V2FrameDataLoader wrp1 (*p1);
        Id3V2FrameDataLoader wrp2 (*p2);

        n = memcmp(wrp1.getData(), wrp2.getData(), p1->m_nMemDataSize);
        return n < 0;
    }
};

}


// returns true if all of these happen: pId3V2Stream is ID3V2.3.0, no unsynch is used, the frames are identical except for their order; padding is ignored;
bool Id3V230StreamWriter::equalTo(Id3V2StreamBase* p) const
{
    if (0 == p)
    {
        //qDebug("diff ver");
        return false;
    }

    if (p->hasUnsynch())
    {
        //qDebug("diff unsynch");
        return false;
    }

    return contentEqualTo(p);
}


bool Id3V230StreamWriter::contentEqualTo(Id3V2StreamBase* p) const
{
    const vector<Id3V2Frame*>& v (p->getFrames());

    if (cSize(m_vpAllFrames) != cSize(v))
    {
        //qDebug("diff frame cnt");
        return false;
    }


    vector<const Id3V2Frame*> v1 (m_vpAllFrames);
    vector<const Id3V2Frame*> v2;
    v2.insert(v2.end(), v.begin(), v.end());

    SortFrm s;
    sort(v1.begin(), v1.end(), s);
    sort(v2.begin(), v2.end(), s);
    /*for (int i = 0, n = cSize(v1); i < n; ++i)
    {
        qDebug("frm %s %d %s %d", v1[i]->m_szName, v1[i]->m_nMemDataSize, v2[i]->m_szName, v2[i]->m_nMemDataSize);
    }//Id3V2Frame p; p.*/

    for (int i = 0, n = cSize(v1); i < n; ++i)
    {
        if (s(v1[i], v2[i]) || s(v2[i], v1[i]))
        {
            //qDebug("diff content %s %s", v1[i]->m_szName, v2[i]->m_szName);
            return false;
        }
    }

    return true;
}


//============================================================================================================
//============================================================================================================
//============================================================================================================
/*
struct KOKO
{
    KOKO()
    {
        Id3V230StreamWriter w;
        w.addTextFrame("TGRF", "abcd");
        QString s ("ab\xfeg");
        w.addTextFrame("TQQW", convStr(s));

        ofstream_utf8 out ("/r/temp/1/tmp2/e/qqq.mp3", ios::binary);
        w.write(out);
    }
};

KOKO qwerqwr;
*/



