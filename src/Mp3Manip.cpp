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


#include  <iostream>
#include  <sstream>

#include  "Mp3Manip.h"

#include  "Helpers.h"
#include  "DataStream.h"
#include  "MpegStream.h"
#include  "ApeStream.h"
#include  "LyricsStream.h"
#include  "Id3V230Stream.h"
#include  "Id3V240Stream.h"
#include  "OsFile.h"

using namespace std;
using namespace pearl;



//============================================================================================================
//============================================================================================================
//============================================================================================================



//#define VERBOSE
//#define TRACE_FRAME_INFO

//============================================================================================================
//============================================================================================================
//============================================================================================================

/*static */ const QualThresholds& QualThresholds::getDefaultQualThresholds()
{
    static QualThresholds defaultQualThresholds = {

        192000, // m_nStereoCbr
        192000, // m_nJointStereoCbr
        192000, // m_nDoubleChannelCbr
        170000, // m_nStereoVbr
        160000, // m_nJointStereoVbr
        180000  // m_nDoubleChannelVbr
    };

    return defaultQualThresholds;
};

//============================================================================================================
//============================================================================================================
//============================================================================================================


static string s_strCrtMp3Handler;
static string s_strPrevMp3Handler;

string getGlobalMp3HandlerName() // a hack to get the name of the current file from inside various streams without storing the name there //ttt2 review
{
    return s_strCrtMp3Handler + "  (" + s_strPrevMp3Handler + ")";
}


Mp3Handler* Mp3Handler::create(const string& strFileName, bool bStoreTraceNotes, const QualThresholds& qualThresholds)
{
    static Timer timer; //ttt2 not multi-threaded, but doesn't matter
    static int64_t WAIT_AFTER_FAIL (30 * 1000000000L);
    static int64_t nLastFail (timer.getCrtTime() - WAIT_AFTER_FAIL);
    static int64_t nLastOkFirstAttempt (nLastFail);
    static int64_t nLastOkAfterRetry (nLastFail);
    try
    {
        Mp3Handler* res = new Mp3Handler(strFileName, bStoreTraceNotes, qualThresholds);
        nLastOkFirstAttempt = timer.getCrtTime();
        return res;
    }

//#define RETRY_EXCP const FileNotFound& ex
#define RETRY_EXCP const exception& ex
//#define RETRY_EXCP ...
    catch (RETRY_EXCP)
    {
        TRACER(convStr(QString("Attempt to open file \"%1\" failed. Reason: %2").arg(strFileName.c_str()).arg(ex.what())));
        int64_t nCrt = timer.getCrtTime();
        if (nCrt - nLastFail < WAIT_AFTER_FAIL)
        {
            TRACER(convStr(QString("Couldn't open file \"%1\" and last retry was too recent (%2 ms). Won't retry. (LastOkFirstAttempt=%3, LastOkAfterRetry=%4)").arg(strFileName.c_str()).arg((nCrt - nLastFail - WAIT_AFTER_FAIL) / 1000000).arg(nLastOkFirstAttempt).arg(nLastOkAfterRetry)));
            // we failed too recently; won't retry but also won't update nLastFail
            throw;
        }
        int nSlp (1000);
        TRACER1(convStr(QString("Couldn't open file \"%1\". Will retry, as last retry was not too recent (%2 ms)").arg(strFileName.c_str()).arg((nCrt - nLastFail - WAIT_AFTER_FAIL) / 1000000)), 1);
        for (;;)
        {
            TRACER(convStr(QString("Attempt to open file \"%1\" failed. Will sleep %2 ms ...").arg(strFileName.c_str()).arg(nSlp)));
            PausableThread::msleep(nSlp);
            try
            {
                Mp3Handler* res = new Mp3Handler(strFileName, bStoreTraceNotes, qualThresholds);
                nLastOkAfterRetry = timer.getCrtTime();
                return res;
            }
            catch (RETRY_EXCP)
            {
                nSlp *= 2;
                if (nSlp > 16)
                {
                    TRACER(convStr(QString("Multiple attempts to open file \"%1\" failed. Abandoning ...").arg(strFileName.c_str())));
                    nLastFail = timer.getCrtTime();
                    throw;
                }
            }
        }
    }
    catch (...)
    {
        TRACER(convStr(QString("Attempt to open file \"%1\" failed due to an unknown error. Won't retry").arg(strFileName.c_str())));
        throw;
    }
}


