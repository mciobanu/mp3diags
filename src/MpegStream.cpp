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

#if defined(GAPLESS_SUPPORT)
#include  <lame/lame.h>
#endif

#include  "MpegStream.h"
#include  "Mp3Manip.h"
#include  "Widgets.h"  // for GlobalTranslHlp


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
        MP3_THROW (m_pos, audioTooShort, CB_EXCP2(StreamTooShort, strInfo, m_nFrameCount));
    }

    MP3_CHECK (!m_bVbr || bVbr2, m_pos, diffBitrateInFirstFrame, CB_EXCP(UnknownHeader)); //ttt2 perhaps add test for "null": whatever is in the first bytes that allows Xing & Co to not generate audio in decoders that don't know about them

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
        convStr(GlobalTranslHlp::tr(boolAsYesNo(m_firstFrame.getCrcUsage()))) << ", " << convStr(DataStream::tr("frame count=")) << m_nFrameCount;
    out << "; " << convStr((m_bRemoveLastFrameCalled ? DataStream::tr("last frame removed; it was located at 0x%1") : DataStream::tr("last frame located at 0x%1")).arg(m_posLastFrame, 0, 16));
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
#if 1
// throws if it can't write to the disk
void createXing(const string& /*strFileName*/, streampos /*nStreamPos*/, ostream& out, const MpegFrame& frame1, int nFrameCount, streamoff nStreamSize)
{
    const MpegFrameBase& frame (frame1.getBigBps());

    int nSize (frame.getSize());
    out.write(frame.getHeader(), MpegFrame::MPEG_FRAME_HDR_SIZE);
    int nSideInfoSize (frame.getSideInfoSize());
    writeZeros(out, nSideInfoSize);
    out.write("Xing\0\0\0\x07", 8);
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
    CB_CHECK (out, WriteError);
}
#else // changes made while playing with gapless; later it turned out that various fields that have values copied from some file in the code below can actually be left as zero (at least LAME decodes them and players play them)
void createXing(const string& /*strFileName*/, streampos /*nStreamPos*/, ostream& out, const MpegFrame& frame1, int nFrameCount, streamoff nStreamSize)
{
    const MpegFrameBase& frame (frame1.getBigBps());

    int nSize (frame.getSize());
    out.write(frame.getHeader(), MpegFrame::MPEG_FRAME_HDR_SIZE);
    int nSideInfoSize (frame.getSideInfoSize());
    writeZeros(out, nSideInfoSize);
    out.write("Xing\0\0\0\x0f", 8);
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
    //writeZeros(out, nSize - MpegFrame::MPEG_FRAME_HDR_SIZE - nSideInfoSize - 8 - 4 - 4 - 100);

    out.write("\0\0\0:", 4);
    out.write("LAME3.92 ", 9);
    out.write("\x02\xbb", 2);
    writeZeros(out, 8);
    out.write("\x02\xc0", 2);
    //out.write("\x24\x04\x22", 3);
    out.write("\x94\x09\x22", 3);
    //writeZeros(out, nSize - MpegFrame::MPEG_FRAME_HDR_SIZE - nSideInfoSize - 8 - 4 - 4 - 100 - 4 - 9 - 2 - 8 - 2 - 3);

    out.write("\x45\0\0\0\0\x90\x72\x56\x3e\x5e\x58\xd2", 12);

    writeZeros(out, nSize - MpegFrame::MPEG_FRAME_HDR_SIZE - nSideInfoSize - 8 - 4 - 4 - 100 - 4 - 9 - 2 - 8 - 2 - 3 - 12);
    CB_CHECK (out, WriteError);
}

#endif

#elif defined(GAPLESS_SUPPORT)



namespace {

const int MPEG_SAMPLES_PER_FRAME (1152);
const int CD_SAMPLES_PER_FRAME (588);
const int MPEG_BFR_SIZE (65536);

DEFINE_CB_EXCP(DecodeError);
DEFINE_CB_EXCP(ZeroesNotFound);


struct MpegFrameBfr : public MpegFrameBase
{
    MpegFrameBfr(NoteColl& notes, const char* bfr) : MpegFrameBase(notes, 0, bfr) {}
};

class Mp3Decoder
{
    vector<unsigned char> m_vcMp3Bfr;
    struct LameResourceWrapper
    {
        hip_t m_pHip;
        LameResourceWrapper()
        {
            m_pHip = hip_decode_init();
        }

        ~LameResourceWrapper()
        {
            hip_decode_exit(m_pHip);
        }
    };

    void decode3(istream& in, streamoff nStreamSize);

public:
    Mp3Decoder() : m_vcMp3Bfr(MPEG_BFR_SIZE) {}

