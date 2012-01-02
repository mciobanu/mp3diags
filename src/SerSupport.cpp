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


#include  "CommonData.h"

#include  "SerSupport.h"


//#include  <boost/archive/binary_oarchive.hpp>
//#include  <boost/archive/binary_iarchive.hpp>
//#include  <boost/archive/xml_oarchive.hpp>

#include  <boost/serialization/vector.hpp>
//#include  "fstream_unicode.h"
#include  <boost/serialization/deque.hpp>

//#include  <boost/serialization/split_member.hpp>
//#include  <boost/serialization/split_free.hpp>


// headers needed only for serialization:
#include  "Id3V2Stream.h"
#include  "Id3V230Stream.h"
#include  "Id3V240Stream.h"
#include  "ApeStream.h"
#include  "MpegStream.h"
#include  "LyricsStream.h"



// serialization for std::streampos // ttt2 perhaps put this in SerSupport.h, but it doesn't matter much; it needs to be visible only when instantiating the templates
namespace boost {
namespace serialization {

//ttt2 see if a newer version of Boost handles std::streampos

template<class Archive>
inline void save(Archive& ar, const std::streampos& pos, const unsigned int /*nVersion*/)
{
    long long n (pos); // ttt2 this conversion isn't quite right, but it's OK for MP3 files
    ar << n;
}

template<class Archive>
inline void load(Archive& ar, std::streampos& pos, const unsigned int /*nVersion*/)
{
    long long n;
    ar >> n;
    pos = n;
}

} // namespace serialization
} // namespace boost

BOOST_SERIALIZATION_SPLIT_FREE(std::streampos);

using namespace std;
using namespace pearl;

#if 0
#if 1
// serialization for std::streampos
namespace boost {
namespace serialization {

//ttt2 see if a newer version of Boost handles std::streampos

template<class Archive>
inline void save(Archive& ar, const std::streampos& pos, const unsigned int /*nVersion*/)
{
    long long n (pos); // ttt2 this conversion isn't quite right, but it's OK for MP3 files
    ar << n;
}

template<class Archive>
inline void load(Archive& ar, std::streampos& pos, const unsigned int /*nVersion*/)
{
    long long n;
    ar >> n;
    pos = n;
}

} // namespace serialization
} // namespace boost

BOOST_SERIALIZATION_SPLIT_FREE(std::streampos);

//BOOST_SERIALIZATION_SPLIT_FREE(std::fpos<__mbstate_t>);

#endif

struct MyClass
{
    int a;
};

struct MyClass2
{
    int b;

    template<class Archive>
    void save(Archive& ar, const unsigned int ) const
    {
        // note, version is always the latest when saving
        ar  & b;
    }

    template<class Archive>
    void load(Archive& ar, const unsigned int )
    {
        ar  & b;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER();
};


namespace boost {
namespace serialization {

template<class Archive>
inline void save(Archive& ar, const MyClass& x, const unsigned int /*nVersion*/)
{
    long long n (x.a); // ttt2 this conversion isn't quite right, but it's OK for MP3 files
    ar << n;
}

template<class Archive>
inline void load(Archive& ar, MyClass& x, const unsigned int /*nVersion*/)
{
    long long n;
    ar >> n;
    x.a = n;
}

//BOOST_SERIALIZATION_SPLIT_FREE(std::streampos);


} // namespace serialization
} // namespace boost

BOOST_SERIALIZATION_SPLIT_FREE(MyClass);

namespace boost {
namespace serialization {

/*template<class Archive>
void serialize1(Archive& ar, MyClass& x, const unsigned int version)
{
    ar & x.a;
}*/

template<class Archive>
void serialize1(Archive& ar, MyClass& x, const unsigned int version)
{
    split_free(ar, x, version);
}

BOOST_SERIALIZATION_SPLIT_FREE(MyClass);

} // namespace serialization
}

#if 1

#include "fstream_unicode.h"

struct BoostTstBase01
{
    BoostTstBase01(int n1) : m_n1(n1) {}
    virtual ~BoostTstBase01()
    {
    }

    int m_n1;
    std::streampos m_pos;
    MyClass m;
    MyClass2 m2;

protected:
    BoostTstBase01() {}
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & m_n1;
        ar & m_pos;
        ar & m;
        ar & m2;
    }
};


void qwrqwrqsrq()
{
    ifstream_utf8 in ("qqq");
    boost::archive::text_iarchive iar (in);
    //boost::archive::text_iarchive iar (in);
    //iar >> m_vpAllHandlers;
    BoostTstBase01* p;
    iar >> p;
}
#endif

#endif


#define REGISTER_SER_TYPES0(ar)

