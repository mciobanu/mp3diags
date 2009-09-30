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


#ifndef DataStreamH
#define DataStreamH

#include  <vector>

#include  "SerSupport.h"

#include  "Notes.h"
#include  "CommonTypes.h"

#define MP3_CHECK(COND, POS, MSG_ID, EXCP) { if (!(COND)) { notes.add(new Note(Notes::MSG_ID(), POS)); throw EXCP; } }
#define MP3_CHECK_T(COND, POS, MSG, EXCP) { if (!(COND)) { static Note::SharedData d (MSG, false); notes.add(new Note(d, POS)); throw EXCP; } }
#define MP3_THROW(POS, MSG_ID, EXCP) { notes.add(new Note(Notes::MSG_ID(), POS)); throw EXCP; }
#define MP3_THROW_T(POS, MSG, EXCP) { static Note::SharedData d (MSG, false); notes.add(new Note(d, POS)); throw EXCP; }
#define MP3_NOTE(POS, MSG_ID) { notes.add(new Note(Notes::MSG_ID(), POS)); }
#define MP3_NOTE_D(POS, MSG_ID, DETAIL) { notes.add(new Note(Notes::MSG_ID(), POS, DETAIL)); }
#define MP3_TRACE(POS, MSG) { static Note::SharedData d (MSG, false); notes.add(new Note(d, POS)); }

/*

DataStream constructors may leave the input file in EOF or other invalid state and the read pointer with an arbitrary value, regardless of them succeeding or not. Therefore they the file state should be cleared by the user.

*/



//============================================================================================================
//============================================================================================================
//============================================================================================================


#define DECL_NAME(s) \
    static const char* getClassDisplayName() { return s; } \
    /*override*/ const char* getDisplayName() const { return getClassDisplayName(); }


// this is to be used with TagReader descendants:
#define DECL_RD_NAME(s) \
    static const char* getClassDisplayName() { return s; } \
    /*override*/ const char* getDisplayName() const { return getClassDisplayName(); } \
    /*override*/ const char* getName() const { return getClassDisplayName(); }



#define STRM_ASSERT(COND) { if (!(COND)) { assertBreakpoint(); ::trace("assert"); logAssert(__FILE__, __LINE__, #COND, getGlobalMp3HandlerName()); ::exit(1); } }

std::string getGlobalMp3HandlerName(); // a hack to get the name of the current file from inside various streams without storing the name there //ttt1 review


class DataStream
{
    int m_nIndex;
    DataStream(const DataStream&);
    DataStream& operator=(const DataStream&);
protected:
    DataStream(int nIndex) : m_nIndex(nIndex) {}
    DataStream() {} // serialization-only constructor
public:
    virtual ~DataStream() {}
    // throws WriteError if there are errors writing to the file; derived classes have several options on implementing this, from simply copying content from the input to ignoring the input file completely; e.g. ID3V2 can change the size of the padding, drop some frames, replace others, ...
    virtual void copy(std::istream& in, std::ostream& out) = 0;
    virtual const char* getDisplayName() const = 0;
    virtual std::string getInfo() const = 0;

    virtual std::streampos getPos() const = 0;
    virtual std::streamoff getSize() const = 0;
    std::streampos getEnd() const { return getPos() + getSize(); }

    int getIndex() const { return m_nIndex; }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & m_nIndex;
    }
};


// simply copies content from the input file; stores position and size to be able to do so
class SimpleDataStream : public DataStream
{
protected:
    std::streampos m_pos;
    std::streamoff m_nSize;

    SimpleDataStream(int nIndex, std::streampos pos, std::streamoff nSize) : DataStream(nIndex), m_pos(pos), m_nSize(nSize) {}
    SimpleDataStream() {} // serialization-only constructor
public:
    // seeks "in" for its beginning and appends m_nSize bytes to "out";
    /*override*/ void copy(std::istream& in, std::ostream& out);

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return m_nSize; }

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<DataStream>(*this);

        ar & m_pos;
        ar & m_nSize;
    }
};


class UnknownDataStreamBase : public SimpleDataStream // ttt2 perhaps unify with SimpleDataStream, as it's its only descendant; OTOH SimpleDataStream might get other descendants in the future, so maybe not // ttt2 not the best name, but others don't seem better
{
public:
    enum { BEGIN_SIZE = 32 };
protected:
    char m_begin [BEGIN_SIZE];
    void append(const UnknownDataStreamBase&);
    UnknownDataStreamBase() {} // serialization-only constructor

public:
    UnknownDataStreamBase(int nIndex, NoteColl& notes, std::istream& in, std::streamoff nSize);
    /*override*/ std::string getInfo() const;