    // tries to fill the vectors with as many samples as fit, but it will shrink the vectors if there are not enough samples; returns the total sample count
    int decode(istream& in, streamoff nStreamSize, vector<int16_t>& vSamplesBeginLeft, vector<int16_t>& vSamplesBeginRight, vector<int16_t>& vSamplesEndLeft, vector<int16_t>& vSamplesEndRight, int& globalMax);
};


void Mp3Decoder::decode3(istream& in, streamoff nStreamSize) {
    const int MPEG_BFR_SIZE (2000);
    vector<unsigned char> bfr (MPEG_BFR_SIZE);
    vector<short> pcmBfrL (MPEG_BFR_SIZE*100);
    vector<short> pcmBfrR (MPEG_BFR_SIZE*100);
    ofstream outL ("cpp-out-d3-l.pcm"); //ttt3 improve
    ofstream outR ("cpp-out-d3-r.pcm");
    LameResourceWrapper lameWrp;
    StreamStateRestorer rst (in);
    const int HDR_SIZE (4);
    while (nStreamSize > 0) {
        streamsize nRead (read(in, (char*)(&bfr[0]), HDR_SIZE));
        NoteColl noteColl;
        MpegFrameBfr frame (noteColl, (const char*)&bfr[0]);
        int frameSize (frame.getSize());
        nRead = read(in, (char*)(&bfr[HDR_SIZE]), frameSize - HDR_SIZE);
        nRead += HDR_SIZE;
        nStreamSize -= nRead;
        CB_CHECK(nRead == frameSize || nStreamSize == 0, DecodeError);
        int nSamples (hip_decode(lameWrp.m_pHip, &bfr[0], frameSize, &pcmBfrL[0], &pcmBfrR[0]));
        CB_CHECK(nSamples >= 0, DecodeError);
        if (nSamples > 0) {
            outL.write((const char*)(&pcmBfrL[0]), nSamples * 2);
            outR.write((const char*)(&pcmBfrR[0]), nSamples * 2);
        }
    }
}


/**
 * @return number of samples; -1 for fatal error
 */
int Mp3Decoder::decode(istream& in, streamoff nStreamSize, vector<int16_t>& vSamplesBeginLeft, vector<int16_t>& vSamplesBeginRight, vector<int16_t>& vSamplesEndLeft, vector<int16_t>& vSamplesEndRight, int& globalMax) {

    //decode3(in, nStreamSize);
    globalMax = 0;
    CB_CHECK (vSamplesBeginLeft.size() == vSamplesBeginRight.size() && vSamplesEndLeft.size() == vSamplesEndRight.size(), DecodeError);
    vector<int16_t> vLeftPcmBuffer (MPEG_SAMPLES_PER_FRAME * 2000);
    vector<int16_t> vRightPcmBuffer (MPEG_SAMPLES_PER_FRAME * 2000);
    uint nPcmBufferSampleCount (0); // number of samples in the PCM buffers
    uint nPcmBufferSampleIndex (0); // index of first sample in the PCM buffers
    const int MAX_FRAME_SIZE (320000*144/8000 + 4); // this is probably more than needed, as not all combinations are valid; 320000=max bitrate; 144=max multiplying constant; 8000=min frequency
    const int MIN_FRAME_SIZE (40);

    //int nSyncPos (0);
    int nMp3BfrOffset (0);
    int nMp3BfrFirstFree (0); // reading buffer starts here, as the beginning is usually taken by the truncated end of the previous buffer
    //bool bFirst (true);
    int nTotalSamples (0);
    int nCrtFrameAddr (0);
    LameResourceWrapper lameWrp;
    bool bCopiedFirst (false);
    int crtFrame (0);
    while (nStreamSize > 0)
    {
        streamsize nToRead (min((size_t)nStreamSize, m_vcMp3Bfr.size() - nMp3BfrFirstFree));
        streamsize nRead (read(in, (char*)(&m_vcMp3Bfr[nMp3BfrFirstFree]), nToRead));
        CB_CHECK (nToRead == nRead, DecodeError);
        nStreamSize -= nRead;
        CB_CHECK(nStreamSize == 0 || nRead > 0, DecodeError);
        for (;;)
        {
            NoteColl noteColl;
            MpegFrameBfr frame (noteColl, (const char*)&m_vcMp3Bfr[nCrtFrameAddr]);

            if (nCrtFrameAddr + frame.getSize() > cSize(m_vcMp3Bfr))
            { //we don't have the whole frame
                break;
            }
            //qDebug("frame at global offset %d (%x), buffer offset %d; size %d", nMp3BfrOffset + nCrtFrameAddr, nMp3BfrOffset + nCrtFrameAddr, nCrtFrameAddr, frame.getSize());
            int nSamples (hip_decode(lameWrp.m_pHip, &m_vcMp3Bfr[nCrtFrameAddr], frame.getSize(), &vLeftPcmBuffer[nPcmBufferSampleCount], &vRightPcmBuffer[nPcmBufferSampleCount]));
            ++crtFrame;
            if (crtFrame++ >= 11483) {
                //qDebug("crtFrame=%d", crtFrame);
            }
            for (int i = 0; i < nSamples; ++i)
            {
                globalMax = max(max(globalMax, abs((int)vLeftPcmBuffer[nPcmBufferSampleCount + i])), abs((int)vRightPcmBuffer[nPcmBufferSampleCount + i]));
            }

            CB_CHECK(nSamples >= 0, DecodeError);
            nPcmBufferSampleCount += nSamples;

            if (!bCopiedFirst && nPcmBufferSampleCount >= vSamplesBeginLeft.size())
            {
                bCopiedFirst = true;
                copy(vLeftPcmBuffer.begin(), vLeftPcmBuffer.begin() + vSamplesBeginLeft.size(), vSamplesBeginLeft.begin());
                copy(vRightPcmBuffer.begin(), vRightPcmBuffer.begin() + vSamplesBeginRight.size(), vSamplesBeginRight.begin());
            }

            if (nPcmBufferSampleCount > vLeftPcmBuffer.size() - 10*MPEG_SAMPLES_PER_FRAME) {
                uint nDiscard (nPcmBufferSampleCount - vSamplesEndLeft.size());
                copy(vLeftPcmBuffer.begin() + nDiscard, vLeftPcmBuffer.begin() + nPcmBufferSampleCount, vLeftPcmBuffer.begin());
                copy(vRightPcmBuffer.begin() + nDiscard, vRightPcmBuffer.begin() + nPcmBufferSampleCount, vRightPcmBuffer.begin());
                nPcmBufferSampleIndex += nDiscard;
                nPcmBufferSampleCount = vSamplesEndLeft.size();
            }

            nTotalSamples += nSamples;
            if (nSamples != MPEG_SAMPLES_PER_FRAME)
            {
                //qDebug("frame at global offset %d (%x), buffer offset %d; size %d; decoded %d samples", nMp3BfrOffset + nCrtFrameAddr, nMp3BfrOffset + nCrtFrameAddr, nCrtFrameAddr, frame.getSize(), nSamples);
            }
            nCrtFrameAddr += frame.getSize();
            if ((nMp3BfrFirstFree + nRead) - nCrtFrameAddr <= MIN_FRAME_SIZE)
            { // we need more data for sure
                break;
            }
        }
        if (nStreamSize > 0)
        { // more data should be read; move the frame fragment at the end to the beginning and continue
            nMp3BfrFirstFree = m_vcMp3Bfr.size() - nCrtFrameAddr;
            CB_CHECK (nMp3BfrFirstFree <= MAX_FRAME_SIZE, DecodeError);
            copy(m_vcMp3Bfr.begin() + nCrtFrameAddr, m_vcMp3Bfr.end(), m_vcMp3Bfr.begin());
            nCrtFrameAddr = 0;
            nMp3BfrOffset += m_vcMp3Bfr.size() - nMp3BfrFirstFree;
        }
    }

    CB_CHECK (bCopiedFirst, DecodeError);
    CB_CHECK (nPcmBufferSampleCount >= vSamplesEndLeft.size(), DecodeError);
    copy(vLeftPcmBuffer.begin() + nPcmBufferSampleCount - vSamplesEndLeft.size(), vLeftPcmBuffer.begin() + nPcmBufferSampleCount, vSamplesEndLeft.begin());
    copy(vRightPcmBuffer.begin() + nPcmBufferSampleCount - vSamplesEndRight.size(), vRightPcmBuffer.begin() + nPcmBufferSampleCount, vSamplesEndRight.begin());

    //qDebug("total samples %d", nTotalSamples);
    return nTotalSamples;
}


const int QUIET_THRESHOLD (100);

bool isQuiet(const vector<short>& v) {
    for (uint i = 0; i < v.size(); ++i) {
        if (abs(v[i]) > QUIET_THRESHOLD) {
            return false;
        }
    }
    return true;
}



int findZeroesByAverages(const vector<short>& v, double threshold) {

    uint n (100);
    long total (0);
    for (uint i = 0; i < v.size(); ++i) {
        total += abs(v[i]);
    }
    int globalContrib (2 * n * total / v.size());
    int nRes (-1);

    for (uint i = 0; i < n; ++i) {
        //qDebug("0");
    }
    int sumBefore (0), sumAfter (0);
    for (uint i = 0; i < n; ++i) {
        sumBefore += abs(v[i]);
        sumAfter += abs(v[i + n]);
    }
    for (uint i = n; i < v.size() - n - 1; ++i)
    {
        double f (sumBefore / (1.0 + globalContrib + sumAfter));
        //qDebug("i=%d, val=%d, sumBefore=%d, sumAfter=%d, f=%f", i, v[i], sumBefore, sumAfter, f);
        //qDebug("%f", f);
        if (f > threshold && nRes == -1)
        {
            nRes = i;
        }
        sumBefore += abs(v[i]);
        sumBefore -= abs(v[i - n]);
        sumAfter  -= abs(v[i]);
        sumAfter  += abs(v[i + n]);
    }
    for (uint i = 0; i < n; ++i) {
        //qDebug("0");
    }

    return nRes;
}


// returns -1 if the limit cannot be found
int findZeroesByMax(const vector<short>& v, double threshold, int globalMax) {

    double maxVal (globalMax);
    //qDebug("maxVal=%f", maxVal);
    int nCnt (0);
    for (uint i = 0; i < v.size(); ++i)
    {
        if (abs(v[i]) / maxVal > threshold) {
            ++nCnt;
            if (nCnt > 5) {
                //qDebug("threshold %f, result %d", threshold, i);
                return i;
            }
        }
    }

    //qDebug("threshold %f, result %d", threshold, -1);
    return -1;
}

//just for tests
int findZeroesByMaxHlp(const vector<short>& v, int globalMax) {
    for (double t = 0.001; t < 0.2; t *= 1.2) {
        findZeroesByMax(v, t, globalMax);
    }
    //return findZeroesHlp(v, 0.03);
    return findZeroesByMax(v, 0.08, globalMax);
}

bool approxEq(int a, int b, int limit)
{
    return abs(a - b) < limit;
}

int computeAvg(vector<int> v) {
    sort(v.begin(), v.end());
    if (v[0] == -1) {
        return -1;
    }
    if (v[0] + 400 < v[v.size() - 1]) { // difference is too big
        return -1;
    }
    int sum (0);
    for (uint i = 1; i < v.size() - 1; ++i) {
        sum += v[i];
    }
    return sum / (v.size() - 2);
}

int computeZeroes(vector<int16_t>& v, short globalMax) {
    vector<int> zMax, zAvg;
    zMax.push_back(findZeroesByMax(v, 0.03, globalMax));
    zMax.push_back(findZeroesByMax(v, 0.04, globalMax));
    zMax.push_back(findZeroesByMax(v, 0.08, globalMax));
    zMax.push_back(findZeroesByMax(v, 0.15, globalMax));
    zAvg.push_back(findZeroesByAverages(v, 0.1));
    zAvg.push_back(findZeroesByAverages(v, 0.15));
    zAvg.push_back(findZeroesByAverages(v, 0.2));
    zAvg.push_back(findZeroesByAverages(v, 0.3));
    //qDebug("----------------------");
    //qDebug("%d,%d,%d,%d,%d,%d,%d,%d", zMax[0], zMax[1], zMax[2], zMax[3], zAvg[0], zAvg[1], zAvg[2], zAvg[3]);

    int resMax (computeAvg(zMax)), resAvg(computeAvg(zAvg));
    if (resMax == -1) {
        return resAvg;
    }
    if (resAvg == -1) {
        return resMax;
    }
    return min(resMax, resAvg);
}



void getDelayAndPadding(istream& in, streamoff nStreamSize, unsigned& nDelay, unsigned& nPadding)
{
    Mp3Decoder dec;
    int globalMax;
    vector<int16_t> vSamplesBeginLeft (0xfff), vSamplesBeginRight (0xfff), vSamplesEndLeft (0xfff), vSamplesEndRight (0xfff);
    int nTotalSamples (dec.decode(in, nStreamSize, vSamplesBeginLeft, vSamplesBeginRight, vSamplesEndLeft, vSamplesEndRight, globalMax));

    reverse(vSamplesEndLeft.begin(), vSamplesEndLeft.end());
    reverse(vSamplesEndRight.begin(), vSamplesEndRight.end());

    const int FRONT_ZEROES = 1105;
    const int LAME_DELAY = 529;

    int nDelay1 (-1);
    if (!isQuiet(vSamplesBeginLeft)) {
        nDelay1 = computeZeroes(vSamplesBeginLeft, globalMax);
        CB_CHECK (nDelay1 == -1 || nDelay1 >= FRONT_ZEROES - 180, ZeroesNotFound); // the first 1105 samples were supposed to be zero
    }

    int nDelay2 (-1);
    if (!isQuiet(vSamplesBeginRight)) {
        nDelay2 = computeZeroes(vSamplesBeginRight, globalMax);
        CB_CHECK (nDelay2 == -1 || nDelay2 >= FRONT_ZEROES - 180, ZeroesNotFound);
    }

    nDelay = FRONT_ZEROES - LAME_DELAY;

    int nPadding1 (-1), nRoundedPadding1 (-1);
    if (!isQuiet(vSamplesEndLeft)) {
        nPadding1 = computeZeroes(vSamplesEndLeft, globalMax);
        if (nPadding1 != -1) {
            int nNonZeroSamples1 (nTotalSamples - FRONT_ZEROES - nPadding1);
            nNonZeroSamples1 = (nNonZeroSamples1 + CD_SAMPLES_PER_FRAME/2) / CD_SAMPLES_PER_FRAME * CD_SAMPLES_PER_FRAME;
            nRoundedPadding1 = nTotalSamples - FRONT_ZEROES - nNonZeroSamples1;
            //CB_CHECK (approxEq(nRoundedPadding1, nPadding1, 150), ZeroesNotFound); //ttt2 maybe put this back and do more testing; the thing is it is possible that there is no fadeout and the sound really stops 2000 samples before the end, with no relation to the CD frames
            if (!approxEq(nRoundedPadding1, nPadding1, 150)) {
                nRoundedPadding1 = -1;
            }
        }
    }

    int nPadding2 (-1), nRoundedPadding2 (-1);
    if (!isQuiet(vSamplesEndRight)) {
        nPadding2 = computeZeroes(vSamplesEndRight, globalMax);
        if (nPadding2 != -1) {
            int nNonZeroSamples2 (nTotalSamples - FRONT_ZEROES - nPadding2);
            nNonZeroSamples2 = (nNonZeroSamples2 + CD_SAMPLES_PER_FRAME/2) / CD_SAMPLES_PER_FRAME * CD_SAMPLES_PER_FRAME;
            nRoundedPadding2 = nTotalSamples - FRONT_ZEROES - nNonZeroSamples2;
            //CB_CHECK (approxEq(nRoundedPadding2, nPadding2, 150), ZeroesNotFound);
            if (!approxEq(nRoundedPadding2, nPadding2, 150)) {
                nRoundedPadding2 = -1;
            }
        }
    }

    //qDebug("bytesProcessed=%ld, nTotalSamples=%d, nDelay1=%d, nDelay2=%d, nPadding1=%d, nPadding2=%d, nRoundedPadding1=%d, nRoundedPadding2=%d", nStreamSize, nTotalSamples, nDelay1, nDelay2, nPadding1, nPadding2, nRoundedPadding1, nRoundedPadding2);

    if (nRoundedPadding1 == -1) {
        if (nRoundedPadding2 == -1) {
            int nNonZeroSamples (nTotalSamples - FRONT_ZEROES - 600); // "600" just to have something; since it's quite, the listener won't notice even if it's wrong
            nNonZeroSamples = (nNonZeroSamples + CD_SAMPLES_PER_FRAME/2) / CD_SAMPLES_PER_FRAME * CD_SAMPLES_PER_FRAME;
            int nRoundedPadding (nTotalSamples - FRONT_ZEROES - nNonZeroSamples);
            nPadding = nRoundedPadding + LAME_DELAY;
        } else {
            nPadding = nRoundedPadding2 + LAME_DELAY;
        }
    } else {
        //CB_CHECK (nRoundedPadding1 == nRoundedPadding2, ZeroesNotFound);
        nPadding = min(nRoundedPadding1, nRoundedPadding2) + LAME_DELAY;
    }

}

} // namespace{



