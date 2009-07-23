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


#ifndef Id3V2StreamH
#define Id3V2StreamH

#include  <iosfwd>

#include  "DataStream.h"

#include  "SerSupport.h"



// Frame of an ID3V2 tag
struct Id3V2Frame
{
    char m_szName[5];
    int m_nMemDataSize;  // size in memory, excluding header and unsynch
    int m_nDiskDataSize; // size including unsynchronization, excluding the header; if unsynch is used, m_nDiskDataSize>=m_nMemDataSize; if unsynch is not used, m_nDiskDataSize==m_nMemDataSize
    int m_nDiskHdrSize;  // normally it's ID3_FRAME_HDR_SIZE but may be larger due to the unsynch scheme
    unsigned char m_cFlag1;
    unsigned char m_cFlag2;
    enum { ID3_FRAME_HDR_SIZE = 10 };
    std::streampos m_pos; // position of the frame on the disk, including the header
    StringWrp* m_pFileName; // needed for serialization (it seems that the serialization cannot save pointers to strings, which makes some sense given that std::string is a "fundamental" type for the Boost serialization) // ttt1 make sure it's true
    bool m_bHasUnsynch;
    mutable bool m_bHasLatin1NonAscii; //ttt2 "mutable" doesn't look right

    virtual ~Id3V2Frame();

    enum { SHORT_INFO, FULL_INFO };
    void print(std::ostream& out, bool bFullInfo) const; // if bFullInfo is true some frames print more extensive details, e.g. lyrics
    virtual std::string getUtf8String() const = 0;
    std::string getReadableName() const; // normally returns m_szName, but if it has invalid characters (<=32 or >=127), returns a hex representation

    void writeUnsynch(std::ostream& out) const; // copies the frame to out, removing the unsynch bytes if they are present

    std::vector<char> m_vcData; // this is emptied when the constructor exits if it needs to much space; to reliably access a frame's data, Id3V2FrameDataLoader should be used

    virtual bool discardOnChange() const = 0; // ttt2 should distinguish between audio and ID3V2 change, should be called when audio is changed, ...

    struct NotId3V2Frame {};
    struct UnsupportedId3V2Frame {};

    enum ApicStatus { NO_APIC, ERROR, USES_LINK, NOT_SUPPORTED, OK };
    ApicStatus m_eApicStatus;
    enum PictureType { OTHER = 0, ICON = 1, COVER = 3 };
    int m_nPictureType; // for APIC only (cover, back, ...)
    int m_nImgOffset;   // for APIC only
    int m_nImgSize;     // for APIC only
    ImageInfo::Compr m_eCompr;   // for APIC only
    short m_nWidth;     // for APIC only
    short m_nHeight;    // for APIC only

private:
    Id3V2Frame(const Id3V2Frame&);
    Id3V2Frame& operator=(const Id3V2Frame&);

protected:
    enum { NO_UNSYNCH, HAS_UNSYNCH };
    Id3V2Frame(std::streampos pos, bool bHasUnsynch, StringWrp* pFileName);
    Id3V2Frame() : m_nWidth(-1), m_nHeight(-1) {} // serialization-only constructor

    static std::string utf8FromBomUtf16(const char* pData, int nSize); // returns a UTF8 string from a buffer containing a UTF16 sting starting with BOM

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        ar & m_szName;
        ar & m_nMemDataSize;
        ar & m_nDiskDataSize;
        ar & m_nDiskHdrSize;
        ar & m_cFlag1;
        ar & m_cFlag2;

        ar & m_pos;
        ar & m_pFileName;
        ar & m_bHasUnsynch;
        ar & m_bHasLatin1NonAscii;

        ar & m_vcData;

        ar & m_eApicStatus;
        ar & m_nPictureType;
        ar & m_nImgOffset;
        ar & m_nImgSize;
        ar & m_eCompr;

        if (nVersion > 0)
        {
            ar & m_nWidth;
            ar & m_nHeight;
        }
    }
};

BOOST_CLASS_VERSION(Id3V2Frame, 1);


// Makes that frame data available, loading it from the file when necessary. This is needed for pictures or other large frames that would take too much space if they were stored in the memory, so they get discarded after the constructor completes.
class Id3V2FrameDataLoader
{
    Id3V2FrameDataLoader(const Id3V2FrameDataLoader&);
    Id3V2FrameDataLoader& operator=(const Id3V2FrameDataLoader&);
    const Id3V2Frame& m_frame;
    const char* m_pData;
    bool m_bOwnsData;
public:
    Id3V2FrameDataLoader(const Id3V2Frame& frame);

    ~Id3V2FrameDataLoader();

    const char* getData() const { return m_pData; }

    struct LoadFailure {}; // thrown if the file is deleted / moved (and perhaps changed) after frames are identified
};



