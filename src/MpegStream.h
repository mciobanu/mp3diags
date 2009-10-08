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


#ifndef MpegStreamH
#define MpegStreamH

#include  "DataStream.h"
#include  "MpegFrame.h"
#include  "Helpers.h"


// reads a single frame
class MpegStreamBase : public DataStream
{
    StreamStateRestorer* m_pRst;
protected:
    MpegStreamBase() : m_pRst(0) {} // serialization-only constructor

    MpegFrame m_firstFrame;

    std::streampos m_pos;

    MpegStreamBase(int nIndex, NoteColl& notes, std::istream& in);

    void setRstOk()
    {
        m_pRst->setOk();
        releasePtr(m_pRst); // destroy and set to 0
    }
public:
    ~MpegStreamBase();
    /*override*/ void copy(std::istream& in, std::ostream& out); // copies "m_firstFrame"; derived classes should decide if it's good enough for them

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return m_firstFrame.getSize(); }

    const MpegFrame& getFirstFrame() const { return m_firstFrame; }

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<DataStream>(*this);

        ar & m_firstFrame;

        ar & m_pos;
        //ar & m_pRst; // 2009.04.24 - shouldn't be needed, because it's only used by the "real" constructor
    }
};



class MpegStream : public MpegStreamBase
{
    std::streamoff m_nSize;
    int m_nBitrate;
    bool m_bVbr;
    int m_nFrameCount;
    std::streampos m_posLastFrame;
    MpegFrame m_lastFrame;
    long long m_nTotalBps;
    bool m_bRemoveLastFrameCalled;

    enum { MIN_FRAME_COUNT = 10 };
public:
    MpegStream(int nIndex, NoteColl& notes, std::istream& in);
    /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_NAME("MPEG Audio");
    /*override*/ std::string getInfo() const;

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return m_nSize; }

    int getBitrate() const { return  m_nBitrate; }
    bool isVbr() const { return  m_bVbr; }

    using MpegStreamBase::getFirstFrame;

    std::streampos getLastFramePos() const { return m_posLastFrame; }
    void removeLastFrame(); // this can only be called once; the second call will throw (it's harder and quite pointless to allow more than one call)

    bool isCompatible(const MpegFrameBase& frame);
    int getFrameCount() const { return m_nFrameCount; }

    MpegFrame::ChannelMode getChannelMode() const { return m_firstFrame.getChannelMode(); }

    bool findNextCompatFrame(std::istream& in, std::streampos posMax); // moves the read pointer to the first frame compatible with the stream; returns "false" if no such frame is found

    std::string getDuration() const;

    struct StreamTooShort // exception thrown if a stream has less than 10 frames
    {
        std::string m_strInfo;
        int m_nFrameCount;
        StreamTooShort(const std::string& strInfo, int nFrameCount) : m_strInfo(strInfo), m_nFrameCount(nFrameCount) {}
    };

    struct UnknownHeader {}; // exception thrown if the first frame seems to be part of a (Lame) header, having a different framerate in an otherwise CBR stream

    void createXing(std::ostream& out);

private:
    friend class boost::serialization::access;
    MpegStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<MpegStreamBase>(*this);

        ar & m_nSize;
        ar & m_nBitrate;
        ar & m_bVbr;
        ar & m_nFrameCount;
        ar & m_posLastFrame;
        ar & m_lastFrame;
        ar & m_nTotalBps;
        ar & m_bRemoveLastFrameCalled;
    }
};


void createXing(std::ostream& out, const MpegFrame& frame, int nFrameCount, std::streamoff nStreamSize); // throws if it can't write to the disk

class XingStreamBase : public MpegStreamBase
{
    unsigned char m_cFlags;
    int m_nFrameCount;
    int m_nByteCount;
    char m_toc [100];
    int m_nQuality;
protected:
    void getXingInfo(std::ostream&) const;
    XingStreamBase() {} // serialization-only constructor
public:
    XingStreamBase(int nIndex, NoteColl& notes, std::istream& in);
    // /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_NAME("Xing Header");
    /*override*/ std::string getInfo() const;
    std::string getInfoForXml() const;