void createXing(const string& strFileName, streampos nStreamPos, ostream& out, const MpegFrame& frame, int nFrameCount, streamoff nStreamSize)
{
    //qDebug("-------------------- %s -----------------", strFileName.c_str());
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
    //writeZeros(out, nSize - MpegFrame::MPEG_FRAME_HDR_SIZE - nSideInfoSize - 8 - 4 - 4);

    out.write("LAME3.92 ", 9);
    out.write("\x0\x0", 2);
    writeZeros(out, 8);
    out.write("\x0\x0", 2);
    //out.write("\x24\x04\x22", 3);
    //out.write("\x24\x06\xb3", 3);

    ifstream_utf8 in (strFileName.c_str(), ios::binary);
    in.seekg(nStreamPos);

    unsigned nDelay, nPadding;
    getDelayAndPadding(in, nStreamSize, nDelay, nPadding);
    //qDebug("delay=%d, padding=%d", nDelay, nPadding);
//nDelay -= 200;
//nPadding -= 644;
//qDebug("adjusted: delay=%d, padding=%d", nDelay, nPadding);
    bfr[0] = nDelay >> 4;
    bfr[1] = (nDelay << 4) ^ (nPadding >> 8);
    bfr[2] = nPadding;
    //out.write("\x24\x08\xc4", 3);
    out.write(bfr, 3);
    //out.write("\x45\0\0\0\0\x90\x72\x56\x3e\x5e\x58\xd2", 12); //ttt1 these should be included too
    writeZeros(out, nSize - MpegFrame::MPEG_FRAME_HDR_SIZE - nSideInfoSize - 8 - 4 - 4 - 9 - 2 - 8 - 2 - 3);

//CB_OLD_CHECK1 (false, DecodeError()); //ttt0

    //ttt0 catch exceptions ...
    CB_CHECK (out, WriteError);
}

