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


#include  <QString>

#include  "ApeStream.h"

#include  "Helpers.h"

using namespace std;
using namespace pearl;


namespace {

//ttt3 these might get used uninitialized, if read before main()

const char* LBL_TITLE ("Title"); //!!! no translation - these are the names of the standard APE keys, corresponding to "TIT2" in ID3V2
const char* LBL_ARTIST ("Artist"); //ttt2 may be list
//const char* LBL_TRACK_NUMBER ("TRCK");
//const char* LBL_TRACK_YEAR ("TYER");
//const char* LBL_TRACK_DATE ("TDAT");
const char* LBL_GENRE ("Genre"); //ttt2 may be list
//const char* LBL_IMAGE ("APIC");
const char* LBL_ALBUM ("Album");


const set<string>& getUtf8Keys()
{
    static bool bFirstTime (true);
    static set<string> sUtf8Keys;
    if (bFirstTime)
    {
        sUtf8Keys.insert(LBL_TITLE);
        sUtf8Keys.insert(LBL_ARTIST);
        //sUtf8Keys.insert(LBL_TRACK_NUMBER);
        //sUtf8Keys.insert(LBL_TRACK_YEAR);
        //sUtf8Keys.insert(LBL_TRACK_DATE);
        sUtf8Keys.insert(LBL_GENRE);
        //sUtf8Keys.insert(LBL_IMAGE);
        sUtf8Keys.insert(LBL_ALBUM);

        bFirstTime = false;
    }

    return sUtf8Keys;
}

const set<string>& getUsedKeys()
{
    static bool bFirstTime (true);
    static set<string> sUsedFrames;
    if (bFirstTime)
    {
        sUsedFrames.insert(LBL_TITLE);
        sUsedFrames.insert(LBL_ARTIST); // ttt2 really list
        //sUsedFrames.insert(LBL_TRACK_NUMBER);
        //sUsedFrames.insert(LBL_TRACK_YEAR);
        //sUsedFrames.insert(LBL_TRACK_DATE);
        sUsedFrames.insert(LBL_GENRE);
        //sUsedFrames.insert(LBL_IMAGE);
        sUsedFrames.insert(LBL_ALBUM);

        bFirstTime = false;
    }

    return sUsedFrames;
}


}

//============================================================================================================
//============================================================================================================
//============================================================================================================


ApeItem::ApeItem(NoteColl& notes, istream& in) : m_eType(BINARY)
{
    StreamStateRestorer rst (in);
    streampos pos (in.tellg());
    const int BFR_SIZE (4 + 4 + 255 + 1); // key name is up to 255 chars
    char bfr [BFR_SIZE];
    streamsize nRead (read(in, bfr, BFR_SIZE));

    MP3_CHECK (nRead >= 4 + 4 + 2 + 1, pos, apeItemTooShort, ApeStream::NotApeStream());
    unsigned char* pUnsgBfr (reinterpret_cast<unsigned char*>(bfr));
    m_cFlags1 = pUnsgBfr[4];
    m_cFlags2 = pUnsgBfr[5];
    m_cFlags3 = pUnsgBfr[6];
    m_cFlags4 = pUnsgBfr[7];

    MP3_CHECK (0 == m_cFlags1 && 0 == m_cFlags2 && 0 == m_cFlags3 && 0 == m_cFlags4, pos, apeFlagsNotSupported, StreamIsUnsupported(ApeStream::getClassDisplayName(), tr("Ape stream whose items have unsupported flags.")));
    //MP3_CHECK (0 == (m_cFlags1 & 0xf8u) && 0 == m_cFlags2 && 0 == m_cFlags3 && 0 == m_cFlags4, pos, apeFlagsNotSupported, StreamIsUnsupported(ApeStream::getClassDisplayName(), "Ape stream whose items have unsupported flags.")); // ttt1 allow this; see 02_-_Brave_-_Driven.mp3, which has a "BINARY" value (hence the "2" flag, which is also larger than 256)
//inspect(bfr, BFR_SIZE);
    int nDataSize (pUnsgBfr[0] + (pUnsgBfr[1] << 8) + (pUnsgBfr[2] << 16) + (pUnsgBfr[3] << 24));
    char* p (bfr + 8);
    for (; 0 != *p && p < bfr + nRead; ++p) {}
    MP3_CHECK (p < bfr + nRead, pos, apeMissingTerminator, ApeStream::NotApeStream());

    m_strName = string(bfr + 8, p);

    if (nDataSize > 256)
    {
        MP3_NOTE_D (pos, apeItemTooBig, tr("Ape Item seems too big. Although the size may be any 32-bit integer, 256 bytes should be enough in practice. If this message is determined to be mistaken, it will be removed in the future. Item key: %1; item size: %2").arg(convStr(m_strName)).arg(nDataSize));
        throw ApeStream::NotApeStream(); //ttt1 actually it's possible; see 02_-_Brave_-_Driven.mp3
    }

    m_vcValue.resize(nDataSize);
    pos += getTotalSize() - nDataSize;
    in.clear();
    in.seekg(pos);
    MP3_CHECK (nDataSize == read(in, &m_vcValue[0], nDataSize), pos, apeItemTooShort, ApeStream::NotApeStream());

    if (getUtf8Keys().count(m_strName) > 0)
    {
        m_eType = UTF8;
    }

    if (UTF8 != m_eType)
    {
        if (nDataSize <= 256)
        {
            for (int i = 0; i < nDataSize; ++i)
            {
                if (m_vcValue[i] < 32 || m_vcValue[i] > 126) { goto e1; }
            }
        }
        m_eType = UTF8;
    e1:;
    }

    rst.setOk();
//inspect(m_aValue, m_nDataSize);
}