Mp3Handler::Mp3Handler(const string& strFileName, bool bStoreTraceNotes, const QualThresholds& qualThresholds) :
        m_pFileName(new StringWrp(strFileName)),

        m_pId3V230Stream(0),
        m_pId3V240Stream(0),
        m_pId3V1Stream(0),

        m_pLameStream(0),
        m_pXingStream(0),
        m_pVbriStream(0),
        m_pMpegStream(0),

        m_pApeStream(0),
        m_pLyricsStream(0),

        m_notes(1000), //ttt2 hard-coded

        m_nFastSaveTime(0)
{
#if 0
    { // simulate random disk error //ttt0 put back and test more cases
        static int max = 10;
        static int cnt = max;
        --cnt;
        if (cnt == 0)
        {
            cnt = max;
            CB_TRACE_AND_THROW1(FileNotFound, strFileName);
        }
    }
#endif
    s_strPrevMp3Handler = s_strCrtMp3Handler;
    s_strCrtMp3Handler = strFileName;
    //TRACER1A("Mp3Handler constr ", 1);

    TRACER("Mp3Handler constr: " + strFileName);
    ifstream_utf8 in (m_pFileName->s.c_str(), ios::binary);
    //ifstream_utf8 in (endsWith(m_pFileName->s, ".mp3") ? m_pFileName->s.c_str() : "/a/b/c", ios::binary);

    if (!in)
    {
        qDebug("Couldn't open file \"%s\"", strFileName.c_str());
        //inspect(strFileName.c_str(), cSize(strFileName) + 1);
        trace("Couldn't open file: " + strFileName);
        CB_TRACE_AND_THROW1(FileNotFound, strFileName); //ttt2 got triggered (email on 2016.03.05); 2016.06.22: was able to simulate the crash by setting a breakpoint and then applying a transformation and removing the temporary file when it paused
    }
    //TRACER1A("Mp3Handler constr ", 2);

    ostringstream out;
    time_t t (time(0));
    out << "************************* " << strFileName << " ************************* memory: " << getMemUsage() << "; time: " << ctime(&t);
    string s (out.str());
    s.erase(s.size() - 1); // needed because ctime() uses a terminating '\n'
    trace("");
    trace(s);

    istream* pIn (&in);
    stringstream bfr;
    for (int i = 0; i < 3; ++i)
    {
        long long nChangeTime, nSize;
        getFileInfo(strFileName, nChangeTime, nSize);
        if (nSize > 30000000)
        {
            TRACER("File is too big to be buffered: " + strFileName);
            break;
        }

        bfr.str(string());
        const int READ_SIZE (1 << 19);
        char a [READ_SIZE];
        streamsize nTotalRead (0);
        for (;;)
        {
            streamsize nCrtRead (read(in, a, READ_SIZE));
            bfr.write(a, nCrtRead);
            nTotalRead += nCrtRead;
            if (nCrtRead < READ_SIZE)
            {
                break;
            }
        }
        if (nTotalRead == nSize)
        {
            pIn = &bfr;
            break;
        }
        TRACER("Failed to read whole file in the internal buffer. Retrying: " + strFileName);
        PausableThread::msleep(50);
        in.close();
        in.clear();
        in.open(m_pFileName->s.c_str(), ios::binary);
    }//*/


    // cout << s << endl;
//TRACER1A("Mp3Handler constr ", 3);

    //parse(in);
    try
    {
        parse(*pIn);
    }
    catch (const exception& ex)
    {
        TRACER(string("Mp3Handler::Mp3Handler() / parse: ") + ex.what());
        std::streampos pos (0);
        //ttt2 not sure if better to show the other streams and notes or not
        /*if (!m_vpAllStreams.empty())
        {
            pos = m_vpAllStreams.back()->getEnd();
        }*/
        clearPtrContainer(m_vpAllStreams);
        m_vpAllStreams.push_back(new UnreadableDataStream(m_vpAllStreams.size(), pos, string("Failure point: ") + ex.what())); // ttt1 actually the exception could be related to other issues, perhaps having nothing to do with reading the file; anyway, the note says that it couldn't read the file, not that the file/drive is bad
        //ttt1 not clear if it makes more sense to have the failure point in the stream or in the note
    }

    //TRACER1A("Mp3Handler constr ", 4);
    m_notes.resetCounter();
    //TRACER1A("Mp3Handler constr ", 5);
    analyze(qualThresholds);
    //TRACER1A("Mp3Handler constr ", 6);

    if (!bStoreTraceNotes)
    {
    //TRACER1A("Mp3Handler constr ", 7);
        m_notes.removeTraceNotes();
    }
//TRACER1A("Mp3Handler constr ", 8);
    getFileInfo(strFileName, m_nTime, m_nSize);
    //TRACER1A("Mp3Handler constr ", 9);
}


Mp3Handler::~Mp3Handler()
{
    TRACER("Mp3Handler destr: " + m_pFileName->s);
//qDebug("begin destroying Mp3Handler at %p", this);
    /*delete m_pId3V230Stream;
    delete m_pId3V240Stream;
    delete m_pId3V1Stream;
    delete m_pLameStream;
    delete m_pXingStream;
    delete m_pMpegStream;
    delete m_pApeStream;
    clearPtrContainer(m_vpNullStreams);
    clearPtrContainer(m_vpUnknownStreams);*/

    clearPtrContainer(m_vpAllStreams);
    //TRACER1A("Mp3Handler destr ", 1);
    delete m_pFileName;

//qDebug("done destroying Mp3Handler at %p", this);
    //clearPtrContainer(m_notes);
}


const Id3V2StreamBase* Mp3Handler::getId3V2Stream() const { if (0 != m_pId3V230Stream) { return m_pId3V230Stream; } return  m_pId3V240Stream; }


