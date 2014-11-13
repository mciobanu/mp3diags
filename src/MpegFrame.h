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


#ifndef MpegFrameH
#define MpegFrameH

#include  <iosfwd>
#include  <string>
#include  <stdexcept>

#include  "SerSupport.h"
#include  "CbException.h"


struct Note;
class NoteColl;

// based on http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
struct MpegFrameBase
{
    enum Version { MPEG1, MPEG2 };
    enum Layer { LAYER1, LAYER2, LAYER3 };
    enum ChannelMode { STEREO, JOINT_STEREO, DUAL_CHANNEL, SINGLE_CHANNEL };

    enum { MPEG_FRAME_HDR_SIZE = 4 };

private:

    //void init(const unsigned char* pHeader, std::istream* pIn, std::streampos pos, int nRelPos);

    //bool hasXingHdr();
    //bool hasVbriHdr(); // Amarok doesn't seem to care about this, so the Xing header must be used anyway for VBR
    void init(NoteColl& notes, const char* bfr);

    char m_header[4];

protected:

    Version m_eVersion;
    Layer m_eLayer;
    int m_nBitrate;
    int m_nFrequency;
    int m_nPadding;
    ChannelMode m_eChannelMode;

    int m_nSize;
    bool m_bCrc;

    std::streampos m_pos;

public:
    //MpegFrame(const unsigned char* pHeader, std::istream& in, std::streampos pos, int nRelPos);
    //MpegFrame(const char* pHeader, std::istream& in, std::streampos pos, int nRelPos);
    //MpegFrame(std::istream& in, std::streampos pos);
    //MpegFrame(std::istream& in, std::streampos pos);
    MpegFrameBase(NoteColl& notes, std::istream& in);

    MpegFrameBase();
    MpegFrameBase(NoteColl& notes, std::streampos pos, const char* bfr);

    std::ostream& write(std::ostream& out) const;


    Version getVersion() const { return m_eVersion; }
    Layer getLayer() const { return m_eLayer; }
    ChannelMode getChannelMode() const { return m_eChannelMode; }
    int getBitrate() const { return m_nBitrate; }
    int getFrequency() const { return m_nFrequency; }
    int getSize() const { return m_nSize; } // total size, including the header
    bool getCrcUsage() const { return m_bCrc; }
    std::streampos getPos() const { return m_pos; }

    const char* getSzVersion() const;
    const char* getSzLayer() const;
    const char* getSzChannelMode() const;

    const char* getHeader() const { return m_header; }
    int getSideInfoSize() const; // needed by Xing

#ifdef GENERATE_TOC
    MpegFrameBase getBigBps() const; // returns a "similar" frame to "this" but with a bigger bps, so it can hold a Xing TOC
#endif

    DEFINE_CB_EXCP(NotMpegFrame); // exception

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { CB_THROW1(CbRuntimeError, "invalid version of serialized file"); }

        //ar & boost::serialization::base_object<DataStream>(*this);
        ar & m_header;

        ar & m_eVersion;
        ar & m_eLayer;
        ar & m_nBitrate;
        ar & m_nFrequency;
        ar & m_nPadding;
        ar & m_eChannelMode;

        ar & m_nSize;
        ar & m_bCrc;

        ar & m_pos;
    }
};


struct MpegFrame : public MpegFrameBase
{
    DEFINE_CB_EXCP1(PrematurelyEndedMpegFrame, std::string, m_strInfo);

    MpegFrame(NoteColl& notes, std::istream& in);
    MpegFrame() {}
};


std::ostream& operator<<(std::ostream& out, const MpegFrameBase& frm);




#endif // ifndef MpegFrameH
