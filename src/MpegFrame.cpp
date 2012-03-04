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


#include  <istream>
#include  <sstream>

#include  "MpegFrame.h"

#include  "DataStream.h"
#include  "Helpers.h"
#include  "Notes.h"
#include  "Widgets.h"  // for GlobalTranslHlp


using namespace std;
//using namespace pearl;



// based on http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm

MpegFrameBase::MpegFrameBase(NoteColl& notes, istream& in)
{
    m_pos = in.tellg();

    const int BFR_SIZE (4);
    char bfr [BFR_SIZE];
    MP3_CHECK_T (BFR_SIZE == read(in, bfr, BFR_SIZE), m_pos, "Not an MPEG frame. File too short.", NotMpegFrame());
    init(notes, bfr);
}


MpegFrameBase::MpegFrameBase(NoteColl& notes, streampos pos, const char* bfr)
{
    m_pos = pos;
    init(notes, bfr);
}


#ifdef GENERATE_TOC
MpegFrameBase MpegFrameBase::getBigBps() const // returns a "similar" frame to "this" but with a bigger bps, so it can hold a Xing TOC
{
    //char bfr [1000];
    string s (&m_header[0], 4);
    //s[2] = (s[2] & 0x0f) | 0xe0; //ttt2 maybe use this; it provides maximum space;
    s[2] = (s[2] & 0x0f) | 0x90;
    s.resize(2000);
    NoteColl notes;
    istringstream in (s);
    MpegFrameBase res (notes, in);
    return res;
}
#endif

void MpegFrameBase::init(NoteColl& notes, const char* bfr) //ttt2 should have some means to enforce bfr being large enough (perhaps switch to a vector)
{
    memcpy(m_header, bfr, 4);
//inspect(bfr, BFR_SIZE);
    //ttt2 check the CRC right after the header, if present (note that the size of a frame doesn't change as a result of using the CRC, as it can be seen in several files, e.g. "05 Are You Gonna Go My Way.mp3")
    const unsigned char* pHeader (reinterpret_cast<const unsigned char*>(bfr));
    MP3_CHECK_T (0xff == *pHeader && 0xe0 == (0xe0 & *(pHeader + 1)), m_pos, "Not an MPEG frame. Synch missing.", NotMpegFrame()/*"missing synch"*/);
    ++pHeader;

    {
        int nVer ((*pHeader & 0x18) >> 3);
        switch (nVer)
        {//TRACE
        case 0x00: MP3_THROW_T (m_pos, "Not an MPEG frame. Unsupported version (2.5).", NotMpegFrame()); //ttt2 see about supporting this: search for MPEG1 to find other places
            // ttt2 in a way it would make more sense to warn that it's not supported, with "MP3_THROW(SUPPORT, ...)", but before warn, make sure it's a valid 2.5 frame, followed by another frame ...

        case 0x02: m_eVersion = MPEG2; break;
        case 0x03: m_eVersion = MPEG1; break;

        default: MP3_THROW_T (m_pos, "Not an MPEG frame. Invalid version.", NotMpegFrame());
        }
    }

    {
        int nLayer ((*pHeader & 0x06) >> 1);
        switch (nLayer)
        {
        case 0x01: m_eLayer = LAYER3; break;
        case 0x02: m_eLayer = LAYER2; break;
        case 0x03: m_eLayer = LAYER1; break;

        default: MP3_THROW_T (m_pos, "Not an MPEG frame. Invalid layer.", NotMpegFrame());
        }
    }

    {
        m_bCrc = !(*pHeader & 0x01);
    }

    ++pHeader;
    {
        static int s_bitrates [14][5] =
            {
                {  32,      32,      32,      32,       8 },
                {  64,      48,      40,      48,      16 },
                {  96,      56,      48,      56,      24 },
                { 128,      64,      56,      64,      32 },
                { 160,      80,      64,      80,      40 },
                { 192,      96,      80,      96,      48 },
                { 224,     112,      96,     112,      56 },
                { 256,     128,     112,     128,      64 },
                { 288,     160,     128,     144,      80 },
                { 320,     192,     160,     160,      96 },
                { 352,     224,     192,     176,     112 },
                { 384,     256,     224,     192,     128 },
                { 416,     320,     256,     224,     144 },
                { 448,     384,     320,     256,     160 }
            };
        int nRateIndex ((*pHeader & 0xf0) >> 4);
        MP3_CHECK_T (nRateIndex >= 1 && nRateIndex <= 14, m_pos, "Not an MPEG frame. Invalid bitrate.", NotMpegFrame()/*"invalid bitrate"*/); //ttt3 add tests for invalid combinations of channel mode and bitrate (MPEG 1 Layer II only)
        int nTypeIndex (m_eVersion*3 + m_eLayer);
        if (nTypeIndex == 5) { nTypeIndex = 4; }
        m_nBitrate = s_bitrates[nRateIndex - 1][nTypeIndex]*1000;
    }

    {
        int nSmpl ((*pHeader & 0x0c) >> 2);
        switch (m_eVersion)
        {
        case MPEG1:
            switch (nSmpl)
            {
            case 0x00: m_nFrequency = 44100; break;
            case 0x01: m_nFrequency = 48000; break;
            case 0x02: m_nFrequency = 32000; break;

            default: MP3_THROW_T (m_pos, "Not an MPEG frame. Invalid frequency for MPEG1.", NotMpegFrame());
            }
            break;

        case MPEG2:
            switch (nSmpl)
            {
            case 0x00: m_nFrequency = 22050; break;
            case 0x01: m_nFrequency = 24000; break;
            case 0x02: m_nFrequency = 16000; break;

            default: MP3_THROW_T (m_pos, "Not an MPEG frame. Invalid frequency for MPEG2.", NotMpegFrame());
            }
            break;

        default: CB_ASSERT(false); // it should have thrown before getting here
        }
    }

    {
        m_nPadding = (0x02 & *pHeader) >> 1;
    }

    ++pHeader;
    {
        int nChMode ((*pHeader & 0xc0) >> 6);
        m_eChannelMode = (ChannelMode)nChMode;
    }

    switch (m_eLayer)
    {
    case LAYER1:
        m_nSize = (12*m_nBitrate/m_nFrequency + m_nPadding)*4;
        break;

    case LAYER2:
        m_nSize = 144*m_nBitrate/m_nFrequency + m_nPadding;
        break;

    case LAYER3:
//cout << "m_nBitrate:    " << m_nBitrate << endl;
//cout << "m_nFrequency: " << m_nFrequency << endl;
//cout << "m_nPadding:    " << m_nPadding << endl;
        m_nSize = (MPEG1 == m_eVersion ? 144*m_nBitrate/m_nFrequency + m_nPadding : 72*m_nBitrate/m_nFrequency + m_nPadding);
        //MP3_CHECK (MPEG1 == m_eVersion, m_pos, "Temporary test for MPEG2. Remove after making sure the code works.", NotMpegFrame()/*"temporary"*/); // see http://www.codeproject.com/KB/audio-video/mpegaudioinfo.aspx
        break;

    default: CB_ASSERT(false); // it should have thrown before getting here
    }
}