// what looks like the last frame in an MPEG stream may actually be truncated and somewhere inside it an ID3V1 or Ape tag may actually begin; if that's the case, that "frame" is removed from the stream; then most likely an "Unknown" stream will be detected, followed by an ID3V1 or Ape stream //ttt2 make sure that that is the case; a possibility is that the standard allows the last frame to be shorter than the calculated size, if some condition is met; this seems unlikely, though
void Mp3Handler::checkLastFrameInMpegStream(istream& in)
{
    STRM_ASSERT (!m_vpAllStreams.empty());
    MpegStream* pStream (dynamic_cast<MpegStream*>(m_vpAllStreams.back()));
    if (0 == pStream) { return; }

    streampos pos (pStream->getLastFramePos());
    streampos posNext (pStream->getPos()); posNext += pStream->getSize();

    NoteColl notes (0);

    for (;;)
    {
        if (posNext - pos <= 0)
        {
            //clearPtrContainer(vpNotes);
            return;
        }
        in.clear();

        in.seekg(pos);
        try
        {
            DataStream* p (new Id3V240Stream(0, notes, in, m_pFileName));
            delete p;
            break;
        }
        catch (const std::bad_alloc&) { throw; }
        catch (...) //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        {
            in.clear();
            in.seekg(pos);
        }

        in.seekg(pos);
        try
        {
            DataStream* p (new ApeStream(0, notes, in));
            delete p;
            break;
        }
        catch (const std::bad_alloc&) { throw; }
        catch (...) //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        {
            in.clear();
            in.seekg(pos);
        }

        in.seekg(pos);
        try
        {
            DataStream* p (new Id3V1Stream(0, notes, in));
            delete p;
            break;
        }
        catch (const std::bad_alloc&) { throw; }
        catch (...) //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        {
            in.clear();
            in.seekg(pos);
        }

        in.seekg(pos);
        try
        {
            DataStream* p (new LyricsStream(0, notes, in, m_pFileName->s));
            delete p;
            break;
        }
        catch (const std::bad_alloc&) { throw; }
        catch (...) //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        {
            in.clear();
            in.seekg(pos);
        }


        try
        {
            pos = getNextStream(in, pos);
        }
        catch (const EndOfFile&)
        {
            return;
        }

    }

    //clearPtrContainer(notes);
    pStream->removeLastFrame();
}


void Mp3Handler::parse(istream& in) // ttt2 this function is a mess; needs rethinking
{
    in.seekg(0, ios::end);
    m_posEnd = in.tellg();
    in.seekg(0, ios::beg);
    STRM_ASSERT (in);
    int nIndex (0);
    //NoteColl& notes (m_notes);

    streampos pos (0);
    while (pos < m_posEnd)
    {
        in.clear();
        in.seekg(pos);

        //if (m_vpAllStreams.size() > 48)
        if (cSize(m_vpAllStreams) > 100) //ttt2 perhaps make this configurable
        {
            {
                m_notes.resetCounter();
                NoteColl& notes (m_notes);
                MP3_NOTE (pos, tooManyStreams);
            }

            UnknownDataStream* p (new UnknownDataStream(nIndex, m_notes, in, m_posEnd - pos));
            UnknownDataStream* pPrev (m_vpAllStreams.empty() ? 0 : dynamic_cast<UnknownDataStream*>(m_vpAllStreams.back()));
            if (0 != pPrev)
            { // last stream is "unknown" too
                pPrev->append(*p);
                delete p;
            }
            else
            {
                m_vpAllStreams.push_back(p);
                ++nIndex;
            }
            pos = m_posEnd;
            break;
        }

        bool bBrokenMpegFrameFound (false);
        int nBrokenMpegFrameCount (0);

        string strBrokenInfo; const char* szBrokenName (0);
        string strUnsupportedInfo; const char* szUnsupportedName (0);

        try
        {
            m_vpAllStreams.push_back(new Id3V230Stream(nIndex, m_notes, in, m_pFileName));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new Id3V240Stream(nIndex, m_notes, in, m_pFileName));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new LameStream(nIndex, m_notes, in));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const MpegFrame::PrematurelyEndedMpegFrame& ex)
        { // this could as well be in the Xing, Vbri or MPEG block and the program would perform pretty much the same (the only difference is that more exceptions would be thrown and caught as "...", with no side effect, if one of the other blocks is used); the exception means that what began as an MPEG frame should be longer than what is left in the file
            bBrokenMpegFrameFound = true;
            szBrokenName = MpegStream::getClassDisplayName();
            strBrokenInfo = ex.m_strInfo;
            goto e1;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new XingStream(nIndex, m_notes, in));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new VbriStream(nIndex, m_notes, in));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new MpegStream(nIndex, m_notes, in));
            trace("enter checkLastFrameInMpegStream()");
            checkLastFrameInMpegStream(in);
            trace("exit checkLastFrameInMpegStream()");
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const MpegStream::StreamTooShort& ex)
        { // not 100% correct, but most likely it gets here after in the previous step it managed to read an MPEG stream which had a truncated last frame, which was removed from that stream and now it has thrown this exception; quite similar to what PrematurelyEndedMpegFrame as caught in the LameStream block above is doing, the difference here being that there are enough bytes left in the file to read a full "frame", while there it's EOF; both cases are likely to have some other streams inside them
            bBrokenMpegFrameFound = true;
            szBrokenName = MpegStream::getClassDisplayName();
            strBrokenInfo = ex.m_strInfo;
            nBrokenMpegFrameCount = ex.m_nFrameCount;
            goto e1;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (const CbException& ex)
        {
            //qDebug("%s", ex.what()); //ttt2 here we hide, e.g. "Unsupported version (2.5)", so the end user never sees it
        }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new ApeStream(nIndex, m_notes, in));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new Id3V1Stream(nIndex, m_notes, in));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new LyricsStream(nIndex, m_notes, in, m_pFileName->s));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        try
        {
            m_vpAllStreams.push_back(new NullDataStream(nIndex, m_notes, in));
            pos += m_vpAllStreams.back()->getSize();
            ++nIndex;
            continue;
        }
        catch (const StreamIsBroken& ex) { if (0 == szBrokenName) { szBrokenName = ex.getStreamName(); strBrokenInfo = ex.getInfo(); } }
        catch (const StreamIsUnsupported& ex) { if (0 == szUnsupportedName) { szUnsupportedName = ex.getStreamName(); strUnsupportedInfo = ex.getInfo(); } }
        catch (const std::bad_alloc&) { throw; }
        catch (...) {} //ttt2 replace "..." with something app-specific, to avoid catching system exceptions
        in.clear(); in.seekg(pos);

        //------------------------------------------------------------------
