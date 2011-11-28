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
#include  <memory>

#include  "MpegStream.h"
#include  "Mp3Manip.h"


using namespace std;


/*namespace {

// moves the read pointer to the end of the frame; throws if it cannot do so
MpegFrame getMpegFrame(istream& in)
{
    MpegFrame frame (in);
}

}*/


MpegStreamBase::MpegStreamBase(int nIndex, NoteColl& notes, istream& in) : DataStream(nIndex)
{
    m_pRst = new StreamStateRestorer(in);
    auto_ptr<StreamStateRestorer> pRst (m_pRst);
    m_pos = in.tellg();
    m_firstFrame = MpegFrame(notes, in);

    /*m_eVersion;
    m_eLayer;
    m_eChannelMode;
    m_nFrequency;
    m_nSize;*/

    MP3_TRACE (m_pos, "MpegStreamBase built.");
    pRst.release();
}

MpegStreamBase::~MpegStreamBase()
{
    delete m_pRst;
}


/*override*/ void MpegStreamBase::copy(std::istream& in, std::ostream& out)
{
    appendFilePart(in, out, m_pos, m_firstFrame.getSize());
}





MpegStream::MpegStream(int nIndex, NoteColl& notes, istream& in) : MpegStreamBase(nIndex, notes, in), m_bVbr(false), m_nFrameCount(1), m_posLastFrame(-1), m_bRemoveLastFrameCalled(false)
{
    MpegFrame frm;
    //int nFrameCount (1);
    streampos pos (m_pos);
    pos += m_firstFrame.getSize();
    m_nTotalBps = m_firstFrame.getBitrate();
    int nSecondFrameBitrate (-1); // to determine if the first frame has different bitrate from the others
    bool bVbr2 (false);
    for (;;)
    {
        try
        {
            frm = MpegFrame(notes, in);
        }
        catch (const EndOfFile&)
        {
            break;
        }
        catch (const MpegFrame::NotMpegFrame&)
        {
            break;
        }
        catch (const MpegFrame::PrematurelyEndedMpegFrame&)
        {
            if (m_nFrameCount >= MIN_FRAME_COUNT) { MP3_NOTE (pos, incompleteFrameInAudio); } //ttt2 perhaps include ver/layer/... in PrematurelyEndedMpegFrame and have different messages when the incomplete frame matches the stream and when it doesn't
            break;
        }

        if (m_firstFrame.getVersion() != frm.getVersion())
        {
            // end; not part of the sequence
            if (m_nFrameCount >= MIN_FRAME_COUNT) { MP3_NOTE (pos, validFrameDiffVer); }
            break;
        }

        if (m_firstFrame.getLayer() != frm.getLayer())
        {
            // end; not part of the sequence
            if (m_nFrameCount >= MIN_FRAME_COUNT) { MP3_NOTE (pos, validFrameDiffLayer); }
            break;
        }

        if (m_firstFrame.getChannelMode() != frm.getChannelMode())
        {
            // end; not part of the sequence
            if (m_nFrameCount >= MIN_FRAME_COUNT) { MP3_NOTE (pos, validFrameDiffMode); }
            break;
        }

        if (m_firstFrame.getFrequency() != frm.getFrequency())
        {
            // end; not part of the sequence
            if (m_nFrameCount >= MIN_FRAME_COUNT) { MP3_NOTE (pos, validFrameDiffFreq); }
            break;
        }

        if (m_firstFrame.getCrcUsage() != frm.getCrcUsage())
        {
            // end; not part of the sequence
            if (m_nFrameCount >= MIN_FRAME_COUNT) { MP3_NOTE (pos, validFrameDiffCrc); }
            break;
        }

        m_posLastFrame = pos;
        m_lastFrame =  frm;
        pos += frm.getSize();
        m_nTotalBps += frm.getBitrate();
        ++m_nFrameCount;
        if (frm.getBitrate() != m_firstFrame.getBitrate())
        {
            m_bVbr = true;
        }

        if (-1 == nSecondFrameBitrate)
        {
            nSecondFrameBitrate = frm.getBitrate();
        }
        else
        {
            if (nSecondFrameBitrate != frm.getBitrate())
            {
                bVbr2 = true;
            }
        }
    }

    //MP3_CHECK (m_nFrameCount >= MIN_FRAME_COUNT, m_pos, "Invalid MPEG stream. Stream has fewer than 10 frames.", StreamTooShort(getInfo()));
    if (m_nFrameCount < MIN_FRAME_COUNT)
    {
        in.clear();
        in.seekg(m_pos);
        char bfr [4];
        string strInfo;
        if (4 == read(in, bfr, 4)) // normally this should work, because to get here at least a full frame needs to be read
        {
            strInfo = decodeMpegFrame(bfr, ", ");
        }
        MP3_THROW (m_pos, audioTooShort, StreamTooShort(strInfo, m_nFrameCount));
    }

    MP3_CHECK (!m_bVbr || bVbr2, m_pos, diffBitrateInFirstFrame, UnknownHeader()); //ttt2 perhaps add test for "null": whatever is in the first bytes that allows Xing & Co to not generate audio in decoders that don't know about them

    m_nSize = pos - m_pos;
    in.seekg(pos);
    m_nBitrate = int(m_nTotalBps / m_nFrameCount);

    /*if (m_firstFrame.getCrcUsage())
    {
        MP3_NOTE (m_pos, "Stream uses CRC.");
    }*/

    if (
        !(MpegFrame::MPEG1 == m_firstFrame.getVersion() && MpegFrame::LAYER3 == m_firstFrame.getLayer()) &&
        !(MpegFrame::MPEG2 == m_firstFrame.getVersion() && MpegFrame::LAYER3 == m_firstFrame.getLayer()))
    {
        MP3_NOTE (m_pos, untestedEncoding);
    }

    MP3_TRACE (m_pos, "MpegStream built.");

    setRstOk();
}



