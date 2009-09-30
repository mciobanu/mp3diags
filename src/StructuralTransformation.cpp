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


#include  "StructuralTransformation.h"

#include  "Helpers.h"
#include  "Mp3Manip.h"
#include  "MpegStream.h"
#include  "Id3V230Stream.h"
#include  "Id3V240Stream.h"


using namespace std;
using namespace pearl;



//===============================================================================================================
//===============================================================================================================
//===============================================================================================================


// locates a frame that starts about 2 seconds before pos and is compatible with pAudio;
// if the stream is too short or such a frame can't be found, it returns pos;
// doesn't change the pointer for "in";
static streampos findNearMpegFrameAtLeft(streampos pos, istream& in, MpegStream* pAudio)
{
    StreamStateRestorer rst (in);

    streampos posRes (pos);
    in.seekg(pos);
    int nSize (pAudio->getBitrate()/8*2); // about 2 seconds
    if (posRes > nSize)
    {
        posRes -= nSize;
    }
    else
    {
        posRes = 0;
    }

    in.seekg(posRes);
    if (pAudio->findNextCompatFrame(in, pos))
    {
        posRes = in.tellg();
        return posRes;
    }
    return pos;
}


// locates a frame that starts about 2 seconds after pos and is compatible with pAudio;
// if the stream is too short or such a frame can't be found, it returns pos;
// doesn't change the pointer for "in";
static streampos findNearMpegFrameAtRight(streampos pos, istream& in, MpegStream* pAudio)
{
    StreamStateRestorer rst (in);

    streampos posRes (pos);
    in.seekg(pos);
    int nSize (pAudio->getBitrate()/8*2); // about 2 seconds
    posRes += nSize;
    if (posRes > getSize(in))
    {
        posRes = pos; // ttt3 perhaps should go a little farther
    }

    in.seekg(posRes);
    streampos posLimit (pos);
    posLimit += nSize*2;
    if (pAudio->findNextCompatFrame(in, posLimit))
    {
        posRes = in.tellg();
        return posRes;
    }
    return pos;
}




//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



/*static*/ bool SingleBitRepairer::isUnknown(const DataStream* p)
{
    // UnknownDataStreamBase covers UnknownDataStream, BrokenDataStream, UnsupportedDataStream and TruncatedMpegDataStream, which is probably OK; doesn't matter much if it returns true for types that it shouldn't - there's just a small performance penalty
    return 0 != dynamic_cast<const UnknownDataStreamBase*>(p);
}