e1:
        streampos posNextFrame;
        try
        {
            posNextFrame = getNextStream(in, pos);
        }
        catch (const EndOfFile&)
        {
            posNextFrame = m_posEnd;
            in.clear();
        }

        //try
        {
            bool bAdded (false);

            if (bBrokenMpegFrameFound) //ttt2 review this: "bBrokenMpegFrameFound gets set if MpegStream::StreamTooShort is thrown"; it's probably OK
            {
                try
                {
                    MpegStream* pPrev (m_vpAllStreams.empty() ? 0 : dynamic_cast<MpegStream*>(m_vpAllStreams.back()));
                    m_vpAllStreams.push_back(new TruncatedMpegDataStream(pPrev, nIndex, m_notes, in, posNextFrame - pos));
                    pos += m_vpAllStreams.back()->getSize();
                    ++nIndex;
                    bAdded = true;
                }
                catch (const TruncatedMpegDataStream::NotTruncatedMpegDataStream&)
                {
                    in.clear();
                    in.seekg(pos);
/*
                      ttt2 perhaps add limit of 10 broken streams per file
                            keep in mind that a transformation that removes broken streams will have trouble processing a file with "10 broken streams", because after it removes the first, a new one will get created, always having 10 of them
*/

                }
            }

//ttt2 consider this: there's a stream beginning identified at 1000 and the next one is at 8000; if the first begins with a valid MPEG frame and the second is something else, the whole block from 1000 up to 7999 is identified as "broken mpeg"; this isn't quite right: since no other mpeg frame can be found in the block (if it could, it would be the second stream beginning), there's a lot of other data in the stream besides an mpeg frame; this doesn't feel right;

            if (!bAdded)
            {
                streamoff nSize (posNextFrame - pos);
                if (0 != szUnsupportedName)
                {
                    m_vpAllStreams.push_back(new UnsupportedDataStream(nIndex, m_notes, in, nSize, szUnsupportedName, strUnsupportedInfo));
                    //pos += m_vpAllStreams.back()->getSize();
                    ++nIndex;
                }
                else
                { // either broken or unknown; these have little useful information, so try to append to the previous one, in these cases: both broken and unknown can be appended to truncated and unknown alone can be appended to unknown
                    DataStream* pPrev (m_vpAllStreams.empty() ? 0 : m_vpAllStreams.back());

                    TruncatedMpegDataStream* pPrevTrunc (dynamic_cast<TruncatedMpegDataStream*>(pPrev));
                    UnknownDataStream* pPrevUnkn (dynamic_cast<UnknownDataStream*>(pPrev));
                    UnsupportedDataStream* pPrevUnsupp (dynamic_cast<UnsupportedDataStream*>(pPrev));

                    if (MpegStream::getClassDisplayName() == szBrokenName && 1 == nBrokenMpegFrameCount)
                    { // a "broken audio" with a single sense doesn't make much sense at this point (but it mattered above, to add a truncated audio stream)
                        szBrokenName = 0;
                    }

                    if (0 != pPrevTrunc && pPrevTrunc->hasSpace(nSize))
                    { // append to truncated
                        UnknownDataStream* p (new UnknownDataStream(nIndex, m_notes, in, nSize));
                        pPrevTrunc->append(*p);
                        delete p;
                    }
                    else if (0 != szBrokenName)
                    { // create broken
                        m_vpAllStreams.push_back(new BrokenDataStream(nIndex, m_notes, in, nSize, szBrokenName, strBrokenInfo));
                        //pos += m_vpAllStreams.back()->getSize();
                        ++nIndex;
                    }
                    else if (0 != pPrevUnkn)
                    { // append to unknown
                        UnknownDataStream* p (new UnknownDataStream(nIndex, m_notes, in, nSize));
                        pPrevUnkn->append(*p);
                        delete p;
                    }
                    else if (0 != pPrevUnsupp)
                    { // append to unknown
                        UnknownDataStream* p (new UnknownDataStream(nIndex, m_notes, in, nSize));
                        pPrevUnsupp->append(*p);
                        delete p;
                    }
                    else
                    { // create unknown
                        UnknownDataStream* p (new UnknownDataStream(nIndex, m_notes, in, nSize));
                        m_vpAllStreams.push_back(p);
                        ++nIndex;
                    }
                }

                pos += nSize;
            }
        }
        /*catch (...) // !!! DON'T catch anything; there's no point; the file is changed or something else pretty bad happened, because creating an Unknown stream shouldn't throw; most likely the existing streams became invalid too; also, there's a chance that the implementation of UnknownDataStream has a bug and an infinite loop will be entered, as all the constructors fail and the position in the file doesn't advance
        {
            in.clear();
            in.seekg(pos); not ok
        }*/

    }

    //cout << "=======================\n";

    //CB_ASSERT (!m_vpAllStreams.empty());
    STRM_ASSERT (pos == m_posEnd); // ttt1 triggered according to mail on 2012.12.16 and on 2016.09.04
    pos = 0;
    for (int i = 0; i < cSize(m_vpAllStreams); ++i)
    {
        DataStream* p (m_vpAllStreams[i]);
        //cout << p->getInfo() << endl;
        STRM_ASSERT (p->getPos() == pos);
        pos += p->getSize();
    }
    STRM_ASSERT (pos == m_posEnd);
}