    bool matchesStructure(const MpegStream&) const; // checks that there is a metch for version, layer, frequency
    bool matches(const MpegStream&) const; // checks that there is a metch for version, layer, frequency and frame count
    bool matches(const DataStream* pNext) const; // checks that pNext is MpegStream* in addition to matches(const MpegStream&)

    bool isBrokenByMp3Fixer(const DataStream* pNext, const DataStream* pAfterNext) const;

    const MpegFrame& getFrame() const { return m_firstFrame; }
    int getFrameCount() const { return m_nFrameCount; }

    using MpegStreamBase::getFirstFrame;

    struct NotXingStream {};

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<MpegStreamBase>(*this);

        ar & m_cFlags;
        ar & m_nFrameCount;
        ar & m_nByteCount;
        ar & m_toc;
        ar & m_nQuality;
    }
};

class XingStream : public XingStreamBase
{
public:
    XingStream(int nIndex, NoteColl& notes, std::istream& in);

private:
    friend class boost::serialization::access;
    XingStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<XingStreamBase>(*this);
    }
};


class LameStream : public XingStreamBase //ttt2 read & interpret data from Lame tag
{
public:
    LameStream(int nIndex, NoteColl& notes, std::istream& in);
    // /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_NAME("Lame Header");
    /*override*/ std::string getInfo() const;

    struct NotLameStream {};

private:
    friend class boost::serialization::access;
    LameStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<XingStreamBase>(*this);
    }
};


class VbriStream : public MpegStreamBase // Amarok doesn't seem to care about this, so the Xing header must be used anyway for VBR to work
{
public:
    VbriStream(int nIndex, NoteColl& notes, std::istream& in);
    // /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_NAME("VBRI Header");
    /*override*/ std::string getInfo() const;

    const MpegFrame& getFrame() const { return m_firstFrame; }

    struct NotVbriStream {};

private:
    friend class boost::serialization::access;
    VbriStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<MpegStreamBase>(*this);
    }
};


//-------------------------------------------------------------------------------------------------------

class Id3V1Stream : public DataStream, public TagReader
{
    char m_data [128];
    std::streampos m_pos; // needed only to display info
    enum TestResult { BAD, ZERO_PADDED, SPACE_PADDED, MIXED_PADDED, NOT_PADDED };
    static TestResult checkId3V1String(const char* p, int nSize); // makes sure that a valid string is stored at the address given, meaning no chars smaller than 32; well, except for 0: 0 isn't really valid, as the fields are supposed to be padded with spaces at the right, but some tools use 0 anyway
    static bool isLegal(char c);

    std::string getStr(int nAddr, int nMaxSize) const; // returns the string located at nAddr, removing trailing spaces; the result is in UTF8 format
    enum Version { V10, V11, V11b };
    Version m_eVersion;
    int getCommSize() const;
public:
    Id3V1Stream(int nIndex, NoteColl& notes, std::istream& in);

    /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_RD_NAME("ID3V1");
    /*override*/ std::string getInfo() const;

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return 128; }

    const char* getVersion() const;

    struct NotId3V1Stream {};

    // ================================ TagReader =========================================
    /*override*/ std::string getTitle(bool* pbFrameExists = 0) const;

    /*override*/ std::string getArtist(bool* pbFrameExists = 0) const;

    /*override*/ std::string getTrackNumber(bool* pbFrameExists = 0) const;

    /*override*/ TagTimestamp getTime(bool* pbFrameExists = 0) const;

    /*override*/ std::string getGenre(bool* pbFrameExists = 0) const;

    /*override*/ ImageInfo getImage(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); }

    /*override*/ std::string getAlbumName(bool* pbFrameExists = 0) const;

    /*override*/ std::string getOtherInfo() const;

    /*override*/ SuportLevel getSupport(Feature) const;

private:
    friend class boost::serialization::access;

    Id3V1Stream() {} // serialization-only constructor
    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<DataStream>(*this);

        ar & m_data;
        ar & m_pos;
        ar & m_eVersion;
    }
};

const char* getId3V1Genre(int n); // doesn't throw; for invalid params returns ""


#endif // ifndef MpegStreamH