string ApeItem::getUtf8String() const
{
    switch (m_eType)
    {
    case BINARY: return convStr(TagReader::tr("<non-text value>"));
    case UTF8: return string(&m_vcValue[0], &m_vcValue[0] + cSize(m_vcValue));
    }
    CB_ASSERT(false);
}


//============================================================================================================
//============================================================================================================
//============================================================================================================


ApeStream::ApeStream(int nIndex, NoteColl& notes, istream& in) : DataStream(nIndex)
{
    StreamStateRestorer rst (in);

    m_pos = in.tellg();

    //const int APE_LABEL_SIZE (8);
    //const int BFR_SIZE (32);//(APE_LABEL_SIZE + 4 + 4);
    char bfr [32];

    streamsize nRead (read(in, bfr, 32));
    MP3_CHECK_T (32 == nRead, m_pos, "Not an Ape tag. File too short.", NotApeStream());
//inspect (bfr, m_pos);

    MP3_CHECK_T (0 == strncmp("APETAGEX", bfr, 8), m_pos, "Not an Ape tag. Invalid header.", NotApeStream());
    m_nVersion = *reinterpret_cast<int*>(bfr + 8); // ttt2 assume 32-bit int + little-endian
    m_nSize = *reinterpret_cast<int*>(bfr + 12); // ttt2 assume 32-bit int + little-endian

    MP3_CHECK (0x80 == (0xc0 & (unsigned char)bfr[23]), m_pos, apeUnsupported, StreamIsUnsupported(ApeStream::getClassDisplayName(), tr("Tag missing header or footer."))); //ttt2 assumes both header & footer are present, but they are optional;
    MP3_CHECK (0 != (0x20 & bfr[23]), m_pos, apeFoundFooter, NotApeHeader());

    streampos posEnd (m_pos);
    posEnd += m_nSize;
    m_nSize += 32; // account for header
    in.seekg(posEnd);

    char bfr2 [32];
    nRead = read(in, bfr2, 32);
    MP3_CHECK (32 == nRead, m_pos, apeTooShort, NotApeStream());

    MP3_CHECK (0 == (0x20 & bfr2[23]), m_pos, apeFoundHeader, NotApeFooter());
    bfr2[23] |= 0x20;
//inspect (bfr2, posEnd);
    for (int i = 0; i < 32; ++i)
    {
        MP3_CHECK (bfr[i] == bfr2[i], m_pos, apeHdrFtMismatch, HeaderFooterMismatch());
    }

    readKeys(notes, in);

    MP3_TRACE (m_pos, "ApeStream built.");

    rst.setOk();
}

ApeStream::~ApeStream()
{
    clearPtrContainer(m_vpItems);
}

/*override*/ void ApeStream::copy(std::istream& in, std::ostream& out)
{
    appendFilePart(in, out, m_pos, m_nSize);
}

/*override*/ std::string ApeStream::getInfo() const
{
    return "";
}

void ApeStream::readKeys(NoteColl& notes, std::istream& in)
{
    streampos posEnd (m_pos);
    posEnd += m_nSize - 32; // where the footer begins

    streampos posCrt (m_pos);
    posCrt += 32;
    in.seekg(posCrt);

    while (posCrt < posEnd)
    {
        ApeItem* p (new ApeItem(notes, in));
        posCrt += p->getTotalSize();
        m_vpItems.push_back(p);
    }
}


ApeItem* ApeStream::findItem(const char* szFrameName)  //ttt2 finds the first item, but doesn't care about duplicates; not sure how Ape views duplicate keys
{
    for (int i = 0, n = cSize(m_vpItems); i < n; ++i)
    {
        ApeItem* p = m_vpItems[i];
        if (szFrameName == p->m_strName) { return p; }
    }
    return 0;
}