// checks the streams for issues (missing ID3V2, Unknown streams, inconsistencies, ...)
void Mp3Handler::analyze(const QualThresholds& qualThresholds)
{
    NoteColl& notes (m_notes); // for MP3_NOTE()

    if (cSize(m_vpAllStreams) == 1)
    {
        UnreadableDataStream* p (dynamic_cast<UnreadableDataStream*>(m_vpAllStreams[0]));
        if (0 != p)
        {
            m_notes.removeNotes(0, 1 << 30);
            MP3_NOTE (0, failedToRead);
            return; // no need for other notes if the file couldn't be read
        }
    }

    for (int i = 0, n = cSize(m_vpAllStreams); i < n; ++i)
    {
        DataStream* pDs (m_vpAllStreams[i]);
        {
            Id3V230Stream* p (dynamic_cast<Id3V230Stream*>(pDs));
            if (0 != p)
            {
                if (0 == m_pId3V230Stream)
                    m_pId3V230Stream = p;
                else
                    MP3_NOTE (p->getPos(), twoId3V230);
            }
        }

        {
            Id3V240Stream* p (dynamic_cast<Id3V240Stream*>(pDs));
            if (0 != p)
            {
                if (0 == m_pId3V240Stream)
                    m_pId3V240Stream = p;
                else
                    MP3_NOTE (p->getPos(), twoId3V240);
            }
        }

        {
            Id3V1Stream* p (dynamic_cast<Id3V1Stream*>(pDs));
            if (0 != p)
            {
                if (0 == m_pId3V1Stream)
                    m_pId3V1Stream = p;
                else
                    MP3_NOTE (p->getPos(), twoId3V1);
            }
        }

        {
            LameStream* p (dynamic_cast<LameStream*>(pDs));
            if (0 != p)
            {
                if (0 == m_pLameStream)
                    m_pLameStream = p;
                else
                    MP3_NOTE (p->getPos(), twoLame);
            }
        }

        {
            XingStreamBase* p (dynamic_cast<XingStreamBase*>(pDs));
            if (0 != p)
            {
                if (0 == m_pXingStream)
                {
                    m_pXingStream = p;
                    if (i < n - 2 && p->isBrokenByMp3Fixer(m_vpAllStreams[i + 1], m_vpAllStreams[i + 2]))
                    {
                        MP3_NOTE (p->getPos(), xingAddedByMp3Fixer);
                    }
                    if (i < n - 1)
                    {
                        MpegStream* q (dynamic_cast<MpegStream*>(m_vpAllStreams[i + 1]));
                        if (0 != q && p->getFrameCount() != q->getFrameCount())
                        {
                            if (p->getFrameCount() == q->getFrameCount() + 1)
                            {
                                MP3_NOTE (p->getPos(), xingFrameInCount);
                            }
                            else
                            {
                                MP3_NOTE (p->getPos(), xingFrameCountMismatch);
                            }
                        }
                    }
                }
                else
                    MP3_NOTE (p->getPos(), twoXing);
            }
        }

        {
            VbriStream* p (dynamic_cast<VbriStream*>(pDs));
            if (0 != p)
            {
                if (0 == m_pVbriStream)
                    m_pVbriStream = p;
                else
                    MP3_NOTE (p->getPos(), twoVbri);
            }
        }

        {
            MpegStream* p (dynamic_cast<MpegStream*>(pDs));
            if (0 != p)
            {
                if (0 == m_pMpegStream)
                    m_pMpegStream = p;
                else
                    MP3_NOTE (p->getPos(), twoAudio);

                //const MpegFrame& frm (p->getFirstFrame());
                if (p->isVbr())
                {
                    switch (p->getChannelMode())
                    { // ttt2 should be possible that changing qual thresholds in config triggers notes being added / removed
                    case MpegFrame::STEREO: if (p->getBitrate() < qualThresholds.m_nStereoVbr) { MP3_NOTE (p->getPos(), lowQualAudio); } break;
                    case MpegFrame::JOINT_STEREO: if (p->getBitrate() < qualThresholds.m_nJointStereoVbr) { MP3_NOTE (p->getPos(), lowQualAudio); } break;
                    case MpegFrame::DUAL_CHANNEL: if (p->getBitrate() < qualThresholds.m_nDoubleChannelVbr) { MP3_NOTE (p->getPos(), lowQualAudio); } break;
                    case MpegFrame::SINGLE_CHANNEL: if (p->getBitrate() < qualThresholds.m_nDoubleChannelVbr / 2) { MP3_NOTE (p->getPos(), lowQualAudio); } break;
                    }
                }
                else
                {
                    switch (p->getChannelMode())
                    {
                    case MpegFrame::STEREO: if (p->getBitrate() < qualThresholds.m_nStereoCbr) { MP3_NOTE (p->getPos(), lowQualAudio); } break;
                    case MpegFrame::JOINT_STEREO: if (p->getBitrate() < qualThresholds.m_nJointStereoCbr) { MP3_NOTE (p->getPos(), lowQualAudio); } break;
                    case MpegFrame::DUAL_CHANNEL: if (p->getBitrate() < qualThresholds.m_nDoubleChannelCbr) { MP3_NOTE (p->getPos(), lowQualAudio); } break;
                    case MpegFrame::SINGLE_CHANNEL: if (p->getBitrate() < qualThresholds.m_nDoubleChannelCbr / 2) { MP3_NOTE (p->getPos(), lowQualAudio); } break;
                    }
                }
            }
        }

        {
            ApeStream* p (dynamic_cast<ApeStream*>(pDs));
            if (0 != p)
            {
                if (0 == m_pApeStream)
                    m_pApeStream = p;
                else
                    MP3_NOTE (p->getPos(), twoApe);
            }
        }

        {
            LyricsStream* p (dynamic_cast<LyricsStream*>(pDs));
            if (0 != p)
            {
                if (0 == m_pLyricsStream)
                    m_pLyricsStream = p;
                else
                    MP3_NOTE (p->getPos(), twoLyr);
            }
        }

        {
            NullDataStream* p (dynamic_cast<NullDataStream*>(pDs));
            if (0 != p)
            {
                m_vpNullStreams.push_back(p);
            }
        }

        {
            UnknownDataStream* p (dynamic_cast<UnknownDataStream*>(pDs));
            if (0 != p)
            {
                m_vpUnknownStreams.push_back(p);
            }
        }

        {
            BrokenDataStream* p (dynamic_cast<BrokenDataStream*>(pDs));
            if (0 != p)
            {
                m_vpBrokenStreams.push_back(p);
            }
        }

        {
            UnsupportedDataStream* p (dynamic_cast<UnsupportedDataStream*>(pDs));
            if (0 != p)
            {
                m_vpUnsupportedStreams.push_back(p);
            }
        }

        {
            TruncatedMpegDataStream* p (dynamic_cast<TruncatedMpegDataStream*>(pDs));
            if (0 != p)
            {
                m_vpTruncatedMpegStreams.push_back(p);
            }
        }
    }

    if (0 == m_pMpegStream) { MP3_NOTE (-1, noAudio); }
    if (0 == m_pId3V230Stream) { MP3_NOTE (-1, noId3V230); }
    if (0 != m_pId3V230Stream && 0 != m_pId3V240Stream) { MP3_NOTE (m_pId3V240Stream->getPos(), bothId3V230_V240); }
    if (0 != m_pVbriStream) { MP3_NOTE (m_pVbriStream->getPos(), vbriFound); }
    //if (0 != m_pLyricsStream) { MP3_NOTE (m_pLyricsStream->getPos(), lyricsNotSupported); }
    if (0 != m_pId3V1Stream && 0 == m_pId3V230Stream && 0 == m_pId3V240Stream && 0 == m_pApeStream) { MP3_NOTE (m_pId3V1Stream->getPos(), onlyId3V1); }
    if (0 == m_pId3V1Stream && 0 == m_pId3V230Stream && 0 == m_pId3V240Stream && 0 == m_pApeStream) { MP3_NOTE (-1, noInfoTag); }
    if (!m_vpNullStreams.empty()) { MP3_NOTE (m_vpNullStreams[0]->getPos(), foundNull); }
    if (0 != m_pVbriStream && 0 != m_pXingStream) { MP3_NOTE (m_pVbriStream->getPos(), foundVbriAndXing); }
    if (0 != m_pXingStream && 0 != m_pMpegStream && m_pXingStream->getIndex() != m_pMpegStream->getIndex() - 1) { MP3_NOTE (m_pXingStream->getPos(), xingNotBeforeAudio); }

    if (0 != m_pXingStream && 0 != m_pMpegStream && !m_pXingStream->matchesStructure(*m_pMpegStream)) { MP3_NOTE (m_pXingStream->getPos(), incompatXing); }
    if (0 != m_pId3V1Stream && 0 != m_pMpegStream && m_pId3V1Stream->getIndex() < m_pMpegStream->getIndex()) { MP3_NOTE (m_pId3V1Stream->getPos(), id3v1BeforeAudio); }
    if (0 != m_pId3V230Stream && 0 != m_pId3V230Stream->getIndex()) { MP3_NOTE (m_pId3V230Stream->getPos(), id3v230AfterAudio); }
    if (0 == m_pXingStream && 0 != m_pMpegStream && m_pMpegStream->isVbr()) { MP3_NOTE (m_pMpegStream->getPos(), missingXing); }
    if (0 != m_pMpegStream && m_pMpegStream->isVbr() && (MpegFrame::MPEG1 != m_pMpegStream->getFirstFrame().getVersion() || MpegFrame::LAYER3 != m_pMpegStream->getFirstFrame().getLayer())) { MP3_NOTE (m_pMpegStream->getPos(), vbrUsedForNonMpg1L3); }
    if (
            (0 != m_pMpegStream) &&
            (0 == m_pApeStream || !m_pApeStream->hasMp3Gain()) &&
            (0 == m_pId3V230Stream || !m_pId3V230Stream->hasReplayGain()) &&
            (0 == m_pId3V240Stream || !m_pId3V240Stream->hasReplayGain())) //ttt2 not quite OK when mutliple ID3V2 streams are found
    {
        MP3_NOTE (m_pMpegStream->getPos(), noMp3Gain);
    }

    if (!m_vpBrokenStreams.empty())
    {
        if (1 == cSize(m_vpBrokenStreams) && m_vpBrokenStreams.back() == m_vpAllStreams.back())
        {
            MP3_NOTE (m_vpBrokenStreams.back()->getPos(), brokenAtTheEnd);
        }
        else
        {
            MP3_NOTE (m_vpBrokenStreams.back()->getPos(), brokenInTheMiddle);
        }
    }

    if (!m_vpUnknownStreams.empty())
    {
        if (1 == cSize(m_vpUnknownStreams) && m_vpUnknownStreams.back() == m_vpAllStreams.back())
        {
            MP3_NOTE (m_vpUnknownStreams.back()->getPos(), unknownAtTheEnd);
        }
        else
        {
            MP3_NOTE (m_vpUnknownStreams.back()->getPos(), unknownInTheMiddle);
        }
    }

    if (!m_vpUnsupportedStreams.empty())
    {
        MP3_NOTE (m_vpUnsupportedStreams.back()->getPos(), unsupportedFound);
    }

    if (!m_vpTruncatedMpegStreams.empty())
    {
        if (1 == cSize(m_vpTruncatedMpegStreams) && m_vpTruncatedMpegStreams.back() == m_vpAllStreams.back())
        {
            MP3_NOTE (m_vpTruncatedMpegStreams.back()->getPos(), truncAudioWithWholeFile);
        }
        else
        {
            MP3_NOTE (m_vpTruncatedMpegStreams.back()->getPos(), truncAudio);
        }
    }

    m_notes.sort();
    //sort(m_notes.begin(), m_notes.end(), notePtrCmp);
}