// this can only be called once; the second call will throw (it's harder and quite pointless to allow more than one call)
void MpegStream::removeLastFrame()
{
    STRM_ASSERT (!m_bRemoveLastFrameCalled);
    m_bRemoveLastFrameCalled = true;
    m_nSize -= m_lastFrame.getSize();
    m_nTotalBps -= m_lastFrame.getBitrate();
    STRM_ASSERT (m_nFrameCount >= 10);
    --m_nFrameCount;
    m_nBitrate = int(m_nTotalBps / m_nFrameCount);
}


/*override*/ void MpegStream::copy(std::istream& in, std::ostream& out)
{
    appendFilePart(in, out, m_pos, m_nSize);
}


/*override*/ std::string MpegStream::getInfo() const
{
    ostringstream out;
    out << getDuration() << ", " << m_firstFrame.getSzVersion() << " " << m_firstFrame.getSzLayer() << ", " << m_firstFrame.getSzChannelMode() << ", " <<
        m_firstFrame.getFrequency() << "Hz, " << m_nBitrate << "bps " << (m_bVbr ? "VBR" : "CBR") << ", CRC=" <<
        boolAsYesNo(m_firstFrame.getCrcUsage()) << ", frame count=" << m_nFrameCount;
    out << "; last frame" << (m_bRemoveLastFrameCalled ? " removed; it was" : "") << " located at 0x" << hex << m_posLastFrame << dec;
    return out.str();
}

std::string MpegStream::getDuration() const
{
    int nDur (int(m_nSize*8.0/m_nBitrate));
    int nMin (nDur/60);
    int nSec (nDur - nMin*60);
    char a [15];
    sprintf(a, "%d:%02d", nMin, nSec);
    return a;
}

bool MpegStream::isCompatible(const MpegFrameBase& frame)
{
    if (getFirstFrame().getVersion() != frame.getVersion()) { return false; }
    if (getFirstFrame().getLayer() != frame.getLayer()) { return false; }
    if (getFirstFrame().getChannelMode() != frame.getChannelMode()) { return false; }
    if (getFirstFrame().getFrequency() != frame.getFrequency()) { return false; }
    if (getFirstFrame().getCrcUsage() != frame.getCrcUsage()) { return false; }

    return true;
}


// moves the read pointer to the first frame compatible with the stream; returns "false" if no such frame is found
bool MpegStream::findNextCompatFrame(std::istream& in, std::streampos posMax)
{
    streampos pos (in.tellg());
    NoteColl notes (10);
    for (;;)
    {
        try
        {
            pos = getNextStream(in, pos);
            if (pos > posMax)
            {
                return false;
            }

            in.seekg(pos);
            try
            {
                MpegFrameBase frm (notes, in);
                if (isCompatible(frm))
                {
                    in.seekg(pos);
                    return true;
                }
            }
            catch (const MpegFrameBase::NotMpegFrame&)
            {
            }
        }
        catch (const EndOfFile&)
        {
            return false;
        }
    }
}


