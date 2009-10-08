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


#ifndef LyricsStreamH
#define LyricsStreamH

#include  <iosfwd>
#include  <stdexcept>

#include  "DataStream.h"


class LyricsStream : public DataStream, public TagReader
{
    std::streampos m_pos;
    std::streamoff m_nSize;
public:
    LyricsStream(int nIndex, NoteColl& notes, std::istream& in, const std::string& strCrtDir);

    /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_RD_NAME("Lyrics3 V2.00");
    /*override*/ std::string getInfo() const;

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return m_nSize; }

    struct NotLyricsStream {};

    std::string m_strTitle;
    std::string m_strArtist;
    std::string m_strGenre;
    std::string m_strImageFile;
    std::string m_strAlbum;
    std::string m_strAuthor; // composer
    std::string m_strLyrics;
    std::string m_strOther; // these are lost during transfer //ttt2 or maybe not, though not sure if transferring them to some "comment" field would be a good idea
    std::string m_strInd;

    std::string m_strCrtDir;

    bool m_bHasTitle;
    bool m_bHasArtist;
    bool m_bHasGenre;
    bool m_bHasImage;
    bool m_bHasAlbum;
private:
    friend class boost::serialization::access;
    LyricsStream(); // serialization-only constructor


    // ================================ TagReader =========================================
    /*override*/ std::string getTitle(bool* pbFrameExists = 0) const;

    /*override*/ std::string getArtist(bool* pbFrameExists = 0) const;

    /*override*/ std::string getTrackNumber(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); }

    /*override*/ TagTimestamp getTime(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); }

    /*override*/ std::string getGenre(bool* pbFrameExists = 0) const;

    /*override*/ ImageInfo getImage(bool* pbFrameExists = 0) const;

    /*override*/ std::string getAlbumName(bool* pbFrameExists = 0) const;

    /*override*/ std::string getOtherInfo() const;

    /*override*/ SuportLevel getSupport(Feature) const;


    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 1) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<DataStream>(*this);

        ar & m_pos;
        ar & m_nSize;

        if (nVersion > 0)
        {
            ar & m_strTitle;
            ar & m_strArtist;
            ar & m_strGenre;
            ar & m_strImageFile;
            ar & m_strAlbum;
            ar & m_strAuthor;
            ar & m_strLyrics;
            ar & m_strOther;
            ar & m_strInd;

            ar & m_strCrtDir;

            ar & m_bHasTitle;
            ar & m_bHasArtist;
            ar & m_bHasGenre;
            ar & m_bHasImage;
            ar & m_bHasAlbum;
        }
    }
};

BOOST_CLASS_VERSION(LyricsStream, 1);


#endif // ifndef LyricsStreamH