// removes the ID3V2 tag and the notes associated with it and scans the file again, but only the new ID3V2 tag;
// asserts that there was an existing ID3V2 tag at the beginning and it had the same size as the new one;
// this isn't really const, but it seems better to have a const_cast in the only place where it is needed rather than remove the const restriction from many places
void Mp3Handler::reloadId3V2() const
{
    const_cast<Mp3Handler*>(this)->reloadId3V2Hlp();
}

void Mp3Handler::reloadId3V2Hlp()
{
    STRM_ASSERT (!m_vpAllStreams.empty());
    Id3V2StreamBase* pOldId3V2 (dynamic_cast<Id3V2StreamBase*>(m_vpAllStreams[0]));
    STRM_ASSERT (0 != pOldId3V2);

    m_notes.removeNotes(pOldId3V2->getPos(), pOldId3V2->getPos() + pOldId3V2->getSize());

    ifstream_utf8 in (m_pFileName->s.c_str(), ios::binary); //ttt1 the constructor uses a buffer and retries, which should probably happen here as well, but since it's the beginning of file, no rewinding, it's lower risk;

    STRM_ASSERT (in); // ttt2 not quite right; could have been deleted externally

    Id3V230Stream* pNewId3V2;
    try
    {
        pNewId3V2 = new Id3V230Stream(0, m_notes, in, m_pFileName);
    }
    catch (const std::bad_alloc&) { throw; }
    catch (const exception& ex)
    {
        TRACER1("Mp3Handler::reloadId3V2Hlp()", 1);
        TRACER1(ex.what(), 2);
        STRM_ASSERT (false); // ttt2 this assert won't have the exception; sure, it's in the log, but should be in assert as well
    }
    catch (...)
    {
        TRACER("Mp3Handler::reloadId3V2Hlp() - unknown exception");
        STRM_ASSERT (false);
    }

    STRM_ASSERT (pOldId3V2->getSize() == pNewId3V2->getSize());

    delete pOldId3V2;
    m_vpAllStreams[0] = pNewId3V2;
    if (m_pId3V240Stream == pOldId3V2) { m_pId3V240Stream = 0; }
    m_pId3V230Stream = pNewId3V2;

    m_notes.addFastSaveWarn();
    m_notes.sort();
}



