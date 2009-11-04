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


#ifndef Id3V230StreamH
#define Id3V230StreamH

#include  "Id3V2Stream.h"


// Frame of an ID3V2.3.0 tag
struct Id3V230Frame : public Id3V2Frame
{
    Id3V230Frame(NoteColl& notes, std::istream& in, std::streampos pos, bool bHasUnsynch, std::streampos posNext, StringWrp* pFileName);
    Id3V230Frame(const std::string& strName, std::vector<char>& vcData); // needed by Id3V230StreamWriter::addBinaryFrame(), so objects created with this constructor don't get serialized; destroys vcData by doing a swap for its own representation
    /*override*/ ~Id3V230Frame();

    /*override*/ bool discardOnChange() const;

private:
    // may return multiple null characters; it's the job of getUtf8String() to deal with them;
    // chars after the first null are considered comments (or after the second null, for TXXX);
    /*override*/ std::string getUtf8StringImpl() const;

    friend class boost::serialization::access;
    Id3V230Frame() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<Id3V2Frame>(*this);
    }
};



class Id3V230Stream : public Id3V2StreamBase
{
public:
    Id3V230Stream(int nIndex, NoteColl& notes, std::istream& in, StringWrp* pFileName, bool bAcceptBroken = false);
    //typedef typename Id3V2Stream<Id3V230Frame>::NotId3V2 NotId3V230;

    DECL_RD_NAME("ID3V2.3.0");

    /*override*/ TagTimestamp getTime(bool* pbFrameExists = 0) const;
    /*override*/ void setTrackTime(const TagTimestamp&) { throw NotSupportedOp(); }

    /*override*/ SuportLevel getSupport(Feature) const;

private:
    friend class boost::serialization::access;
    Id3V230Stream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<Id3V2StreamBase>(*this);
    }
};


class Id3V240Stream;
class CommonData;


class Id3V230StreamWriter
{
    std::vector<const Id3V2Frame*> m_vpOwnFrames;
    std::vector<const Id3V2Frame*> m_vpAllFrames;
    bool m_bKeepOneValidImg;
    bool m_bFastSave;
    const std::string m_strDebugFileName; // used only to show debug info from asserts

public:
    enum { KEEP_ALL_IMG, KEEP_ONE_VALID_IMG };
    Id3V230StreamWriter(bool bKeepOneValidImg, bool bFastSave, Id3V2StreamBase*, const std::string& strDebugFileName); // if bKeepOneValidImg is true, at most an APIC frame is kept, and it has to be valid or at least link; Id3V2StreamBase* may be 0;
    ~Id3V230StreamWriter();


    void removeFrames(const std::string& strName, int nPictureType = -1); // if multiple frames with the same name exist, they are all removed; asserts that nPictureType is -1 for non-APIC frames; if nPictureType is -1 and strName is APIC, it removes all APIC frames
    //void removeApicFrames(const std::vector<char>& vcData, int nPictureType); // an APIC frame is removed iff it has the image in vcData or the type nPictureType (or both)
    void addTextFrame(const std::string& strName, const std::string& strVal); // strVal is UTF8; the frame will use ASCII if possible and UTF16 otherwise (so if there's a char with a code above 127, UTF16 gets used, to avoid codepage issues for codes between 128 and 255); nothing is added if strVal is empty; all zeroes are saved and not considered terminators;
    void addBinaryFrame(const std::string& strName, std::vector<char>& vcData); // destroys vcData by doing a swap for its own representation; asserts that strName is not APIC

    // the image type is ignored; images are always added as cover;
    // if there is an APIC frame with the same image, it is removed (it doesn't matter if it has different type, description ...);
    // if cover image already exists it is removed;
    void addImg(std::vector<char>& vcData);

    void addNonOwnedFrame(const Id3V2Frame* p);

    void setRecTime(const TagTimestamp& time);

    // this only changes the frames that correspond to the active settings in the configuration;
    // if WMP handling is disabled, TPE2 is left untouched; if WMP handling is enabled, TPE2 is either removed or set to "Various Artists", based on "b"
    // if iTunes handling is disabled, TCON is left untouched; if WMP handling is enabled, TCON is either removed or set to "1", based on "b"
    void setVariousArtists(bool b);

    // throws WriteError if it cannot write, including the case when nTotalSize is too small;
    // if nTotalSize is >0, the padding will be be whatever is left;
    // if nTotalSize is <0 and m_bFastSave is true, there will be a padding of around ImageInfo::MAX_IMAGE_SIZE+Id3V2Expander::EXTRA_SPACE;
    // if (nTotalSize is <0 and m_bFastSave is false) or if nTotalSize is 0 (regardless of m_bFastSave), there will be a padding of between DEFAULT_EXTRA_SPACE and DEFAULT_EXTRA_SPACE + 511;
    // (0 discards extra padding regardless of m_bFastSave)
    void write(std::ostream& out, int nTotalSize = -1) const;

    //bool equalTo(Id3V2StreamBase* pId3V2Stream) const; // returns true if all of these happen: pId3V2Stream is ID3V2.3.0, no unsynch is used, the frames are identical except for their order; padding is ignored;
    bool contentEqualTo(Id3V2StreamBase* pId3V2Stream) const; // returns true if the frames are identical except for their order; padding, unsynch and version is ignored;

    bool isEmpty() const { return m_vpAllFrames.empty(); }

    static const int DEFAULT_EXTRA_SPACE;
};


#endif // ifndef Id3V230StreamH