#else // #ifdef GENERATE_TOC  / #elif defined(GAPLESS_SUPPORT)

void createXing(const string& /*strFileName*/, streampos /*nStreamPos*/, ostream& out, const MpegFrame& frame, int nFrameCount, streamoff nStreamSize)
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
    CB_CHECK (out, WriteError);
}
#endif

void MpegStream::createXing(const string& strFileName, std::streampos nStreamPos, ostream& out)
{
    static const int MIN_FRAME_SIZE (200); // ttt2 this 200 is arbitrary, but there's probably enough room for TOC
    if (m_firstFrame.getSize() >= MIN_FRAME_SIZE)
    {
        ::createXing(strFileName, nStreamPos, out, m_firstFrame, m_nFrameCount, getSize());
        return;
    }

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
            ::createXing(strFileName, nStreamPos, out, frame, m_nFrameCount, getSize());
            return;
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

    MP3_CHECK_T (nBfrSize <= m_firstFrame.getSize(), m_pos, "Not a Xing stream. This kind of MPEG audio doesn't support Xing.", CB_EXCP(NotXingStream)); // !!! some kinds of MPEG audio (e.g. "MPEG-1 Layer I, 44100Hz 32000bps" or "MPEG-2 Layer III, 22050Hz 8000bps") have very short frames, which can't accomodate a Xing header

    streamsize nRead (read(in, bfr, nBfrSize));
    STRM_ASSERT (nBfrSize == nRead); // this was supposed to be a valid frame to begin with (otherwise the base class would have thrown) and nBfrSize is no bigger than the frame

    char* pLabel (bfr + MpegFrame::MPEG_FRAME_HDR_SIZE + nSideInfoSize);
    MP3_CHECK_T (0 == strncmp("Xing", pLabel, XING_LABEL_SIZE) || 0 == strncmp("Info", pLabel, XING_LABEL_SIZE), m_pos, "Not a Xing stream. Header not found.", CB_EXCP(NotXingStream));

    // ttt0 perhaps if it gets this far it should generate some "broken xing": with the incorrect "vbr fix" which created a xing header longer than the mpeg frame that was supposed to contain it, followed by the removal of those extra bytes by the "unknown stream removal" causes the truncated xing header to be considered audio; that wouldn't happen if a "broken xing" stream would be tried before the "audio" stream in Mp3Handler::parse()
    MP3_CHECK_T (4 == read(in, bfr, 4) && 0 == bfr[0] && 0 == bfr[1] && 0 == bfr[2], m_pos, "Not a Xing stream. Header not found.", CB_EXCP(NotXingStream));
    m_cFlags = bfr[3];
    MP3_CHECK_T ((m_cFlags & 0x0f) == m_cFlags, m_pos, "Not a Xing stream. Invalid flags.", CB_EXCP(NotXingStream));
    if (0x01 == (m_cFlags & 0x01))
    { // has frames
        MP3_CHECK_T (4 == read(in, bfr, 4), m_pos, "Not a Xing stream. File too short.", CB_EXCP(NotXingStream));
        m_nFrameCount = get32BitBigEndian(bfr);
    }

    if (0x02 == (m_cFlags & 0x02))
    { // has bytes
        MP3_CHECK_T (4 == read(in, bfr, 4), m_pos, "Not a Xing stream. File too short.", CB_EXCP(NotXingStream));
        m_nByteCount = get32BitBigEndian(bfr);
    }

    if (0x04 == (m_cFlags & 0x04))
    { // has TOC
        MP3_CHECK_T (100 == read(in, m_toc, 100), m_pos, "Not a Xing stream. File too short.", CB_EXCP(NotXingStream));
    }

    if (0x08 == (m_cFlags & 0x08))
    { // has quality
        MP3_CHECK_T (4 == read(in, bfr, 4), m_pos, "Not a Xing stream. File too short.", CB_EXCP(NotXingStream));
        m_nQuality = get32BitBigEndian(bfr);
    }

    streampos posEnd (m_pos);
    posEnd += m_firstFrame.getSize();
    in.seekg(posEnd); //ttt2 2010.12.07 - A header claiming to have TOC but lacking one isn't detected. 1) Should check that values in TOC are ascending. 2) Should check that it actually fits: a 104 bytes-long 32kbps frame cannot hold a 100 bytes TOC along with the header and other things.
}