static bool isMpegHdr(unsigned char* p)
{
    bool b;
    decodeMpegFrame(reinterpret_cast<char*>(p), " ", &b);
    return b;
}



// finds the position where the next ID3V2, MPEG, Xing, Lame or Ape stream begins; "pos" is not considered; the search starts at pos+1; throws if no stream is found
streampos getNextStream(istream& in, streampos pos)
{
    StreamStateRestorer rst (in);
    static const int BFR_SIZE (1024);
    char bfr [BFR_SIZE];

    static const int MPEG_HDR_SIZE (4);
    static const char* ID3V2_HDR ("ID3"); static const int ID3V2_HDR_SIZE (strlen(ID3V2_HDR));
    static const char* ID3V1_HDR ("TAG"); static const int ID3V1_HDR_SIZE (strlen(ID3V1_HDR));
    static const char* APE_HDR ("APETAGEX"); static const int APE_HDR_SIZE (strlen(APE_HDR));
    static const char* LYRICS_HDR ("LYRICSBEGIN"); static const int LYRICS_HDR_SIZE (strlen(LYRICS_HDR));
    static const int MAX_HDR_SIZE (max(MPEG_HDR_SIZE, max(ID3V2_HDR_SIZE, max(ID3V1_HDR_SIZE, max(APE_HDR_SIZE, LYRICS_HDR_SIZE)))));

    pos += 1;
    int i (0);

    in.clear();
    for (;;)
    {
        in.seekg(pos);
        int nRead (read(in, bfr, BFR_SIZE));
        if (0 == nRead)
        {
            CB_THROW(EndOfFile);
        }

        for (i = 0; i < nRead; ++i)
        {
            unsigned char* p (reinterpret_cast<unsigned char*>(bfr + i));
            //if (nRead - i >= MPEG_HDR_SIZE && 0xff == *p && 0xe0 == (0xe0 & *(p + 1))) { goto e1; }
            if (nRead - i >= MPEG_HDR_SIZE && isMpegHdr(p)) { goto e1; } // MPEG, Xing, Lame // older comment: "ff ff" is not a valid MPEG beginning, so it shouldn't pass // 2008.08.06: actually it's valid: "MPEG1, Layer 1, No CRC"; however, "ff e?", "ff f0", "ff f1", "ff f8" and "ff f9" are invalid, and they are tested by isMpegHdr
            if (nRead - i >= ID3V2_HDR_SIZE && 0 == strncmp(ID3V2_HDR, bfr + i, ID3V2_HDR_SIZE)) { goto e1; } // ID3V2 (2.3, 2.2, 2.4)
            if (nRead - i >= ID3V1_HDR_SIZE && 0 == strncmp(ID3V1_HDR, bfr + i, ID3V1_HDR_SIZE)) { goto e1; } // ID3V1
            if (nRead - i >= APE_HDR_SIZE && 0 == strncmp(APE_HDR, bfr + i, APE_HDR_SIZE)) { goto e1; } // Ape V2
            if (nRead - i >= APE_HDR_SIZE && 0 == strncmp(LYRICS_HDR, bfr + i, LYRICS_HDR_SIZE)) { goto e1; } // Lyrics
        }
        pos += BFR_SIZE - MAX_HDR_SIZE + 1;
    }
e1:
    pos += i;
    /*if (g_bVerbose)*/ { TRACE("stream found at 0x" << hex << pos << dec); }

    // !!! no need to call rst.setOk(); we want the read pointer to be restored
    return pos;
}