/*override*/ Transformation::Result SingleBitRepairer::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const string& strOrigSrcName, string& strTempName)
{
    //CB_ASSERT(UnknownDataStreamBase::BEGIN_SIZE >= x)
    //if (strOrigSrcName in tmp or proc) throw; //ttt1
    const vector<DataStream*>& vpStreams (h.getStreams());
    for (int i = 0, n = cSize(vpStreams); i < n - 2; ++i)
    {
        DataStream* pDataStream (vpStreams[i]);
        MpegStream* pAudio (dynamic_cast<MpegStream*>(pDataStream));
        if (0 != pAudio)
        {
            UnknownDataStreamBase* pFrameToChange (dynamic_cast<UnknownDataStreamBase*>(vpStreams[i + 1]));
            if (0 != pFrameToChange)
            {
                for (int k = 0; k < 32; ++k)
                {
                    try
                    {
                        NoteColl notes (100);

                        char bfr [4];
                        const char* q (pFrameToChange->getBegin());
                        bfr[0] = q[0]; bfr[1] = q[1]; bfr[2] = q[2]; bfr[3] = q[3];
                        bfr[k / 8] ^= 1 << (k % 8);

                        MpegFrameBase frame (notes, pFrameToChange->getPos(), bfr);

                        if (pAudio->isCompatible(frame))
                        {
                            streampos posNewFrameEnd (frame.getPos());
                            posNewFrameEnd += frame.getSize();

                            int j (i + 1);
                            for (; j < n; ++j)
                            {
                                UnknownDataStreamBase* pLastReplacedFrame (dynamic_cast<UnknownDataStreamBase*>(vpStreams[j]));
                                if (0 == pLastReplacedFrame) { break; }
                                streampos posLastReplacedFrameEnd (pLastReplacedFrame->getPos());
                                posLastReplacedFrameEnd += pLastReplacedFrame->getSize();

                                if (posLastReplacedFrameEnd == posNewFrameEnd)
                                {
                                    //qDebug("found");

                                    ifstream_utf8 in (h.getName().c_str(), ios::binary);

                                    { // comp
                                        switch (transfConfig.getCompAction())
                                        {
                                        case TransfConfig::TRANSF_DONT_CREATE: break;
                                        case TransfConfig::TRANSF_CREATE:
                                            {
                                                streampos posCompBeg (findNearMpegFrameAtLeft(pFrameToChange->getPos(), in, pAudio));

                                                streampos posCompEnd (findNearMpegFrameAtRight(posLastReplacedFrameEnd, in, pAudio));

                                                in.seekg(posCompBeg);

                                                string strCompBefore;
                                                string strCompAfter;
                                                transfConfig.getCompNames(strOrigSrcName, getActionName(), strCompBefore, strCompAfter);
                                                { // tmp before
                                                    ofstream_utf8 out (strCompBefore.c_str(), ios::binary);
                                                    //partialCopy(in, out, 10000);
                                                    //appendFilePart(in, out, pos, 2*nHalfSize);
                                                    appendFilePart(in, out, posCompBeg, posCompEnd - posCompBeg);
                                                }

                                                { // tmp after
                                                    ofstream_utf8 out (strCompAfter.c_str(), ios::binary);
                                                    appendFilePart(in, out, posCompBeg, posCompEnd - posCompBeg);
                                                    out.seekp(pFrameToChange->getPos() - posCompBeg);
                                                    out.write(bfr, 4);
                                                }
                                            }
                                            break;

                                        default: CB_ASSERT (false);
                                        }
                                    }

                                    { // temp
                                        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
                                        ofstream_utf8 out (strTempName.c_str(), ios::binary);
                                        in.seekg(0);

                                        for (int i1 = 0; i1 < n; ++i1)
                                        {
                                            DataStream* q (vpStreams[i1]);
                                            q->copy(in, out);
                                        }

                                        out.seekp(pFrameToChange->getPos());
                                        out.write(bfr, 4);
                                        CB_CHECK1 (out, WriteError());
                                    }

                                    return CHANGED;
                                }
                                if (posLastReplacedFrameEnd > posNewFrameEnd) { break; }
                            }
                        }

                    }
                    catch (const MpegFrameBase::NotMpegFrame&)
                    {
                    }
                }
            }
        }

        /*BrokenMpegDataStream* pBroken (dynamic_cast<BrokenMpegDataStream*>(pDataStream));
        UnknownDataStream* pUnknown (dynamic_cast<UnknownDataStream*>(pDataStream));
        TruncatedMpegDataStream* pTruncated (dynamic_cast<TruncatedMpegDataStream*>(pDataStream));*/
    }

    return NOT_CHANGED;
}



//================================================================================================================================
//================================================================================================================================
//================================================================================================================================


/*override*/ Transformation::Result InnerNonAudioRemover::apply(const Mp3Handler& h, const TransfConfig& cfg, const std::string& strOrigSrcName, std::string& strTempName)
{
    setupDiscarded(h);
    Result eRes (GenericRemover::apply(h, cfg, strOrigSrcName, strTempName));
    return NOT_CHANGED == eRes ? NOT_CHANGED : CHANGED_NO_RECALL;
}