#define REGISTER_SER_TYPES(ar) \
{ \
    ar.register_type<Id3V230Frame>(); \
    ar.register_type<Id3V240Frame>(); \
    ar.register_type<ApeItem>(); \
 \
    ar.register_type<ApeStream>(); \
    ar.register_type<Id3V1Stream>(); \
    ar.register_type<Id3V230Stream>(); \
    ar.register_type<Id3V240Stream>(); \
    ar.register_type<LyricsStream>(); \
    ar.register_type<MpegStream>(); \
    ar.register_type<VbriStream>(); \
    ar.register_type<XingStreamBase>(); \
    ar.register_type<LameStream>(); \
    ar.register_type<XingStream>(); \
    ar.register_type<NullDataStream>(); \
    ar.register_type<BrokenDataStream>(); \
    ar.register_type<TruncatedMpegDataStream>(); \
    ar.register_type<UnknownDataStream>(); \
    ar.register_type<UnsupportedDataStream>(); \
 \
    ar.register_type<Mp3Handler>(); \
}

/*



*/


/*
    ar.register_type<Id3V2Frame>(); \

    ar.register_type<DataStream>(); \
    ar.register_type<MpegStreamBase>(); \
    ar.register_type<Id3V2StreamBase>(); \
    ar.register_type<SimpleDataStream>(); \
    ar.register_type<UnknownDataStreamBase>(); \

*/



template<class Archive> void CommonData::save(Archive& ar, const unsigned int nVersion) const
{
    if (nVersion > 1) { throw std::runtime_error("invalid version of serialized file"); }

    int n1 (10);
    ar << n1;
    ar << getCrtName();
    int n2 (20);
    ar << n2;
    ar << (const deque<const Mp3Handler*>&)m_vpAllHandlers;
    ar << (const Filter&)m_filter;
    ar << m_eViewMode;
    //qDebug("save name %s", getCrtName().c_str());
}


template<class Archive> void CommonData::load(Archive& ar, const unsigned int nVersion)
{
    if (nVersion > 1) { throw std::runtime_error("invalid version of serialized file"); }

    int n1 (100);
    ar >> n1;
    //string strCrtName;
    ar >> m_strLoadCrtName;
    int n2 (100);
    ar >> n2;

    deque<Mp3Handler*> v;
    ar >> v;
    CB_ASSERT (m_vpAllHandlers.empty());
    for (int i = 0; i < cSize(v); ++i)
    {
        v[i]->sortNotes(); // !!! ususally not needed, but there's an exception: when loading an older version, referencing notes that have been removed; they get replaced by Notes::getMissingNote(), which comes first in sorting by CmpNotePtrById; probably not worth the trouble of figuring if it's really needed or not;
    }

    m_vpAllHandlers.insert(m_vpAllHandlers.end(), v.begin(), v.end());

    m_vpFltHandlers = m_vpAllHandlers;
    m_vpViewHandlers = m_vpAllHandlers;

    //qDebug("load name %s", strCrtName.c_str());
    ar >> m_filter;

    if (nVersion >= 1)
    {
        ar >> m_eViewMode;
    }

    //updateWidgets(m_strLoadCrtName);
}


BOOST_CLASS_VERSION(CommonData, 1);

// returns an error message (or empty string if there's no error)
string CommonData::save(const string& strFile) const
{
    try
    {
        ofstream_utf8 out (strFile.c_str());
        if (!out) { return "Cannot open file \"" + strFile + "\""; }

        //boost::archive::binary_oarchive oar (out);
        boost::archive::text_oarchive oar (out);
        //boost::archive::xml_oarchive oar (out); // ttt3 would be nice to have this as an option, but doesn't seem possible unless we switch everything to XML, because while binary_oarchive can be used as a replacement for text_oarchive, xml_oarchive can't; the serialized classes need to be modified: http://www.boost.org/doc/libs/1_40_0/libs/serialization/doc/wrappers.html#nvp
        REGISTER_SER_TYPES(oar);
        oar << *this;
    }
    catch (const std::exception& ex) //ttt2 not sure if this is the way to catch errors
    {
        return ex.what();
    }

    return "";
}


//#include <iostream>


// returns an error message (or empty string if there's no error)
string CommonData::load(const string& strFile)
{
    try
    {
        ifstream_utf8 in (strFile.c_str());
        if (!in) { return ""; } // !!! "no data" is not an error

        //boost::archive::binary_iarchive iar (in);
        boost::archive::text_iarchive iar (in);
        REGISTER_SER_TYPES(iar);
        iar >> *this;
    }
    //catch (const boost::archive::archive_exception&)
    catch (const std::exception& ex)
    {
        return ex.what();
    }
    catch (...)
    {
        return "err";
    }

    Notes::clearDestroyList();
    return "";
}