/*void Mp3Handler::logStreamInfo() const
{
    //cout << endl;
    for (vector<DataStream*>::const_iterator it = m_vpAllStreams.begin(), end = m_vpAllStreams.end(); it != end; ++it)
    {
        //cout << " " << (*it)->getName();
        TRACE((*it)->getInfo());
    }
    //cout << endl;
}


void Mp3Handler::logStreamInfo(std::ostream& out) const
{
    for (vector<DataStream*>::const_iterator it = m_vpAllStreams.begin(), end = m_vpAllStreams.end(); it != end; ++it)
    {
        out << (*it)->getInfo() << endl;
    }
}
*/


long long Mp3Handler::getSize() const
{
    return m_posEnd; //ttt3 doesn't work for large files
}


QString Mp3Handler::getUiName() const // uses native separators
{
    return toNativeSeparators(convStr(getName()));
}


string Mp3Handler::getShortName() const
{
    string::size_type n (m_pFileName->s.rfind(getPathSep()));
    STRM_ASSERT (string::npos != n);
    return m_pFileName->s.substr(n + 1);
}

string Mp3Handler::getDir() const
{
    return getParent(m_pFileName->s);
}



// if the underlying file seems changed (or removed); looks at time and size, as well as FastSaveWarn and Notes::getMissingNote(); a difference in size always makes this return true; if bConsiderFastSave is true, both m_nFastSaveTime and m_nTime are tested, while m_notes.hasFastSaveWarn() and Notes::getMissingNote() are ignored; if it is false, m_nFastSaveTime is ignored, while m_nTime, m_notes.hasFastSaveWarn(), and Notes::getMissingNote() are tested
bool Mp3Handler::needsReload(bool bUseFastSave) const
{
    long long nSize, nTime;
    try
    {
        getFileInfo(m_pFileName->s, nTime, nSize);
    }
    catch (const NameNotFound&)
    {
        return true;
    }

    if (nSize != m_nSize) { return true; }

    if (bUseFastSave)
    {
        return nTime != m_nTime && nTime != m_nFastSaveTime;
    }

    if (nTime != m_nTime || m_notes.hasFastSaveWarn()) { return true; }

    const vector<Note*>& vpNotes (m_notes.getList());
    return !vpNotes.empty() && vpNotes[0]->getDescription() == Notes::getMissingNote()->getDescription();
}


//============================================================================================================
//============================================================================================================
//============================================================================================================


bool CmpMp3HandlerPtrByName::operator()(const Mp3Handler* p1, const std::string& strName2) const
{
    return cmp(p1->getName(), strName2);
}

bool CmpMp3HandlerPtrByName::operator()(const std::string& strName1, const Mp3Handler* p2) const
{
    return cmp(strName1, p2->getName());
}

bool CmpMp3HandlerPtrByName::operator()(const Mp3Handler* p1, const Mp3Handler* p2) const
{
    return cmp(p1->getName(), p2->getName());
}


bool CmpMp3HandlerPtrByName::cmp(const std::string& strName1, const std::string& strName2) const
{
    const std::string strDir1 (getParent(strName1));
    const std::string strDir2 (getParent(strName2));

    if (strDir1 == strDir2) { return strName1 < strName2; } // ttt3 maybe make comparison locale-dependant
    return strDir1 < strDir2;
}