void XingStreamBase::getXingInfo(std::ostream& out) const
{
    out << "[" << convStr(DataStream::tr("Xing header info:"));
    bool b (false);
    if (0x01 == (m_cFlags & 0x01)) { out << convStr(DataStream::tr(" frame count=")) << m_nFrameCount; b = true; }
    if (0x02 == (m_cFlags & 0x02)) { out << (b ? "," : "") << convStr(DataStream::tr(" byte count=")) << m_nByteCount; b = true; } //ttt2 see what to do with this: it's the size of the whole file, all headers&tags included (at least with c03 Valentin Moldovan - Marea Irlandei.mp3); ??? and anyway,  what's the point of including the size of the whole file as a field?
    if (0x04 == (m_cFlags & 0x04)) { out << (b ? "," : "") << convStr(DataStream::tr(" TOC present")); b = true; }
    if (0x08 == (m_cFlags & 0x08)) { out << (b ? "," : "") << convStr(DataStream::tr(" quality=")) << m_nQuality; b = true; }
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
        << ", " << m_firstFrame.getBitrate() << "bps, CRC=" << convStr(GlobalTranslHlp::tr(boolAsYesNo(m_firstFrame.getCrcUsage()))) << "; ";
    getXingInfo(out);
    return out.str();
}


// checks that there is a match for version, layer, frequency
bool XingStreamBase::matchesStructure(const MpegStream& mpeg) const
{
    const MpegFrame& mpegFrm (mpeg.getFirstFrame());
    return (m_firstFrame.getVersion() == mpegFrm.getVersion() &&
        m_firstFrame.getLayer() == mpegFrm.getLayer() &&
        //m_firstFrame.getChannelMode() == mpegFrm.getChannelMode() &&
        m_firstFrame.getFrequency() == mpegFrm.getFrequency());
}