const ApeItem* ApeStream::findItem(const char* szFrameName) const //ttt2 finds the first item, but doesn't care about duplicates; not sure how Ape views duplicate keys
{
    for (int i = 0, n = cSize(m_vpItems); i < n; ++i)
    {
        const ApeItem* p = m_vpItems[i];
        if (szFrameName == p->m_strName) { return p; }
    }
    return 0;
}


//Mp3Gain ApeStream::getMp3GainStatus() const
/*

There are MP3GAIN and REPLAYGAIN. Not sure how they are related, because the program mp3gain creates both MP3GAIN and REPLAYGAIN entries. Here's a newly generated tag:

MP3GAIN_MINMAX="139,240",
MP3GAIN_UNDO="+005,+005,N",
REPLAYGAIN_TRACK_GAIN="-0.205000 dB",
REPLAYGAIN_TRACK_PEAK="0.466454

and an older one:

MP3GAIN_MINMAX="089,179",
MP3GAIN_ALBUM_MINMAX="000,196",
MP3GAIN_UNDO="+005,+005,N",
REPLAYGAIN_TRACK_GAIN="+6.915000 dB",
REPLAYGAIN_TRACK_PEAK="0.289101",
REPLAYGAIN_ALBUM_GAIN="+6.600000 dB",
REPLAYGAIN_ALBUM_PEAK="0.886662"

It seems that MP3GAIN is about changing audio data (for which there is an undo as long as the Ape tag is not deleted), while REPLAYGAIN only adds tags, which require compatible players.

Not sure how to tell from these if album and/or track info are found. Currently (2009.03.14) it doesn't matter, so a bool is better.
*/
bool ApeStream::hasMp3Gain() const
{
    return 0 != findItem("MP3GAIN_MINMAX") || 0 != findItem("MP3GAIN_ALBUM_MINMAX") || 0 != findItem("REPLAYGAIN_TRACK_GAIN") || 0 != findItem("REPLAYGAIN_ALBUM_GAIN");
}





// ================================ TagReader =========================================




/*override*/ TagReader::SuportLevel ApeStream::getSupport(Feature eFeature) const
{
    switch (eFeature)
    {
    case TITLE:
    case ARTIST:
    //case TRACK_NUMBER:
    //case TRACK_YEAR:
    //case TRACK_DATE:
    case GENRE:
    //case PICTURE:
    case ALBUM:
        return READ_ONLY;

    default:
        return NOT_SUPPORTED;
    }
    // enum SuportLevel { NOT_SUPPORTED, , READ_WRITE };
}


/*override*/ std::string ApeStream::getTitle(bool* pbFrameExists /* = 0*/) const
{
    const ApeItem* p (findItem(LBL_TITLE));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}


/*override*/ std::string ApeStream::getArtist(bool* pbFrameExists /* = 0*/) const
{
    const ApeItem* p (findItem(LBL_ARTIST));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}

/*override*/ std::string ApeStream::getTrackNumber(bool* /*pbFrameExists*/ /* = 0*/) const
{
    /*const ApeItem* p (findItem(LBL_TRACK_NUMBER));
    if (0 == p) { return ""; }
    return p->getUtf8String();*/
    throw NotSupportedOp();
}


/*override*/ TagTimestamp ApeStream::getTime(bool* /* = 0*/) const
{
    throw NotSupportedOp();
}


/*override*/ std::string ApeStream::getGenre(bool* pbFrameExists /* = 0*/) const
{
    const ApeItem* p (findItem(LBL_GENRE)); // not always correct; the specs say it's a "numeric string"; usually a descriptive string seems to be used though, not a number; anyway, "Cenaclul Flacara 3/c06 Anda Calugareanu - Noi, nu.mp3" has a "(80)" in this field
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}

/*override*/ std::string ApeStream::getAlbumName(bool* pbFrameExists /* = 0*/) const
{
    const ApeItem* p (findItem(LBL_ALBUM));
    if (0 != pbFrameExists) { *pbFrameExists = 0 != p; }
    if (0 == p) { return ""; }
    return p->getUtf8String();
}


/*override*/ std::string ApeStream::getOtherInfo() const
{
    const set<string>& sUsedKeys (getUsedKeys());

    set<string> sUsedFrames;

    ostringstream out;
    bool b (false);

    for (int i = 0, n = cSize(m_vpItems); i < n; ++i)
    {
        ApeItem* p = m_vpItems[i];
        if (sUsedKeys.count(p->m_strName) > 0 && sUsedFrames.count(p->m_strName) == 0)
        {
            sUsedFrames.insert(p->m_strName);
        }
        else
        {
            if (b) { out << ", "; }
            out << p->m_strName << "=\"" << p->getUtf8String() << "\"";
            b = true;
        }
    }
    return out.str();
}