    const char* getBegin() const { return m_begin; }

    struct BadUnknownStream {}; // e.g. there are not nSize chars left

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<SimpleDataStream>(*this);

        ar & m_begin;
    }
};


class UnknownDataStream : public UnknownDataStreamBase
{
public:
    UnknownDataStream(int nIndex, NoteColl& notes, std::istream& in, std::streamoff nSize) : UnknownDataStreamBase(nIndex, notes, in, nSize) {}
    DECL_NAME("Unknown");
    using UnknownDataStreamBase::append;

private:
    friend class boost::serialization::access;
    UnknownDataStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<UnknownDataStreamBase>(*this);
    }
};


class BrokenDataStream : public UnknownDataStreamBase
{
    std::string m_strName;
    std::string m_strBaseName;
    std::string m_strInfo;
public:
    BrokenDataStream(int nIndex, NoteColl& notes, std::istream& in, std::streamoff nSize, const char* szBaseName, const std::string& strInfo) :
            UnknownDataStreamBase(nIndex, notes, in, nSize),
            m_strName(std::string("Broken ") + szBaseName),
            m_strBaseName(szBaseName),
            m_strInfo(strInfo.empty() ? UnknownDataStreamBase::getInfo() : strInfo + "; " + UnknownDataStreamBase::getInfo()) {}

    /*override*/ const char* getDisplayName() const { return m_strName.c_str(); } // DECL_NAME doesn't work in this case
    /*override*/ std::string getInfo() const { return m_strInfo; }
    const std::string& getBaseName() const { return m_strBaseName; }

private:
    friend class boost::serialization::access;
    BrokenDataStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<UnknownDataStreamBase>(*this);

        ar & m_strName;
        ar & m_strBaseName;
        ar & m_strInfo;
    }
};


class UnsupportedDataStream : public UnknownDataStreamBase // ttt3 perhaps merge code with BrokenDataStream in some base class
{
    std::string m_strName;
    std::string m_strBaseName;
    std::string m_strInfo;
public:
    UnsupportedDataStream(int nIndex, NoteColl& notes, std::istream& in, std::streamoff nSize, const char* szBaseName, const std::string& strInfo) :
            UnknownDataStreamBase(nIndex, notes, in, nSize),
            m_strName(std::string("Unsupported ") + szBaseName),
            m_strBaseName(szBaseName),
            m_strInfo(strInfo.empty() ? UnknownDataStreamBase::getInfo() : strInfo + "; " + UnknownDataStreamBase::getInfo()) {}

    /*override*/ const char* getDisplayName() const { return m_strName.c_str(); } // DECL_NAME doesn't work in this case
    /*override*/ std::string getInfo() const { return m_strInfo; }
    const std::string& getBaseName() const { return m_strBaseName; }

private:
    friend class boost::serialization::access;
    UnsupportedDataStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<UnknownDataStreamBase>(*this);

        ar & m_strName;
        ar & m_strBaseName;
        ar & m_strInfo;
    }
};


class MpegStream;
struct MpegFrameBase;


// follows an MPEG stream and is compatible with it but it is truncated; some other frame begins before where it should end
class TruncatedMpegDataStream : public UnknownDataStreamBase
{
    MpegFrameBase* m_pFrame;
    TruncatedMpegDataStream(const TruncatedMpegDataStream&);
    TruncatedMpegDataStream& operator=(const TruncatedMpegDataStream&);
public:
    TruncatedMpegDataStream(MpegStream* pPrevMpegStream, int nIndex, NoteColl& notes, std::istream& in, std::streamoff nSize);
    ~TruncatedMpegDataStream();

    DECL_NAME("Truncated MPEG");
    /*override*/ std::string getInfo() const;
    int getExpectedSize() const;

    struct NotTruncatedMpegDataStream {}; // thrown if pMpegStream is 0 or it points to an incompatible stream

    bool hasSpace(std::streamoff nSize) const;
    using UnknownDataStreamBase::append;

private:
    friend class boost::serialization::access;
    TruncatedMpegDataStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<UnknownDataStreamBase>(*this);

