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


#ifndef Id3V240StreamH
#define Id3V240StreamH

#include  "Id3V2Stream.h"


// Frame of an ID3V2.4.0 tag
struct Id3V240Frame : public Id3V2Frame
{
    Id3V240Frame(NoteColl& notes, std::istream& in, std::streampos pos, bool bHasUnsynch, std::streampos posNext, StringWrp* pFileName);
private:
    bool checkSize(std::istream& in, std::streampos posNext); // since broken applications may use all 8 bits for size, although only 7 should be used, this tries to figure out if the size is correct
    /*override*/ bool discardOnChange() const;
    /*override*/ std::string getUtf8StringImpl() const;

private:
    friend class boost::serialization::access;
    Id3V240Frame() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<Id3V2Frame>(*this);
    }
};



class Id3V240Stream : public Id3V2StreamBase
{
public:
    Id3V240Stream(int nIndex, NoteColl& notes, std::istream& in, StringWrp* pFileName, bool bAcceptBroken = false);
    //typedef typename Id3V2Stream<Id3V230Frame>::NotId3V2 NotId3V240;

    DECL_RD_NAME("ID3V2.4.0");

    /*override*/ TagTimestamp getTime(bool* pbFrameExists = 0) const;
    /*override*/ void setTrackTime(const TagTimestamp&) { throw NotSupportedOp(); }

    /*override*/ SuportLevel getSupport(Feature) const;

private:
    friend class boost::serialization::access;
    Id3V240Stream() {} // serialization-only constructor

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & boost::serialization::base_object<Id3V2StreamBase>(*this);
    }
};



#endif // ifndef Id3V240StreamH