#ifdef GENERATE_TOC //ttt2 maybe improve and use
// throws if it can't write to the disk
void createXing(ostream& out, const MpegFrame& frame1, int nFrameCount, streamoff nStreamSize)
{
    const MpegFrameBase& frame (frame1.getBigBps());

    int nSize (frame.getSize());
    out.write(frame.getHeader(), MpegFrame::MPEG_FRAME_HDR_SIZE);
    int nSideInfoSize (frame.getSideInfoSize());
    writeZeros(out, nSideInfoSize);
    out.write("Xing\0\0\0\7", 8);
    char bfr [4];
    put32BitBigEndian(nFrameCount, bfr);
    out.write(bfr, 4);
    put32BitBigEndian(nStreamSize, bfr);
    out.write(bfr, 4);
    for (int i = 0; i < 100; ++ i)
    {
        bfr[0] = i*255/(100 - 1);
        out.write(bfr, 1);
    }
    writeZeros(out, nSize - MpegFrame::MPEG_FRAME_HDR_SIZE - nSideInfoSize - 8 - 4 - 4 - 100);
    CB_CHECK1 (out, WriteError());
}
#else
void createXing(ostream& out, const MpegFrame& frame, int nFrameCount, streamoff nStreamSize)
{
    int nSize (frame.getSize());
    out.write(frame.getHeader(), MpegFrame::MPEG_FRAME_HDR_SIZE);
    int nSideInfoSize (frame.getSideInfoSize());
    writeZeros(out, nSideInfoSize);
    out.write("Xing\0\0\0\3", 8);
    char bfr [4];
    put32BitBigEndian(nFrameCount, bfr);
    out.write(bfr, 4);
    put32BitBigEndian(nStreamSize, bfr);
    out.write(bfr, 4);
    writeZeros(out, nSize - MpegFrame::MPEG_FRAME_HDR_SIZE - nSideInfoSize - 8 - 4 - 4);
    CB_CHECK1 (out, WriteError());
}
#endif

void MpegStream::createXing(ostream& out)
{
    static const int MIN_FRAME_SIZE (200); // ttt2 this 200 is arbitrary, but there's probably enough room for TOC
    if (m_firstFrame.getSize() >= MIN_FRAME_SIZE)
    {
        ::createXing(out, m_firstFrame, m_nFrameCount, getSize());
    }
    else
    {
        static const int BFR_SIZE (2000);
        static char aNewHeader [BFR_SIZE];
        ::fill(aNewHeader, aNewHeader + BFR_SIZE, 0);
        ::copy(m_firstFrame.getHeader(), m_firstFrame.getHeader() + MpegFrame::MPEG_FRAME_HDR_SIZE, aNewHeader);
        NoteColl notes;
        for (int i = 1; i <= 14; ++i)
        {
            aNewHeader[2] = (aNewHeader[2] & 0x0f) + (i << 4);
            istringstream in (string(aNewHeader, BFR_SIZE));

            MpegFrame frame (notes, in);
            if (frame.getSize() >= MIN_FRAME_SIZE)
            {
                ::createXing(out, frame, m_nFrameCount, getSize());
                return;
            }
        }
    }

    CB_ASSERT(false);
}


//ttt2 perhaps make clear in messages that Xing is OK with CBR, which by convention should use "Info" instead of "Xing" but some tools don't follow this; don't delete Xing just because it's followed a CBR