// reads nCount bytes into pDest;
// if bHasUnsynch is true, it actually reads more bytes, applying the unsynchronization algorithm, so pDest gets nCount bytes;
// returns the number of bytes it could read;
// posNext is the position where the next block begins (might be EOF); nothing starting at that position should be read; needed to take care of an ID3V2 tag whose last frame ends with 0xff and has no padding;
// asserts that posNext is not before the current position in the stream
std::streamsize readID3V2(bool bHasUnsynch, std::istream& in, char* pDest, std::streamsize nCount, std::streampos posNext, int& nBytesSkipped);

// the total size, including the 10-byte header
int getId3V2Size(char* pId3Header);



//============================================================================================================
//============================================================================================================
//============================================================================================================


class Id3V2StreamBase : public DataStream, public TagReader
{
protected:
    std::vector<Id3V2Frame*> m_vpFrames;
    int m_nTotalSize; // including the header
    int m_nPaddingSize;
    unsigned char m_cFlags;
    std::streampos m_pos;

    StringWrp* m_pFileName;

    Id3V2Frame* findFrame(const char* szFrameName); //ttt2 finds the first frame, but doesn't care about duplicates
    const Id3V2Frame* findFrame(const char* szFrameName) const; //ttt2 finds the first frame, but doesn't care about duplicates

    void checkDuplicates(NoteColl& notes) const;
    void checkFrames(NoteColl& notes); // various checks to be called from derived class' constructor

    ImageInfo::Status m_eImageStatus;
    Id3V2Frame* m_pPicFrame;
    static const char* decodeApic(NoteColl& notes, std::streampos pos, const char* pData, const char*& szMimeType, int& nPictureType, const char*& szDescription);
    void preparePictureHlp(NoteColl& notes, Id3V2Frame* pFrame, const char* pFrameData, const char* pImgData, const char* szMimeType);
    void preparePicture(NoteColl& notes); // initializes fields used by the APIC frame

    TagTimestamp get230TrackTime(bool* pbFrameExists) const;

    Id3V2StreamBase(int nIndex, std::istream& in, StringWrp* pFileName);
    Id3V2StreamBase() {} // serialization-only constructor

    struct NotSupTextEnc {};
public:
    /*override*/ ~Id3V2StreamBase();

    void printFrames(std::ostream& out) const;

    /*override*/ void copy(std::istream& in, std::ostream& out);
    /*override*/ std::string getInfo() const;

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return m_nTotalSize; }

    static const char* getClassDisplayName();

    const std::vector<Id3V2Frame*>& getFrames() const { return m_vpFrames; }
    bool hasUnsynch() const;

    const Id3V2Frame* getFrame(const char* szName) const; // returns a frame with the given name; normally it returns the first such frame, but it may return another if there's a good reason (currently this is done for APIC only); returns 0 if no frame was found;

    enum { ID3_HDR_SIZE = 10 };

    std::vector<const Id3V2Frame*> getKnownFrames() const; // to be used by Id3V2Cleaner;

    int getPaddingSize() const { return m_nPaddingSize; }

    struct NotId3V2 {};

    // ================================ TagReader =========================================
    /*override*/ std::string getTitle(bool* pbFrameExists = 0) const;

    /*override*/ std::string getArtist(bool* pbFrameExists = 0) const;

    /*override*/ std::string getTrackNumber(bool* pbFrameExists = 0) const;

    /*override*/ std::string getGenre(bool* pbFrameExists = 0) const;

    /*override*/ ImageInfo getImage(bool* pbFrameExists = 0) const;

    /*override*/ std::string getAlbumName(bool* pbFrameExists = 0) const;

    /*override*/ double getRating(bool* pbFrameExists = 0) const;

    /*override*/ std::string getComposer(bool* pbFrameExists = 0) const;

    /*override*/ std::string getOtherInfo() const;

    enum { DONT_ACCEPT_BROKEN, ACCEPT_BROKEN };

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<DataStream>(*this);

        ar & m_vpFrames;
        ar & m_nTotalSize;
        ar & m_nPaddingSize;
        ar & m_cFlags;
        ar & m_pos;

        ar & m_pFileName;
        //StringWrp wrp ("111222");
        //ar & wrp;

        ar & m_eImageStatus;
        ar & m_pPicFrame;
    }
};



//============================================================================================================
//============================================================================================================
//============================================================================================================


struct KnownFrames
{
    static const char* LBL_TITLE();
    static const char* LBL_ARTIST();
    static const char* LBL_TRACK_NUMBER();
    static const char* LBL_TIME_YEAR_230();
    static const char* LBL_TIME_DATE_230();
    static const char* LBL_TIME_240();
    static const char* LBL_GENRE();
    static const char* LBL_IMAGE();
    static const char* LBL_ALBUM();
    static const char* LBL_RATING();
    static const char* LBL_COMPOSER();

    struct InvalidIndex {};

    static const char* getFrameName (int n); // throws InvalidIndex if n is out of bounds
    static const std::set<std::string>& getKnownFrames();

    static bool canHaveDuplicates(const char* szName); // to be counted as duplicates, 2 frames must have the same name and picture type, so the value returned here is only part of the test
};


#endif // ifndef Id3V2StreamH

