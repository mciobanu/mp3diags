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


#ifndef Mp3HandlerH
#define Mp3HandlerH


#include  "fstream_unicode.h"
#include  <vector>

#include  "SerSupport.h"
#include  <boost/serialization/split_free.hpp>

#include  "Notes.h"
#include  "ThreadRunnerDlgImpl.h"

class Id3V2StreamBase;
class Id3V230Stream;
class Id3V240Stream;
class Id3V1Stream;

class LameStream;
class XingStreamBase;
class VbriStream;
class MpegStream;

class ApeStream;
class LyricsStream;

class NullDataStream;
class UnknownDataStream;
class BrokenDataStream;
class UnsupportedDataStream;
class TruncatedMpegDataStream;

class DataStream;

PausableThread* getSerThread(); //ttt1 global function

struct QualThresholds
{
    int m_nStereoCbr;           // below this bitrate for stereo CBR a warning is generated; value is in bps
    int m_nJointStereoCbr;      // below this bitrate for joint stereo CBR a warning is generated
    int m_nDoubleChannelCbr;    // below this bitrate for dual channel CBR a warning is generated; half of this value is used for mono streams
    int m_nStereoVbr;           // below this bitrate for stereo VBR a warning is generated
    int m_nJointStereoVbr;      // below this bitrate for joint stereo VBR a warning is generated
    int m_nDoubleChannelVbr;    // below this bitrate for dual channel VBR a warning is generated; half of this value is used for mono streams

    // ttt2 add Lame header info analysis, which might give a better idea about quality than simple bitrate
};



// the details for an MP3 file
class Mp3Handler
{
    StringWrp* m_pFileName; // couldn't get Boost ser to allow an object, rather than a pointer; tried BOOST_CLASS_TRACKING; the reason seems to be that when using an object, it is also saved by the frames, as a pointer; so they all should be pointers;

    Id3V230Stream* m_pId3V230Stream;
    Id3V240Stream* m_pId3V240Stream;
    Id3V1Stream* m_pId3V1Stream;

    LameStream* m_pLameStream;
    XingStreamBase* m_pXingStream;
    VbriStream* m_pVbriStream;
    MpegStream* m_pMpegStream;

    ApeStream* m_pApeStream;
    LyricsStream* m_pLyricsStream;

    std::vector<NullDataStream*> m_vpNullStreams;
    std::vector<UnknownDataStream*> m_vpUnknownStreams;
    std::vector<BrokenDataStream*> m_vpBrokenStreams;
    std::vector<UnsupportedDataStream*> m_vpUnsupportedStreams;
    std::vector<TruncatedMpegDataStream*> m_vpTruncatedMpegStreams;

    std::vector<DataStream*> m_vpAllStreams;
    ifstream_utf8* m_pIn; // this isn't used after the constructor completes; however, many streams have a StreamStateRestorer member, which does this on its destructor: it restores the stream position if the stream's constructor fails and clears the errors otherwise, assuming that the stream pointer is non-0; m_pIn is set to 0 after Mp3Handler's constructor completes, so the restorers of the successfuly built streams don't do anything when the streams get destroyed, because they see a null pointer;

    //bool m_bHasId3V2;
    //MpegFrame m_firstFrame;
    //const char* szFileName;
    //bool m_bVbr;
    //int m_nBitrate;
    std::streampos m_posEnd;

    long long m_nSize, m_nTime;

    NoteColl m_notes; // owned pointers

    void parse(ifstream_utf8&);
    void analyze(const QualThresholds& qualThresholds); // checks the streams for issues (missing ID3V2, Unknown streams, inconsistencies, ...)

    //void checkMpegAudio();

    //void checkEqualFrames(std::streampos pos);
    //void findFirstFrame(std::streampos& pos);

    void checkLastFrameInMpegStream(ifstream_utf8& in); // what looks like the last frame in an MPEG stream may actually be truncated and somewhere inside it an ID3V1 or Ape tag may actually begin; if that's the case, that "frame" is removed from the stream; then most likely an "Unknown" stream will be detected, followed by an ID3V1 or Ape stream

    void reloadId3V2Hlp();

public:
    Mp3Handler(const std::string& strFileName, bool bStoreTraceNotes, const QualThresholds& qualThresholds);
    ~Mp3Handler();

    //void copyMpeg(std::istream& in, std::ostream& out);

    //void logStreamInfo() const;
    //void logStreamInfo(std::ostream& out) const;

    const NoteColl& getNotes() const { return m_notes; }
    const std::vector<DataStream*>& getStreams() const { return m_vpAllStreams; }
    const std::string& getName() const { return m_pFileName->s; }
    QString getUiName() const; // uses native separators

    long long getSize() const;
    std::string getShortName() const;
    std::string getDir() const;

    const Id3V2StreamBase* getId3V2Stream() const;

    bool id3V2NeedsReload(bool bConsiderTime) const;
    bool needsReload() const; // if the underlying file seems changed (or removed); looks at time and size, as well as FastSaveWarn and Notes::getMissingNote();

    void sortNotes() { m_notes.sort(); } // this is needed when loading from the disk, if unknown (most likely obsolete) notes are found

    // removes the ID3V2 tag and the notes associated with it and scans the file again, but only the new ID3V2 tag;
    // asserts that there was an existing ID3V2 tag at the beginning and it had the same size as the new one;
    // this isn't really const, but it seems better to have a const_cast in the only place where it is needed rather than remove the const restriction from many places
    void reloadId3V2() const;

    struct FileNotFound {};

private:
    friend class boost::serialization::access;

    Mp3Handler() {} // serialization-only constructor
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & m_pFileName;

        ar & m_pId3V230Stream;
        ar & m_pId3V240Stream;
        ar & m_pId3V1Stream;

        ar & m_pLameStream;
        ar & m_pXingStream;
        ar & m_pVbriStream;
        ar & m_pMpegStream;

        ar & m_pApeStream;
        ar & m_pLyricsStream;

        ar & m_vpNullStreams;
        ar & m_vpUnknownStreams;
        ar & m_vpBrokenStreams;
        ar & m_vpUnsupportedStreams;
        ar & m_vpTruncatedMpegStreams;

        ar & m_vpAllStreams;

        ar & m_posEnd;

        ar & m_nSize;
        ar & m_nTime;

        ar & m_notes;

        PausableThread* pThread (getSerThread());
        if (0 != pThread)
        {
            StrList l;
            l.push_back(QString::fromUtf8(m_pFileName->s.c_str()));
            pThread->emitStepChanged(l);
        }
    }
};


struct CmpMp3HandlerPtrByName
{
    /*bool operator()(const std::string& strName1, const std::string& strName2) const
    {
        return cmp(strName1, strName2);
    }

    bool operator()(const std::string& strName1, const Mp3Handler* p2) const
    {
        return cmp(strName1, p2->getName());
    }*/

    bool operator()(const Mp3Handler* p1, const std::string& strName2) const;
    bool operator()(const Mp3Handler* p1, const Mp3Handler* p2) const;

private:
    bool cmp(const std::string& strName1, const std::string& strName2) const;
};



// finds the position where the next ID3V2, MPEG, Xing, Lame or Ape frame begins; "pos" is not considered; the search starts at pos+1; throws if no frame is found
std::streampos getNextStream(std::istream& in, std::streampos pos);




#endif // #ifndef Mp3HandlerH