XingStreamBase::XingStreamBase(int nIndex, NoteColl& notes, istream& in) : MpegStreamBase(nIndex, notes, in), m_nFrameCount(-1), m_nByteCount(-1), m_nQuality(-1)
{
    fill(&m_toc[0], &m_toc[100], 0);
    in.seekg(m_pos);

    const int XING_LABEL_SIZE (4);
    const int BFR_SIZE (MpegFrame::MPEG_FRAME_HDR_SIZE + 32 + XING_LABEL_SIZE); // MPEG header + side info + "Xing" size //ttt2 not sure if space for CRC16 should be added; then not sure if frame size should be increased by 2 when CRC is found
    char bfr [BFR_SIZE];

    int nSideInfoSize (m_firstFrame.getSideInfoSize());

    int nBfrSize (MpegFrame::MPEG_FRAME_HDR_SIZE + nSideInfoSize + XING_LABEL_SIZE);

    MP3_CHECK_T (nBfrSize <= m_firstFrame.getSize(), m_pos, "Not a Xing stream. This kind of MPEG audio doesn't support Xing.", NotXingStream()); // !!! some kinds of MPEG audio (e.g. "MPEG-1 Layer I, 44100Hz 32000bps" or "MPEG-2 Layer III, 22050Hz 8000bps") have very short frames, which can't accomodate a Xing header

    streamsize nRead (read(in, bfr, nBfrSize));
    STRM_ASSERT (nBfrSize == nRead); // this was supposed to be a valid frame to begin with (otherwise the base class would have thrown) and nBfrSize is no bigger than the frame

    char* pLabel (bfr + MpegFrame::MPEG_FRAME_HDR_SIZE + nSideInfoSize);
    MP3_CHECK_T (0 == strncmp("Xing", pLabel, XING_LABEL_SIZE) || 0 == strncmp("Info", pLabel, XING_LABEL_SIZE), m_pos, "Not a Xing stream. Header not found.", NotXingStream());

    // ttt0 perhaps if it gets this far it should generate some "broken xing": with the incorrect "vbr fix" which created a xing header longer than the mpeg frame that was supposed to contain it, followed by the removal of those extra bytes by the "unknown stream removal" causes the truncated xing header to be considered audio; that wouldn't happen if a "broken xing" stream would be tried before the "audio" stream in Mp3Handler::parse()
    MP3_CHECK_T (4 == read(in, bfr, 4) && 0 == bfr[0] && 0 == bfr[1] && 0 == bfr[2], m_pos, "Not a Xing stream. Header not found.", NotXingStream());
    m_cFlags = bfr[3];
    MP3_CHECK_T ((m_cFlags & 0x0f) == m_cFlags, m_pos, "Not a Xing stream. Invalid flags.", NotXingStream());
    if (0x01 == (m_cFlags & 0x01))
    { // has frames
        MP3_CHECK_T (4 == read(in, bfr, 4), m_pos, "Not a Xing stream. File too short.", NotXingStream());
        m_nFrameCount = get32BitBigEndian(bfr);
    }

    if (0x02 == (m_cFlags & 0x02))
    { // has bytes
        MP3_CHECK_T (4 == read(in, bfr, 4), m_pos, "Not a Xing stream. File too short.", NotXingStream());
        m_nByteCount = get32BitBigEndian(bfr);
    }

    if (0x04 == (m_cFlags & 0x04))
    { // has TOC
        MP3_CHECK_T (100 == read(in, m_toc, 100), m_pos, "Not a Xing stream. File too short.", NotXingStream());
    }

    if (0x08 == (m_cFlags & 0x08))
    { // has quality
        MP3_CHECK_T (4 == read(in, bfr, 4), m_pos, "Not a Xing stream. File too short.", NotXingStream());
        m_nQuality = get32BitBigEndian(bfr);
    }

    streampos posEnd (m_pos);
    posEnd += m_firstFrame.getSize();
    in.seekg(posEnd); //ttt2 2010.12.07 - A header claiming to have TOC but lacking one isn't detected. 1) Should check that values in TOC are ascending. 2) Should check that it actually fits: a 104 bytes-long 32kbps frame cannot hold a 100 bytes TOC along with the header and other things.
}


void XingStreamBase::getXingInfo(std::ostream& out) const
{
    out << "[Xing header info:";
    bool b (false);
    if (0x01 == (m_cFlags & 0x01)) { out << " frame count=" << m_nFrameCount; b = true; }
    if (0x02 == (m_cFlags & 0x02)) { out << (b ? "," : "") << " byte count=" << m_nByteCount; b = true; } //ttt2 see what to do with this: it's the size of the whole file, all headers&tags included (at least with c03 Valentin Moldovan - Marea Irlandei.mp3); ??? and anyway,  what's the point of including the size of the whole file as a field?
    if (0x04 == (m_cFlags & 0x04)) { out << (b ? "," : "") << " TOC present"; b = true; }
    if (0x08 == (m_cFlags & 0x08)) { out << (b ? "," : "") << " quality=" << m_nQuality; b = true; }
    out << "]";
}

std::string XingStreamBase::getInfoForXml() const
{
    ostringstream out;
    if (0x01 == (m_cFlags & 0x01)) { out << " frameCount=\"" << m_nFrameCount << "\""; }
    if (0x02 == (m_cFlags & 0x02)) { out << " byteCount=\"" << m_nByteCount << "\""; } //ttt2 see what to do with this: it's the size of the whole file, all headers&tags included (at least with c03 Valentin Moldovan - Marea Irlandei.mp3); ??? and anyway,  what's the point of including the size of the whole file as a field?
    if (0x04 == (m_cFlags & 0x04)) { out << " toc=\"yes\""; }
    if (0x08 == (m_cFlags & 0x08)) { out << " quality=\"" << m_nQuality << "\""; }
    return out.str();
}