void InnerNonAudioRemover::setupDiscarded(const Mp3Handler& h)
{
    m_spStreamsToDiscard.clear();

    const vector<DataStream*>& vpStreams (h.getStreams());
    int nFirstAudioPos (-1);
    int nLastAudioPos (-1);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        DataStream* pDataStream (vpStreams[i]);
//qDebug("%s", pDataStream->getDisplayName());
//?? probably want to remove garbage between vbri and audio, but definitely not the 16 bytes between xing and audio
        if (0 != dynamic_cast<MpegStream*>(pDataStream) || 0 != dynamic_cast<VbriStream*>(pDataStream))// ttt2 should consider these? : || 0 != dynamic_cast<XingStreamBase*>(pDataStream) || 0 != dynamic_cast<VbriStream*>(pDataStream)) // with them, there's a risk of destroying Xing headers created by Mp3Fixer
        {
            if (-1 == nFirstAudioPos)
            {
                nFirstAudioPos = i;
            }
            nLastAudioPos = i;
        }
    }

    if (nFirstAudioPos == nLastAudioPos) { return; }

    for (int i = nFirstAudioPos + 1; i < nLastAudioPos; ++i)
    {
        if (0 == dynamic_cast<MpegStream*>(vpStreams[i]) && 0 == dynamic_cast<VbriStream*>(vpStreams[i]))
        {
            m_spStreamsToDiscard.insert(vpStreams[i]);
        }
    }
}
//ttt1 see about unsynch audio (when some frames use data from other frames) ; http://www.hydrogenaudio.org/forums/index.php?showtopic=35654&st=25&p=354991&#entry354991

/*override*/ bool InnerNonAudioRemover::matches(DataStream* p) const
{
    bool b (m_spStreamsToDiscard.count(p) > 0);
    CB_ASSERT (!b || (0 == dynamic_cast<MpegStream*>(p) && 0 == dynamic_cast<VbriStream*>(p)));
    return b;
}



//================================================================================================================================
//================================================================================================================================
//================================================================================================================================




/*override*/ Transformation::Result GenericRemover::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    const vector<DataStream*>& vpStreams (h.getStreams());
    bool bFoundMatch (false);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        if (matches(vpStreams[i]))
        {
            bFoundMatch = true;
            break;
        }
    }

    if (!bFoundMatch)
    {
        return NOT_CHANGED;
    }

//ttt1 see InnerNonAudioRemover: Lambo has an error in mplayer, because of frame dependency

    ifstream_utf8 in (h.getName().c_str(), ios::binary);

    { // comp
        switch (transfConfig.getCompAction())
        {
        case TransfConfig::TRANSF_DONT_CREATE: break;
        case TransfConfig::TRANSF_CREATE:
            {
                in.seekg(0);

                for (int i = 0, n = cSize(vpStreams); i < n; ++i)
                {
                    DataStream* p (vpStreams[i]);

                    if (matches(p))
                    {
                        MpegStream* pPrevAudio (i > 0 ? dynamic_cast<MpegStream*>(vpStreams[i - 1]) : 0);
                        MpegStream* pNextAudio (i < n - 1 ? dynamic_cast<MpegStream*>(vpStreams[i + 1]) : 0);

                        string strCompBefore;
                        string strCompAfter;
                        transfConfig.getCompNames(strOrigSrcName, getActionName(), strCompBefore, strCompAfter);

                        streampos posCompBeg (0 != pPrevAudio ? findNearMpegFrameAtLeft(pPrevAudio->getEnd(), in, pPrevAudio) : p->getPos());
                        streampos posCompEnd (0 != pNextAudio ? findNearMpegFrameAtRight(pNextAudio->getPos(), in, pNextAudio) : p->getEnd());

                        { // comp before
                            ofstream_utf8 out (strCompBefore.c_str(), ios::binary);
                            appendFilePart(in, out, posCompBeg, posCompEnd - posCompBeg);
                        }

                        { // comp after
                            if (0 != pPrevAudio || 0 != pNextAudio)
                            {
                                ofstream_utf8 out (strCompAfter.c_str(), ios::binary);
                                appendFilePart(in, out, posCompBeg, p->getPos() - posCompBeg);
                                appendFilePart(in, out, p->getEnd(), posCompEnd - p->getEnd());
                            }
                        }
                    }
                }
            }
            break;

        default: CB_ASSERT (false);
        }
    }

    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* pDataStream (vpStreams[i]);
            if (!matches(pDataStream))
            {
                //qDebug("copying %d for %s", (int)pDataStream->getSize(), pDataStream->getDisplayName());
                pDataStream->copy(in, out);
            }
            /*else
            {
                qDebug("skipping %d for %s", (int)pDataStream->getSize(), pDataStream->getDisplayName());
            }*/
        }
    }

    return CHANGED; // CHANGED_NO_RECALL would be ok in some cases but it's hard to tell when
}