// checks that there is a match for version, layer, frequency and frame count
bool XingStreamBase::matches(const MpegStream& mpeg, bool bAcceptSelfInCount) const
{
    // the reason for using bAcceptSelfInCount is that some encoders incorrectly assumed that the Xing header should be counted along with the actual MPEG frames to get the
    // total frame count; it's probably better to ignore this mistake, as the fix will cause rebuilding the Xing header, thus losing the TOC and the gapless info
    return matchesStructure(mpeg) && ((getFrameCount() == mpeg.getFrameCount()) || (bAcceptSelfInCount && (getFrameCount() == mpeg.getFrameCount() + 1)));
}


// checks that pNext is an MpegStream*, in addition to matches(const MpegStream&)
bool XingStreamBase::matches(const DataStream* pNext, bool bAcceptSelfInCount) const
{
    const MpegStream* q (dynamic_cast<const MpegStream*>(pNext));
    return 0 != q && matches(*q, bAcceptSelfInCount);
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
    int nSideInfoSize (m_firstFrame.getSideInfoSize());
    //int LAME_OFFS (156 + nSideInfoSize - 32);
    int LAME_OFFS (12 + nSideInfoSize);
    unsigned char cFlags (getFlags());
    if ((cFlags & 0x01) != 0) {
        LAME_OFFS += 4;
    }
    if ((cFlags & 0x02) != 0) {
        LAME_OFFS += 4;
    }
    if ((cFlags & 0x04) != 0) {
        LAME_OFFS += 100;
    }
    if ((cFlags & 0x08) != 0) {
        LAME_OFFS += 4;
    }
    const int BFR_SIZE (LAME_OFFS + LAME_LABEL_SIZE); // MPEG header + side info + "Xing" size //ttt2 not sure if space for CRC16 should be added; then not sure if frame size should be increased by 2 when CRC is found
    char bfr [200]; // MSVC wants size to be a compile-time constant, so just use something bigger

    MP3_CHECK_T (BFR_SIZE <= m_firstFrame.getSize(), m_pos, "Not a LAME stream. This kind of MPEG audio doesn't support LAME.", CB_EXCP(NotLameStream)); // !!! some kinds of MPEG audio have very short frames, which can't accomodate a VBRI header

    streamsize nRead (read(in, bfr, BFR_SIZE));
    STRM_ASSERT (BFR_SIZE == nRead); // this was supposed to be a valid frame to begin with (otherwise the base class would have thrown) and BFR_SIZE is no bigger than the frame

    MP3_CHECK_T (0 == strncmp("LAME", bfr + LAME_OFFS, LAME_LABEL_SIZE), m_pos, "Not a LAME stream. Header not found.", CB_EXCP(NotLameStream)); // ttt0 lowercase

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
        << ", " << m_firstFrame.getBitrate() << "bps, CRC=" << convStr(GlobalTranslHlp::tr(boolAsYesNo(m_firstFrame.getCrcUsage()))) << "; ";
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

    MP3_CHECK_T (BFR_SIZE <= m_firstFrame.getSize(), m_pos, "Not a VBRI stream. This kind of MPEG audio doesn't support VBRI.", CB_EXCP(NotVbriStream)); // !!! some kinds of MPEG audio have very short frames, which can't accomodate a VBRI header

    streamsize nRead (read(in, bfr, BFR_SIZE));
    STRM_ASSERT (BFR_SIZE == nRead); // this was supposed to be a valid frame to begin with (otherwise the base class would have thrown) and BFR_SIZE is no bigger than the frame

    char* pLabel (bfr + MpegFrame::MPEG_FRAME_HDR_SIZE + 32);
    MP3_CHECK_T (0 == strncmp("VBRI", pLabel, VBRI_LABEL_SIZE), m_pos, "Not a VBRI stream. Header not found.", CB_EXCP(NotVbriStream));

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
        << ", " << m_firstFrame.getBitrate() << "bps, CRC=" << convStr(GlobalTranslHlp::tr(boolAsYesNo(m_firstFrame.getCrcUsage())));
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
    MP3_CHECK_T (BFR_SIZE == nRead, m_pos, "Invalid ID3V1 tag. File too short.", CB_EXCP(NotId3V1Stream));
    MP3_CHECK_T (0 == strncmp("TAG", m_data, 3), m_pos, "Invalid ID3V1 tag. Invalid header.", CB_EXCP(NotId3V1Stream));

    MP3_CHECK (BFR_SIZE == nRead, m_pos, id3v1TooShort, CB_EXCP(NotId3V1Stream));

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
    TestResult eTrack (checkId3V1String(m_data + 3, 30)); MP3_CHECK (BAD != eTrack, m_pos, id3v1InvalidName, CB_EXCP(NotId3V1Stream));
    TestResult eArtist (checkId3V1String(m_data + 33, 30)); MP3_CHECK (BAD != eArtist, m_pos, id3v1InvalidArtist, CB_EXCP(NotId3V1Stream));
    TestResult eAlbum (checkId3V1String(m_data + 63, 30)); MP3_CHECK (BAD != eAlbum, m_pos, id3v1InvalidAlbum, CB_EXCP(NotId3V1Stream));
    TestResult eYear (checkId3V1String(m_data + 93, 4)); MP3_CHECK (BAD != eYear, m_pos, id3v1InvalidYear, CB_EXCP(NotId3V1Stream));
    TestResult eComment (checkId3V1String(m_data + 97, 28)); MP3_CHECK (BAD != eComment, m_pos, id3v1InvalidComment, CB_EXCP(NotId3V1Stream)); // "28" is for ID3V1.1b (there's no reliable way to distinguish among versions 1.0 and 1.1 by design, and in practice among any of them because some tools use 0 instead of space and 0 seems to be a valid value for 1.1b's track and genre, for "undefined") //ttt2 use m_eVersion

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
    CB_CHECK (out, WriteError);
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


/*override*/ std::string Id3V1Stream::getTitle(bool* pbFrameExists /* = 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    return getStr(3, 30);
}


/*override*/ std::string Id3V1Stream::getArtist(bool* pbFrameExists /* = 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    return getStr(33, 30);
}


/*override*/ std::string Id3V1Stream::getTrackNumber(bool* pbFrameExists /* = 0*/) const
{
    if (V11b != m_eVersion) { if (0 != pbFrameExists) { *pbFrameExists = false; } return ""; }
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    char a [10];
    sprintf(a, "%02d", (int)(unsigned char)(m_data[126]));
    return a;
}


/*override*/ std::string Id3V1Stream::getGenre(bool* pbFrameExists /* = 0*/) const
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


/*override*/ std::string Id3V1Stream::getAlbumName(bool* pbFrameExists /* = 0*/) const
{
    if (0 != pbFrameExists) { *pbFrameExists = true; }
    return getStr(63, 30);
}


/*override*/ TagTimestamp Id3V1Stream::getTime(bool* pbFrameExists /* = 0*/) const
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
    return convStr(TagReader::tr("Comment: %1").arg(convStr(strComm)));
}


const char* getId3V1Genre(int n)
{
    if (n <= 0 || n > 147) { return ""; }

    static const char* aGenres [148] =
    {
        "Blues",  //ttt0 review transl - probably not, as ID3V2 is more important but hard to translate
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