/*override*/ std::string XingStreamBase::getInfo() const
{
    ostringstream out;
    out << m_firstFrame.getSzVersion() << " " << m_firstFrame.getSzLayer() << ", " << m_firstFrame.getSzChannelMode() << ", " << m_firstFrame.getFrequency() << "Hz"
        << ", " << m_firstFrame.getBitrate() << "bps, CRC=" << boolAsYesNo(m_firstFrame.getCrcUsage()) << "; ";
    getXingInfo(out);
    return out.str();
}



bool XingStreamBase::matchesStructure(const MpegStream& mpeg) const
{
    const MpegFrame& mpegFrm (mpeg.getFirstFrame());
    return (m_firstFrame.getVersion() == mpegFrm.getVersion() &&
        m_firstFrame.getLayer() == mpegFrm.getLayer() &&
        //m_firstFrame.getChannelMode() == mpegFrm.getChannelMode() &&
        m_firstFrame.getFrequency() == mpegFrm.getFrequency());
}

// checks that there is a metch for version, layer, frequency and frame count
bool XingStreamBase::matches(const MpegStream& mpeg) const
{
    return matchesStructure(mpeg) && getFrameCount() == mpeg.getFrameCount();
}


// checks that pNext is MpegStream* in addition to matches(const MpegStream&)
bool XingStreamBase::matches(const DataStream* pNext) const
{
    const MpegStream* q (dynamic_cast<const MpegStream*>(pNext));
    return 0 != q && matches(*q);
}


bool XingStreamBase::isBrokenByMp3Fixer(const DataStream* pNext, const DataStream* pAfterNext) const
{
    if (16 != pNext->getSize()) { return false; }

    const MpegStream* q (dynamic_cast<const MpegStream*>(pAfterNext));
    return 0 != q && matchesStructure(*q) && getFrameCount() == q->getFrameCount() + 1;
}



XingStream::XingStream(int nIndex, NoteColl& notes, std::istream& in) : XingStreamBase(nIndex, notes, in)
{
    MP3_TRACE (m_pos, "XingStream built.");

    setRstOk();
}




LameStream::LameStream(int nIndex, NoteColl& notes, istream& in) : XingStreamBase(nIndex, notes, in)
{
    in.seekg(m_pos);

    const int LAME_LABEL_SIZE (4);
    const int LAME_OFFS (156);
    const int BFR_SIZE (LAME_OFFS + LAME_LABEL_SIZE); // MPEG header + side info + "Xing" size //ttt2 not sure if space for CRC16 should be added; then not sure if frame size should be increased by 2 when CRC is found
    char bfr [BFR_SIZE];

    MP3_CHECK_T (BFR_SIZE <= m_firstFrame.getSize(), m_pos, "Not a LAME stream. This kind of MPEG audio doesn't support LAME.", NotLameStream()); // !!! some kinds of MPEG audio have very short frames, which can't accomodate a VBRI header

    streamsize nRead (read(in, bfr, BFR_SIZE));
    STRM_ASSERT (BFR_SIZE == nRead); // this was supposed to be a valid frame to begin with (otherwise the base class would have thrown) and BFR_SIZE is no bigger than the frame

    MP3_CHECK_T (0 == strncmp("LAME", bfr + LAME_OFFS, LAME_LABEL_SIZE), m_pos, "Not a LAME stream. Header not found.", NotLameStream());

    streampos posEnd (m_pos);
    posEnd += m_firstFrame.getSize();
    in.seekg(posEnd);

    MP3_TRACE (m_pos, "LameStream built.");

    setRstOk();
}


/*override*/ std::string LameStream::getInfo() const
{
    ostringstream out;
    out << m_firstFrame.getSzVersion() << " " << m_firstFrame.getSzLayer() << ", " << m_firstFrame.getSzChannelMode() << ", " << m_firstFrame.getFrequency() << "Hz"
        << ", " << m_firstFrame.getBitrate() << "bps, CRC=" << boolAsYesNo(m_firstFrame.getCrcUsage()) << "; ";
    getXingInfo(out);
    return out.str();
}