/*override*/ bool UnknownDataStreamRemover::matches(DataStream* p) const { return 0 != dynamic_cast<UnknownDataStream*>(p); }
/*override*/ bool BrokenDataStreamRemover::matches(DataStream* p) const { return 0 != dynamic_cast<BrokenDataStream*>(p); }
/*override*/ bool UnsupportedDataStreamRemover::matches(DataStream* p) const { return 0 != dynamic_cast<UnsupportedDataStream*>(p); }
/*override*/ bool TruncatedMpegDataStreamRemover::matches(DataStream* p) const { return 0 != dynamic_cast<TruncatedMpegDataStream*>(p); }
/*override*/ bool NullStreamRemover::matches(DataStream* p) const { return 0 != dynamic_cast<NullDataStream*>(p); }

/*override*/ bool BrokenId3V2Remover::matches(DataStream* pDataStream) const
{
    BrokenDataStream* p (dynamic_cast<BrokenDataStream*>(pDataStream));
    if (0 == p) { return false; }
    string strBaseName (p->getBaseName());
    return strBaseName == Id3V230Stream::getClassDisplayName() || strBaseName == Id3V240Stream::getClassDisplayName();
}

/*override*/ bool UnsupportedId3V2Remover::matches(DataStream* pDataStream) const
{
    UnsupportedDataStream* p (dynamic_cast<UnsupportedDataStream*>(pDataStream));
    if (0 == p) { return false; }
    string strBaseName (p->getBaseName());
    return strBaseName == Id3V2StreamBase::getClassDisplayName();
}


/*override*/ bool MultipleId3StreamRemover::matches(DataStream* p) const
{
    return m_spStreamsToDiscard.count(p) > 0;
}

void MultipleId3StreamRemover::setupDiscarded(const Mp3Handler& h)
{
    m_spStreamsToDiscard.clear();
    vector<DataStream*> v1, v2;

    const vector<DataStream*>& vpStreams (h.getStreams());
    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        DataStream* p (vpStreams[i]);
        if (0 != dynamic_cast<Id3V2StreamBase*>(p))
        {
            v2.push_back(p);
        }

        if (0 != dynamic_cast<Id3V1Stream*>(p))
        {
            v1.push_back(p);
        }
    }

    if (cSize(v1) > 1)
    {
        m_spStreamsToDiscard.insert(v1.begin(), v1.end() - 1);
    }

    if (cSize(v2) > 1)
    {
        m_spStreamsToDiscard.insert(v2.begin() + 1, v2.end());
    }
}



/*override*/ bool MismatchedXingRemover::matches(DataStream* p) const
{
    return m_spStreamsToDiscard.count(p) > 0;
}

void MismatchedXingRemover::setupDiscarded(const Mp3Handler& h)
{
    m_spStreamsToDiscard.clear();

    const vector<DataStream*>& vpStreams (h.getStreams());
    for (int i = 0, n = cSize(vpStreams); i < n - 1; ++i)
    {
        XingStreamBase* pXing (dynamic_cast<XingStreamBase*>(vpStreams[i]));
        if (0 != pXing)
        {
            MpegStream* pAudio (dynamic_cast<MpegStream*>(vpStreams[i + 1]));
            if (0 != pAudio && pXing->getFrameCount() != pAudio->getFrameCount())
            {
                m_spStreamsToDiscard.insert(pXing);
            }
        }
    }
}




/*override*/ bool Id3V1Remover::matches(DataStream* pDataStream) const
{
    Id3V1Stream* p (dynamic_cast<Id3V1Stream*>(pDataStream));
    return 0 != p;
}


//================================================================================================================================
//================================================================================================================================
//================================================================================================================================



