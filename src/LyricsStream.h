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

#include  "DataStream.h"


class LyricsStream : public DataStream
{
    std::streampos m_pos;
    std::streamoff m_nSize;
public:
    LyricsStream(int nIndex, NoteColl& notes, std::istream& in);

    /*override*/ void copy(std::istream& in, std::ostream& out);
    DECL_NAME("Lyrics3 V2.00");
    /*override*/ std::string getInfo() const;

    /*override*/ std::streampos getPos() const { return m_pos; }
    /*override*/ std::streamoff getSize() const { return m_nSize; }

    struct NotLyricsStream {};

private:
    friend class boost::serialization::access;
    LyricsStream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<DataStream>(*this);

        ar & m_pos;
        ar & m_nSize;
    }
};




#endif // ifndef LyricsStreamH