//ttt2 see why after most VBRI headers comes an "unknown" stream; perhaps there's an error in how VbriStream works
VbriStream::VbriStream(int nIndex, NoteColl& notes, istream& in) : MpegStreamBase(nIndex, notes, in)
{
    in.seekg(m_pos);
    const int VBRI_LABEL_SIZE (4);
    const int BFR_SIZE (MpegFrame::MPEG_FRAME_HDR_SIZE + 32 + VBRI_LABEL_SIZE); // MPEG header + side info + "Xing" size //ttt2 not sure if space for CRC16 should be added; then not sure if frame size should be increased by 2 when CRC is found
    char bfr [BFR_SIZE];

    MP3_CHECK_T (BFR_SIZE <= m_firstFrame.getSize(), m_pos, "Not a VBRI stream. This kind of MPEG audio doesn't support VBRI.", NotVbriStream()); // !!! some kinds of MPEG audio have very short frames, which can't accomodate a VBRI header

    streamsize nRead (read(in, bfr, BFR_SIZE));
    STRM_ASSERT (BFR_SIZE == nRead); // this was supposed to be a valid frame to begin with (otherwise the base class would have thrown) and BFR_SIZE is no bigger than the frame

    char* pLabel (bfr + MpegFrame::MPEG_FRAME_HDR_SIZE + 32);
    MP3_CHECK_T (0 == strncmp("VBRI", pLabel, VBRI_LABEL_SIZE), m_pos, "Not a VBRI stream. Header not found.", NotVbriStream());

    streampos posEnd (m_pos);
    posEnd += m_firstFrame.getSize();
    in.seekg(posEnd);

    MP3_TRACE (m_pos, "VbriStream built.");

    setRstOk();
}

/*override*/ std::string VbriStream::getInfo() const
{
    ostringstream out;
    out << m_firstFrame.getSzVersion() << " " << m_firstFrame.getSzLayer() << ", " << m_firstFrame.getSzChannelMode() << ", " << m_firstFrame.getFrequency() << "Hz"
        << ", " << m_firstFrame.getBitrate() << "bps, CRC=" << boolAsYesNo(m_firstFrame.getCrcUsage());
    return out.str();
}






//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================



Id3V1Stream::Id3V1Stream(int nIndex, NoteColl& notes, istream& in) : DataStream(nIndex), m_pos(in.tellg())
{
    StreamStateRestorer rst (in);

    const int BFR_SIZE (128);
    streamsize nRead (read(in, m_data, BFR_SIZE));
    MP3_CHECK_T (BFR_SIZE == nRead, m_pos, "Invalid ID3V1 tag. File too short.", NotId3V1Stream());
    MP3_CHECK_T (0 == strncmp("TAG", m_data, 3), m_pos, "Invalid ID3V1 tag. Invalid header.", NotId3V1Stream());

    MP3_CHECK (BFR_SIZE == nRead, m_pos, id3v1TooShort, NotId3V1Stream());

    // not 100% correct, but should generally work
    if (0 == m_data[125] && 0 != m_data[126])
    {
        m_eVersion = V11b;
    }
    else
    {
        unsigned char c ((unsigned char)m_data[127]);
        m_eVersion = (' ' == m_data[125] && ' ' == m_data[126]) || (0 == m_data[125] && 0 == m_data[126]) || (c > 0 && c < ' ') ? V11 : V10;
    }

    // http://uweb.txstate.edu/~me02/tutorials/sound_file_formats/mpeg/tags.htm
    TestResult eTrack (checkId3V1String(m_data + 3, 30)); MP3_CHECK (BAD != eTrack, m_pos, id3v1InvalidName, NotId3V1Stream());
    TestResult eArtist (checkId3V1String(m_data + 33, 30)); MP3_CHECK (BAD != eArtist, m_pos, id3v1InvalidArtist, NotId3V1Stream());
    TestResult eAlbum (checkId3V1String(m_data + 63, 30)); MP3_CHECK (BAD != eAlbum, m_pos, id3v1InvalidAlbum, NotId3V1Stream());
    TestResult eYear (checkId3V1String(m_data + 93, 4)); MP3_CHECK (BAD != eYear, m_pos, id3v1InvalidYear, NotId3V1Stream());
    TestResult eComment (checkId3V1String(m_data + 97, 28)); MP3_CHECK (BAD != eComment, m_pos, id3v1InvalidComment, NotId3V1Stream()); // "28" is for ID3V1.1b (there's no reliable way to distinguish among versions 1.0 and 1.1 by design, and in practice among any of them because some tools use 0 instead of space and 0 seems to be a valid value for 1.1b's track and genre, for "undefined") //ttt2 use m_eVersion

    if (ZERO_PADDED == eTrack || ZERO_PADDED == eArtist || ZERO_PADDED == eAlbum || ZERO_PADDED == eYear || ZERO_PADDED == eComment)
    {
        // MP3_NOTE (m_pos, zeroInId3V1 /*"ID3V1 tag contains characters with the code 0, although this is not allowed by the standard (yet used by some tools)."*/);
        if (SPACE_PADDED == eTrack || SPACE_PADDED == eArtist || SPACE_PADDED == eAlbum || SPACE_PADDED == eYear || SPACE_PADDED == eComment)
        {
            MP3_NOTE (m_pos, mixedPaddingInId3V1);
        }
    }
    if (MIXED_PADDED == eTrack || MIXED_PADDED == eArtist || MIXED_PADDED == eAlbum || MIXED_PADDED == eYear || MIXED_PADDED == eComment)
    {
        MP3_NOTE (m_pos, mixedFieldPaddingInId3V1);
    }

    MP3_TRACE (m_pos, "Id3V1Stream built.");

    rst.setOk();
}