/*override*/ Transformation::Result TruncatedAudioPadder::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const string& strOrigSrcName, string& strTempName)
{
    const vector<DataStream*>& vpStreams (h.getStreams());
    ifstream_utf8 in (h.getName().c_str(), ios::binary);
    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        TruncatedMpegDataStream* pTruncStream (dynamic_cast<TruncatedMpegDataStream*>(vpStreams[i]));
        if (0 != pTruncStream)
        {
            int nPaddingSize (pTruncStream->getExpectedSize() - pTruncStream->getSize());

            { // comp
                CB_ASSERT (i > 0); // !!! a stream can be "truncated audio" only if it follows another audio; even if the user is allowed to erase streams at will, after removing an audio stream and rescanning what was "truncated audio" will become something else (perhaps "broken audio")
                MpegStream* pAudio (dynamic_cast<MpegStream*>(vpStreams[i - 1]));

                CB_ASSERT (0 != pAudio);
                switch (transfConfig.getCompAction())
                {
                case TransfConfig::TRANSF_DONT_CREATE: break;
                case TransfConfig::TRANSF_CREATE:
                    {
                        streampos posCompBeg (findNearMpegFrameAtLeft(pTruncStream->getPos(), in, pAudio));

                        streampos posCompEnd (findNearMpegFrameAtRight(pTruncStream->getPos(), in, pAudio));
                        CB_ASSERT(posCompEnd >= pTruncStream->getPos());

                        in.seekg(posCompBeg);

                        string strCompBefore;
                        string strCompAfter;
                        transfConfig.getCompNames(strOrigSrcName, getActionName(), strCompBefore, strCompAfter);
                        { // tmp before
                            ofstream_utf8 out (strCompBefore.c_str(), ios::binary);
                            //partialCopy(in, out, 10000);
                            //appendFilePart(in, out, pos, 2*nHalfSize);
                            appendFilePart(in, out, posCompBeg, posCompEnd - posCompBeg);
                        }

                        { // tmp after
                            ofstream_utf8 out (strCompAfter.c_str(), ios::binary);
                            appendFilePart(in, out, posCompBeg, pTruncStream->getPos() - posCompBeg);
                            writeZeros(out, nPaddingSize);
                            appendFilePart(in, out, pTruncStream->getEnd(), posCompEnd - pTruncStream->getEnd());
                        }
                    }
                    break;

                default: CB_ASSERT (false);
                }
            }

            { // temp
                transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
                ofstream_utf8 out (strTempName.c_str(), ios::binary);
                in.seekg(0);

                for (int i1 = 0; i1 < n; ++i1)
                {
                    DataStream* q (vpStreams[i1]);
                    q->copy(in, out);
                    if (q == pTruncStream)
                    {
                        writeZeros(out, nPaddingSize);
                    }
                }
            }

            return CHANGED;
        }
    }

    return NOT_CHANGED;
}


//================================================================================================================================
//================================================================================================================================
//================================================================================================================================