        ar & m_pFrame;
    }
};



// simply copies content from the input file; stores pos and sizes to be able to do so
class NullDataStream : public DataStream
{
    std::streampos m_pos;
    std::streamoff m_nSize;
public:
    NullDataStream(int nIndex, NoteColl& notes, std::istream& in);

    /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_NAME("Null");
    /*override*/ std::string getInfo() const;

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return m_nSize; }

    struct NotNullStream {};

private:
    friend class boost::serialization::access;
    NullDataStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<DataStream>(*this);

        ar & m_pos;
        ar & m_nSize;
    }
};


//==========================================================================
//==========================================================================


struct NotSupportedOp {}; // operation is not currently supported


/* While building a stream, by reading from a file, several situations can be encountered:
    - it is read OK and there are no issues
    - it is read OK but some parts are incorrect; warnings or errors will be displayed, but it is possible to work with the stream and correct the errors, or perhaps leave it as it is;
    - it begins as expected, but a serious error is then encountered, so the object cannot be built;
    - the beginning is incorrect, meaning that it's just a different kind of stream, end of story; nothing should happen in this case;

StreamIsBroken is for the 3rd case. It should be thrown when it is pretty likely that a stream of the appropriate type begins at the current position in the file but the stream can't be read.
Given that the reason a stream can't be read may actually be that the code is buggy or incomplete, it is possible for a StreamIsBroken to be thrown when an StreamIsUnsupported should have been thrown instead.

*/
struct StreamIsBroken
{
    // doesn't own the pointer; param is supposed to be string literal
    StreamIsBroken(const char* szStreamName, const std::string& strInfo) : m_szStreamName(szStreamName), m_strInfo(strInfo) {}
    const char* getStreamName() const { return m_szStreamName; }
    const std::string& getInfo() const { return m_strInfo; }
private:
    const char* m_szStreamName;
    const std::string m_strInfo; // may be empty, but it should have some details about what made it broken, if possible; or perhaps what could be decyphred from the stream before being being decided that it was broken
};


// to be thrown from the constructor of a stream when it looks like an unsupported version of the stream was found in the input file.
struct StreamIsUnsupported
{
    // doesn't own the pointers; params are supposed to be string literals
    StreamIsUnsupported(const char* szStreamName, const std::string& strInfo) : m_szStreamName(szStreamName), m_strInfo(strInfo) {}
    const char* getStreamName() const { return m_szStreamName; }
    const std::string& getInfo() const { return m_strInfo; }
private:
    const char* m_szStreamName;
    const std::string m_strInfo; // may be empty, but it should have some details about what made it unsupported, if possible
};

// Normally streams that throw StreamIsUnsupported or StreamIsBroken end up as UnsupportedDataStream or BrokenDataStream


//==========================================================================
//==========================================================================

//#include  <QPixmap> // ttt2 would be nicer not to have to depend on Qt in this file, but some image library is needed anyway, so it might as well be Qt


//==========================================================================
//==========================================================================



/*
Times in ID3V2.4.0:
    TDRC - when the audio was recorded
    TDOR - when the original recording of the audio was released
    TDRL - when the audio was first released
    TDEN - when the audio was encoded
    TDTG - when the audio was tagged

Format: subset of ISO 8601: yyyy-MM-ddTHH:mm:ss or shorter variants (truncated at the end)

TDRC is the one used for TIME; corresponds to TYER and TDAT in ID3V2.3.0

*/


//==========================================================================

// uses strings in the format "yyyy-MM-ddTHH:mm:ss" or "yyyy-MM-dd HH:mm:ss" or any prefix truncated right after a group of digits; the empty string is valid, as is the null char* passed on constructor;
// during initialization 'T' is changed to ' ' and then asString() uses this version, with ' '
class TagTimestamp
{
    char m_szVal [20];
    char m_szYear [5]; // either a 4-digit string or empty
    char m_szDayMonth [5]; // either a 4-digit string or empty; if m_strYear is empty, m_strDayMonth is empty too

public:
    explicit TagTimestamp(const std::string&); // may throw InvalidTime; see init();
    explicit TagTimestamp(const char* = 0); // may throw InvalidTime; see init();

    const char* getYear() const { return m_szYear; }
    const char* getDayMonth() const { return m_szDayMonth; }