/*static*/ bool Id3V1Stream::isLegal(char c)
{
    unsigned char x (c);
    return x >= 32;
}

// makes sure that a valid string is stored at the address given, meaning no chars smaller than 32; well, except for 0: 0 isn't really valid, as the fields are supposed to be padded with spaces at the right, but some tools use 0 anyway
/*static*/ Id3V1Stream::TestResult Id3V1Stream::checkId3V1String(const char* p, int nSize)
{
    bool bZeroFound (false);
    int i (0);

    for (; i < nSize; ++i)
    {
        if (0 == p[i])
        {
            bZeroFound = true;
            break;
        }
        if (!isLegal(p[i])) { return BAD; }
    }

    if (bZeroFound)
    {
        bool bMixed (false);
        for (; i < nSize; ++i)
        {
            if (' ' == p[i]) { bMixed = true; }
            if (0 != p[i] && ' ' != p[i]) { return BAD; }
        }
        return bMixed ? MIXED_PADDED : ZERO_PADDED;
    }

    if (' ' == p[nSize - 1] && ' ' == p[nSize - 2]) { return SPACE_PADDED; }

    return NOT_PADDED;
}


/*override*/ void Id3V1Stream::copy(std::istream&, std::ostream& out)
{
    out.write(m_data, 128);
    CB_CHECK1 (out, WriteError());
}


static string getSpacedStr(const string& s)
{
    if (s.empty()) { return string(); }
    return ", " + s;
}


const char* Id3V1Stream::getVersion() const
{
    switch (m_eVersion)
    {
    case V10: return "ID3V1.0";
    case V11: return "ID3V1.1";
    case V11b: return "ID3V1.1b";
    default:
        STRM_ASSERT (false);
    }
}

/*override*/ std::string Id3V1Stream::getInfo() const
{
    string strRes (getVersion());
    strRes += getSpacedStr(getTitle(0)) + getSpacedStr(getArtist(0)) + getSpacedStr(getAlbumName(0)) + getSpacedStr(getGenre(0));
    return strRes;
}




/*override*/ TagReader::SuportLevel Id3V1Stream::getSupport(Feature eFeature) const
{
    switch (eFeature)
    {
    case TITLE:
    case ARTIST:
    case TRACK_NUMBER:
    case TIME:
    case GENRE:
    case ALBUM:
        return READ_ONLY;

    default:
        return NOT_SUPPORTED;
    }
}


// returns the string located at nAddr, removing trailing spaces; the result is in UTF8 format
string Id3V1Stream::getStr(int nAddr, int nMaxSize) const
{
    int nSize (0);
    for (; nSize < nMaxSize; ++nSize)
    {
        unsigned char c ((unsigned char)(m_data[nAddr + nSize]));
        if (c < ' ') { break; } // ASCII-specific, but probably OK for dealing with MP3 tags
    }

    for (; nSize > 0 && ' ' == m_data[nAddr + nSize - 1]; --nSize) {}

    return utf8FromLatin1(string(m_data + nAddr, m_data + nAddr + nSize));
}


/*override*/ std::string Id3V1Stream::getTitle(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    return getStr(3, 30);
}


/*override*/ std::string Id3V1Stream::getArtist(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    return getStr(33, 30);
}


/*override*/ std::string Id3V1Stream::getTrackNumber(bool* pbFrameExists /*= 0*/) const
{
    if (V11b != m_eVersion) { if (0 != pbFrameExists) { *pbFrameExists = false; } return ""; }
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    char a [10];
    sprintf(a, "%02d", (int)(unsigned char)(m_data[126]));
    return a;
}


/*override*/ std::string Id3V1Stream::getGenre(bool* pbFrameExists /*= 0*/) const
{
    switch (m_eVersion)
    {
    case V11:
    case V11b:
        {
            int n (m_data[127]);
            if (0 != pbFrameExists) { *pbFrameExists = true; }
            return getId3V1Genre(n);
        }

    default:
        if (0 != pbFrameExists) { *pbFrameExists = false; }
        return "";
    }
}