//ttt1 in a way this could take care of Xing headers for CBR streams as well, but it doesn't seem the best place to do it, especially as we ignore most of the data in the Lame header and "restoring a header" means just putting back byte count and frame count
/*override*/ Transformation::Result VbrRepairerBase::repair(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName, bool bForceRebuild)
{
    const vector<DataStream*>& vpStreams (h.getStreams());
    ifstream_utf8 in (h.getName().c_str(), ios::binary);
    set<int> sVbriPos;
    int nAudioPos (-1);
    set<int> sXingPos;
    int n (cSize(vpStreams));
    int nXingPos (-1);
    XingStreamBase* pXingStreamBase (0);
    for (int i = 0; i < n; ++i)
    {
        MpegStream* pAudio (dynamic_cast<MpegStream*>(vpStreams[i]));
        if (0 != pAudio)
        {
            nAudioPos = i;
            break;
        }

        VbriStream* pVbriStream (dynamic_cast<VbriStream*>(vpStreams[i]));
        if (0 != pVbriStream)
        {
            sVbriPos.insert(i);
        }

        XingStreamBase* q (dynamic_cast<XingStreamBase*>(vpStreams[i]));
        if (0 != q)
        {
            sXingPos.insert(i);
            nXingPos = i;
            pXingStreamBase = q;
        }
    }

    if (-1 == nAudioPos) { return NOT_CHANGED; } // no audio
    MpegStream* pAudio (dynamic_cast<MpegStream*>(vpStreams[nAudioPos]));
    if (!pAudio->isVbr()) { return NOT_CHANGED; } // CBR audio

    bool bXingOk (1 == cSize(sXingPos) && 1 == sXingPos.count(nAudioPos - 1) && pXingStreamBase->matches(pAudio));
    if (!bForceRebuild && sVbriPos.empty() && bXingOk) { return NOT_CHANGED; } // exit if there's no VBRI and there's one matching Xing right before the audio


    bool bRepairMp3Fixer (false);
    bool bRemoveXing (true); // if true, existing headers are removed
    bool bAddXing (true); // if true, a header is added before the first Audio

    if (1 == cSize(sXingPos) && nXingPos < n - 2 && pXingStreamBase->isBrokenByMp3Fixer(vpStreams[nXingPos + 1], vpStreams[nXingPos + 2]))
    {
        // ttt2 see also http://www.kde-apps.org/content/show.php/Mp3Fixer?content=31539 for a bug that makes mp3fix.rb write its fix at a wrong address for mono files, but that's not going to be fixed now; at least don't try to fix this on mono files (or better: fix only stereo mpeg1 L III)

        bRemoveXing = bAddXing = false;
        bRepairMp3Fixer = true;
    }

    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        int i (0);
        for (; i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            if (0 != dynamic_cast<VbriStream*>(p))
            { // nothing to do; this gets discarded
            }
            else if (0 != dynamic_cast<XingStreamBase*>(p))
            {
                if (bRepairMp3Fixer)
                {
                    XingStreamBase* pXing (dynamic_cast<XingStreamBase*>(p));
                    MpegStream* pAudio (dynamic_cast<MpegStream*>(vpStreams[i + 2]));
                    CB_ASSERT(0 != pXing && 0 != pAudio); // that's what isBrokenByMp3Fixer() is about

                    int nOffs (pXing->getFirstFrame().getSideInfoSize() + MpegFrame::MPEG_FRAME_HDR_SIZE);

                    createXing(out, pXing->getFirstFrame(), pAudio->getFrameCount() + 1, pAudio->getSize() + pXing->getSize());

                    appendFilePart(in, out, p->getPos(), nOffs);
                    streampos pos (p->getPos());
                    pos += 16 + nOffs;
                    appendFilePart(in, out, pos, p->getSize() - nOffs - 16);
                }
                else if (!bRemoveXing)
                {
                    p->copy(in, out);
                }
            }
            else if (0 != dynamic_cast<MpegStream*>(p))
            {
                if (bAddXing)
                {
                    dynamic_cast<MpegStream*>(p)->createXing(out);
                }
                break;
            }
            else
            { // any other stream
                p->copy(in, out);
            }
        }

        for (; i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            p->copy(in, out);
        }
    }

    return CHANGED_NO_RECALL;
}

//ttt0 deal with 05 Are You Gonna Go My Way.mp3 - mismatched xing but cbr

//================================================================================================================================
//================================================================================================================================
//================================================================================================================================

/*override*/ Transformation::Result VbrRepairer::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    return repair(h, transfConfig, strOrigSrcName, strTempName, DONT_FORCE_REBUILD);
}

/*override*/ Transformation::Result VbrRebuilder::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    return repair(h, transfConfig, strOrigSrcName, strTempName, FORCE_REBUILD);
}



//================================================================================================================================
//================================================================================================================================
//================================================================================================================================

/* ttt1 see about file flushing:
    http://support.microsoft.com/default.aspx/kb/148505
    - fdatasync() - like fsync() but doesn't change metadata (e.g. mtime) so it's faster
    - since the C++ Library has nothing to do flushing, perhaps this would work: standalone "commit(const ofstream_utf8&)" and/or "commit(const string&)" ; also, we don't want to commit all the files; or perhaps "commit(ofstream_utf8&)" is needed, which would first close the file
    - there's also out.rdbud()->pubsync(), but what it does is OS-dependent
    - use external disk / flash, to see the LED, for testing
*/