    const char* asString() const { return m_szVal; }

    void init(std::string strVal); // if strVal is invalid it clears m_strVal, m_strYear and m_strDayMonth and throws InvalidTime; the constructors call this too and let the exceptions propagate

    struct InvalidTime {};
};


struct TagReader
{
    virtual ~TagReader();

    enum Feature { TITLE, ARTIST, TRACK_NUMBER, TIME, GENRE, IMAGE, ALBUM, RATING, COMPOSER, VARIOUS_ARTISTS, LIST_END };
    static const char* getLabel(int); // text representation for each Feature

    enum SuportLevel { NOT_SUPPORTED, READ_ONLY/*, READ_WRITE*/ };

    struct FieldNotFound {}; // for variable field lists, to allow for the distinction between an empty field and a missing field

    virtual std::string getTitle(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // UTF8

    virtual std::string getArtist(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // UTF8

    virtual std::string getTrackNumber(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // this is string (and not int) because in ID3V2 it's not necessarily a (single) number; it can be 2/12

    virtual TagTimestamp getTime(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); }

    virtual std::string getGenre(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // UTF8

    virtual ImageInfo getImage(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // !!! this must not be called from a non-GUI thread (see comment in Id3V2Stream<Frame>::preparePictureHlp); if there's a need to do so, perhaps switch from QPixmap to QImage

    virtual std::string getAlbumName(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // UTF8

    virtual double getRating(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // 0 to 5 (5 for best); negative for unknown

    virtual std::string getComposer(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // UTF8

    enum VariousArtists { VA_NONE = 0, VA_ITUNES = 1, VA_WMP = 2 };

    virtual int getVariousArtists(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); } // combination of VariousArtists flags; since several frames might be involved, *pbFrameExists is set to "true" if at least a frame exists

    virtual std::string getOtherInfo() const { return ""; } // non-editable tags

    virtual std::vector<ImageInfo> getImages() const { return std::vector<ImageInfo>(); } // all images, even those with errors

    virtual std::string getImageData(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = false; } return ""; }

    virtual SuportLevel getSupport(Feature) const { return NOT_SUPPORTED; }

    virtual const char* getName() const = 0;

    std::string getValue(Feature) const; // returns the corresponding feature converted to a string; if it's not supported, it returns an empty string; for IMAGE an empty string regardless of a picture being present or not

    static std::string getVarArtistsValue(); // what getValue(VARIOUS_ARTISTS) returns for VA tags, based on global configuration

    static int FEATURE_ON_POS[]; // the order in which Features should appear in grids
    static int POS_OF_FEATURE[]; // the "inverse" of FEATURE_ON_POS: what Feature appears in a given position
};



//==========================================================================
//==========================================================================

/*

Position: std::streampos; actually should be using std::istream::pos_type, but streampos is shorter; if the traits template param for streams are not used, std::istream::pos_type is just a typedef for std::streampos; represents a position in a file as a class (not as some integral); has automatic convertion to std::streamoff, so arithmetic operations work, but not necessarily as expected;

Relative position: std::streamoff; actually should be using std::istream::off_type, but streamoff is shorter; if the traits template param for streams are not used, std::istream::off_type is just a typedef for std::streamoff; signed integral, not necessarily big enough to represent any position in a file; the difference between 2 std::streampos objects is a streamoff;

std::streamsize - count for I/O operations, e.g. bytes read; signed integral; may be smaller than streamoff


// ttt2 perhaps replace "long long" with std::streamoff in file routines too, although "long long" seems well suited long-term and easier to deal with instead of the 3 types in std lib (it seems that the only reason the std lib doesn't just use "long long" is to avoid forcing 16bit and 32bit machines perform 64bit calculations); or, well, if we want to prepare for files with sizes exceeding 64bit, the type to use uniformly for positions, offsets and counts could be something implementation-specific that may be an integral type or a class that looks like a number (has +, -, *, /); on most systems it would just be defined to be "long long"; // 2008.09.08: actually streamoff is a typedef for fpos, which also cares about multi-byte characters, so a simple integral type may not be enough; still, it would be nice to be able to use streamoff everywhere for binary files and get rid of all the conversions and overflows;



More:
 - Standard draft 27.4.4
 - http://www.velocityreviews.com/forums/t287670-postype-how-to-define-it-.html

*/


#endif // #ifndef DataStreamH