/*override*/ std::string Id3V1Stream::getAlbumName(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    return getStr(63, 30);
}


/*override*/ TagTimestamp Id3V1Stream::getTime(bool* pbFrameExists /*= 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    string s (getStr(93, 4));
    if ("    " == s || string("\0\0\0\0") == s) { return TagTimestamp(""); }
    try
    {
        return TagTimestamp(s);
    }
    catch (const TagTimestamp::InvalidTime&)
    {
        return TagTimestamp("");
    }
}


int Id3V1Stream::getCommSize() const
{
    switch (m_eVersion)
    {
    case V10: return 31;
    case V11: return 30;
    case V11b: return 28;
    }
    return 0;
}


/*override*/ std::string Id3V1Stream::getOtherInfo() const
{
    int nCommSize (getCommSize());
    string strComm (getStr(97, nCommSize));
    if (strComm.empty()) { return ""; }
    return "Comment: " + strComm;
}


const char* getId3V1Genre(int n)
{
    if (n <= 0 || n > 147) { return ""; }

    static const char* aGenres [148] =
    {
        "Blues",
        "Classic Rock",
        "Country",
        "Dance",
        "Disco",
        "Funk",
        "Grunge",
        "Hip-Hop",
        "Jazz",
        "Metal",
        "New Age",
        "Oldies",
        "Other",
        "Pop",
        "R&B",
        "Rap",
        "Reggae",
        "Rock",
        "Techno",
        "Industrial",
        "Alternative",
        "Ska",
        "Death Metal",
        "Prank",
        "Soundtrack",
        "Euro-Techno",
        "Ambient",
        "Trip-Hop",
        "Vocal",
        "Jazz+Funk",
        "Fusion",
        "Trance",

        //20 - 3F
        "Classical",
        "Instrumental",
        "Acid",
        "House",
        "Game",
        "Sound Clip",
        "Gospel",
        "Noise",
        "Alternative Rock",
        "Bass",
        "Soul",
        "Punk",
        "Space",
        "Meditative",
        "Instrumental Pop",
        "Instrumental Rock",
        "Ethnic",
        "Gothic",
        "Darkwave",
        "Techno-Industrial",
        "Electronic",
        "Pop-Folk",
        "Eurodance",
        "Dream",
        "Southern Rock",
        "Comedy",
        "Cult",
        "Gangsta",
        "Top 40",
        "Christian Rap",
        "Pop/Funk",
        "Jungle",

        //40 - 5F
        "Native US",
        "Cabaret",
        "New Wave",
        "Psychedelic", // ??? "Psychadelic" in the specs
        "Rave",
        "Showtunes",
        "Trailer",
        "Lo-Fi",
        "Tribal",
        "Acid Punk",
        "Acid Jazz",
        "Polka",
        "Retro",
        "Musical",
        "Rock & Roll",
        "Hard Rock",
        "Folk",
        "Folk-Rock",
        "National Folk",
        "Swing",
        "Fast Fusion",
        "Bebop",
        "Latin",
        "Revival",
        "Celtic",
        "Bluegrass",
        "Avantgarde",
        "Gothic Rock",
        "Progressive Rock",
        "Psychedelic Rock",
        "Symphonic Rock",
        "Slow Rock",

        //60 - 7F
        "Big Band",
        "Chorus",
        "Easy Listening",
        "Acoustic",
        "Humour",
        "Speech",
        "Chanson",
        "Opera",
        "Chamber Music",
        "Sonata",
        "Symphony",
        "Booty Bass",
        "Primus",
        "Porn Groove",
        "Satire",
        "Slow Jam",
        "Club",
        "Tango",
        "Samba",
        "Folklore",
        "Ballad",
        "Power Ballad",
        "Rhytmic Soul",
        "Freestyle",
        "Duet",
        "Punk Rock",
        "Drum Solo",
        "A capella",
        "Euro-House",
        "Dance Hall",
        "Goa",
        "Drum & Bass",

        //80 - 93
        "Club-House",
        "Hardcore",
        "Terror",
        "Indie",
        "BritPop",
        "Negerpunk",
        "Polsk Punk",
        "Beat",
        "Christian Gangsta Rap",
        "Heavy Metal",
        "Black Metal",
        "Crossover",
        "Contemporary Christian",
        "Christian Rock",
        "Merengue",
        "Salsa",
        "Trash Meta",
        "Anime",
        "Jpop",
        "Synthpop"
    };

    return aGenres[n];
}

//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================

//ttt1 GC - Stillness & Crafted Prayer 1.mp3 - audio present but not detected, because it's MPEG 2.5


