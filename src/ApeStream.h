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


#ifndef ApeStreamH
#define ApeStreamH

#include  <vector>
#include  <istream>

#include  <QApplication> // for translation

#include  "DataStream.h"

//struct ApeItem;

struct ApeItem // !!! needs to be public for serialization
{
    Q_DECLARE_TR_FUNCTIONS(ApeItem)

public:
    ApeItem(NoteColl& notes, std::istream& in);
    ~ApeItem() {}

    unsigned char m_cFlags1;
    unsigned char m_cFlags2;
    unsigned char m_cFlags3;
    unsigned char m_cFlags4;

    std::string m_strName;
    std::vector<char> m_vcValue; // raw value; may be binary or UTF8
    enum Type { UTF8, BINARY }; // ttt2 add UTF8LIST, DATE, ...

    Type m_eType;

    std::string getUtf8String() const;

    int getTotalSize() const { return 4 + 4 + int(m_strName.size()) + 1 + int(m_vcValue.size()); }

private:
    friend class boost::serialization::access;
    ApeItem() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & m_cFlags1;
        ar & m_cFlags2;
        ar & m_cFlags3;
        ar & m_cFlags4;

        ar & m_strName;
        ar & m_vcValue;
        ar & m_eType;
    }
};



class ApeStream : public DataStream, public TagReader
{
    Q_DECLARE_TR_FUNCTIONS(ApeStream)

    int m_nVersion;
    std::streampos m_pos;
    std::streamoff m_nSize;

    std::vector<ApeItem*> m_vpItems;
    void readKeys(NoteColl& notes, std::istream& in);

    ApeItem* findItem(const char* szFrameName);
    const ApeItem* findItem(const char* szFrameName) const;
public:
    ApeStream(int nIndex, NoteColl& notes, std::istream& in);
    ~ApeStream();
    /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_RD_NAME("Ape")
    /*override*/ std::string getInfo() const;

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return m_nSize; }

    //enum Mp3Gain { NONE = 0x00, TRACK = 0x01, ALBUM = 0x02, BOTH = 0x03 };
    //Mp3Gain getMp3GainStatus() const;
    bool hasMp3Gain() const;

    struct NotApeStream {};
    struct NotApeHeader {};
    struct NotApeFooter {};
    struct HeaderFooterMismatch {};

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
    ApeStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<DataStream>(*this);
        ar & m_nVersion;
        ar & m_pos;
        ar & m_nSize;

        ar & m_vpItems;
    }
};




#endif // ifndef ApeStreamH