MpegFrameBase::MpegFrameBase() : m_eVersion(MPEG1), m_eLayer(LAYER1), m_nBitrate(-1), m_nFrequency(-1), m_nPadding(-1), m_eChannelMode(SINGLE_CHANNEL), m_nSize(-1), m_bCrc(false) {}


const char* MpegFrameBase::getSzVersion() const
{
    static const char* s_versionName[] = { QT_TRANSLATE_NOOP("DataStream", "MPEG-1"), QT_TRANSLATE_NOOP("DataStream", "MPEG-2") };
    return s_versionName[m_eVersion];
}

const char* MpegFrameBase::getSzLayer() const
{
    static const char* s_layerName[] = { QT_TRANSLATE_NOOP("DataStream", "Layer I"), QT_TRANSLATE_NOOP("DataStream", "Layer II"), QT_TRANSLATE_NOOP("DataStream", "Layer III") };
    return s_layerName[m_eLayer];
}

const char* MpegFrameBase::getSzChannelMode() const
{
    static const char* s_channelModeName[] = { QT_TRANSLATE_NOOP("DataStream", "Stereo"), QT_TRANSLATE_NOOP("DataStream", "Joint stereo"), QT_TRANSLATE_NOOP("DataStream", "Dual channel"), QT_TRANSLATE_NOOP("DataStream", "Single channel") };
    return s_channelModeName[m_eChannelMode];
}

ostream& MpegFrameBase::write(ostream& out) const
{
    out << convStr(DataStream::tr(getSzVersion())) << " " << convStr(DataStream::tr(getSzLayer())) << ", " << convStr(DataStream::tr(getSzChannelMode())) << ", " << m_nFrequency << "Hz, " <<
            m_nBitrate << "bps, CRC=" << convStr(GlobalTranslHlp::tr(boolAsYesNo(m_bCrc))) << ", " << convStr(DataStream::tr("length=")) <<
            m_nSize << " (0x" << hex << m_nSize << dec << "), " << convStr(DataStream::tr("padding=")) << convStr(GlobalTranslHlp::tr(boolAsYesNo(m_nPadding)));

    return out;
}


ostream& operator<<(ostream& out, const MpegFrameBase& frm)
{
    return frm.write(out);
}

int MpegFrameBase::getSideInfoSize() const // needed by Xing
{
    return MpegFrame::MPEG1 == getVersion() ?
        (MpegFrame::SINGLE_CHANNEL == getChannelMode() ? 17 : 32) :
        (MpegFrame::SINGLE_CHANNEL == getChannelMode() ? 9 : 17);
}


MpegFrame::MpegFrame(NoteColl& notes, std::istream& in) : MpegFrameBase(notes, in)
{
    streampos pos (m_pos);
    StreamStateRestorer rst (in);

    pos += m_nSize - 1;
    in.seekg(pos);
    char bfr [1];
    if (1 != read(in, bfr, 1))
    {
        ostringstream out;
        write(out);
        MP3_THROW_T (pos, "Not an MPEG frame. File too short.", PrematurelyEndedMpegFrame(out.str()));
    }

    rst.setOk();
}



