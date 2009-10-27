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


#ifdef MSVC_QMAKE
    #pragma warning (disable : 4100)
#endif


#include  <algorithm>

#include  <QFile>
#include  <QClipboard>
#include  <QMimeData>

#include  "TagWriter.h"

#include  "Helpers.h"
#include  "OsFile.h"
#include  "ImageInfoPanelWdgImpl.h"
#include  "SongInfoParser.h"
#include  "Widgets.h"
#include  "DiscogsDownloader.h" // just for SOURCE_NAME
#include  "MusicBrainzDownloader.h" // just for SOURCE_NAME
#include  "Id3V230Stream.h"
#include  "Id3V240Stream.h"
#include  "Mp3Manip.h"
#include  "CommonData.h"
#include  "MpegStream.h"

////#include  <iostream> //ttt remove

using namespace std;
using namespace pearl;


//======================================================================================================================
//======================================================================================================================
//======================================================================================================================



namespace
{
    ostream& operator<<(ostream& out, const TagReaderInfo& inf)
    {
        out << "<" << inf.m_strName << ", " << inf.m_nPos << ", " << (inf.m_bAlone ? "true" : "false") << ">";
        return out;
    }

    void printVec(ostream& out, const vector<TagReaderInfo>& v)
    {
        for (int i = 0, n = cSize(v); i < n; ++i)
        {
            out << v[i] << "  ";
        }
        out << endl;
    }
}


ostream& operator<<(ostream& out, const TagWriter::OrigValue& val)
{
    out << "song: " << val.m_nSong << ", field: " << val.m_nField << ", status: " << (int)val.m_eStatus << ", value: " << val.m_strVal;
    return out;
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

ImageColl::ImageColl() : m_nCurrent(-1)
{
}

int ImageColl::addImage(const ImageInfo& img, const string& strFile /*= ""*/) // returns the index of the image; if it already exists it's not added again; if it's invalid returns -1
{
    if (img.isNull()) { return -1; }
    int n (find(img));
    if (-1 != n)
    {
        if (!strFile.empty())
        {
            m_vTagWrtImageInfo[n].m_sstrFiles.insert(strFile);
        }
        return n;
    }

    m_vTagWrtImageInfo.push_back(TagWrtImageInfo(img, strFile));
    return size() - 1;
}


void ImageColl::addWidget(ImageInfoPanelWdgImpl* p) // first addImage gets called by TagWriter and after it's done it tells MainFormDlgImpl to create widgets, which calls this;
{
    m_vpWidgets.push_back(p);
}

void ImageColl::clear()
{
    clearPtrContainer(m_vpWidgets);
    m_vTagWrtImageInfo.clear();
    m_nCurrent = -1;
}

void ImageColl::select(int n) // -1 deselects all
{
    CB_ASSERT (cSize(m_vpWidgets) == cSize(m_vTagWrtImageInfo));

    if (m_nCurrent >= 0)
    {
        m_vpWidgets[m_nCurrent]->setNormalBackground();
    }
    m_nCurrent = n;

    if (n >= 0)
    {
        CB_ASSERT (n < size());
        m_vpWidgets[m_nCurrent]->setHighlightBackground();
    }
}


int ImageColl::find(const ImageInfo& img) const
{
    vector<TagWrtImageInfo>::const_iterator it (std::find(m_vTagWrtImageInfo.begin(), m_vTagWrtImageInfo.end(), img));
    if (m_vTagWrtImageInfo.end() == it) { return -1; }
    return it - m_vTagWrtImageInfo.begin();
}





//======================================================================================================================
//======================================================================================================================
//======================================================================================================================



TrackTextReader::TrackTextReader(SongInfoParser::TrackTextParser* pTrackTextParser, const std::string& s) :
    m_dRating(-1),
    m_bHasTitle(false),
    m_bHasArtist(false),
    m_bHasTrackNumber(false),
    m_bHasTimeStamp(false),
    m_bHasGenre(false),
    m_bHasAlbumName(false),
    m_bHasRating(false),
    m_bHasComposer(false),
    m_szType(pTrackTextParser->isFileNameBased() ? "(file)" : "(table)")
{
    vector<string> v ((int)LIST_END + 1, "\1"); // !!! "LIST_END + 1" instead of just "LIST_END", so the last entry can be used for "%i" (i.e. "ignored")
    pTrackTextParser->assign(s, v);

    if ("\1" != v[TITLE]) { m_bHasTitle = true; m_strTitle = v[TITLE]; }
    if ("\1" != v[ARTIST]) { m_bHasArtist = true; m_strArtist = v[ARTIST]; }
    if ("\1" != v[TRACK_NUMBER]) { m_bHasTrackNumber = true; m_strTrackNumber = v[TRACK_NUMBER]; }
    if ("\1" != v[TIME])
    {
        try
        {
            m_timeStamp = TagTimestamp(v[TIME]);
            m_bHasTimeStamp = true;
        }
        catch (const TagTimestamp::InvalidTime&)
        { // !!! nothing
        }
    }
    if ("\1" != v[GENRE]) { m_bHasGenre = true; m_strGenre = v[GENRE]; }
    //if ("\1" != v[IMAGE]) { q = true; m_str = v[IMAGE]; }
    if ("\1" != v[ALBUM]) { m_bHasAlbumName = true; m_strAlbumName = v[ALBUM]; }
    if ("\1" != v[RATING])
    {
        const string& s (v[RATING]);
        if (1 == cSize(s))
        {
            char c (s[0]);
            //                                 a    b    c    d    e    f    g    h    i    j    k    l    m    n    o    p   q   r    s    t    u    v    w    x    y    z
            static double s_ratingMap [] = { 5.0, 4.5, 4.0, 3.5, 3.0, 2.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 1.5, 1.0, 1.0, 1.0, -1, -1, 1.0, 1.0, 1.0, 1.0, 1.0, 0.5, 0.5, 0.0 }; // note that if this changes it should be synchronized with RatingPattern
            // negative values are ignored
            if (c >= 'a' && c <= 'z') //ttt3 ASCII-specific
            {
                double d (s_ratingMap[c - 'a']);
                if (d >= 0)
                {
                    m_bHasRating = true;
                    //m_dRating = ('z' - c)*254/('z' - 'a' /*+ 1*/) + 1;
                    //m_nRating = int(d*254/5 + 1.1);
                    m_dRating = d;
                }
            }
        }
    }
    if ("\1" != v[COMPOSER]) { m_bHasComposer = true; m_strComposer = v[COMPOSER]; }
}



/*override*/ TrackTextReader::~TrackTextReader()
{
    //qDebug("destr TrackTextReader %p", this);
}



/*override*/ TrackTextReader::SuportLevel TrackTextReader::getSupport(Feature eFeature) const
{
    switch (eFeature)
    {
    case TITLE:
    case ARTIST:
    case TRACK_NUMBER:
    case TIME:
    case GENRE:
    //case IMAGE:
    case ALBUM:
    case RATING:
    case COMPOSER:
    //case VARIOUS_ARTISTS:
        return READ_ONLY;

    default:
        return NOT_SUPPORTED;
    }
}


//======================================================================================================================
//======================================================================================================================
//======================================================================================================================



WebReader::WebReader(const AlbumInfo& albumInfo, int nTrackNo) : m_strType(albumInfo.m_strSourceName)
{
    //if (nTrackNo >= cSize(albumInfo.m_vTracks)) { return; }
    CB_ASSERT (0 <= nTrackNo && nTrackNo < cSize(albumInfo.m_vTracks));

    const TrackInfo& ti (albumInfo.m_vTracks[nTrackNo]);
    m_strTitle = ti.m_strTitle;
    m_strArtist = ti.m_strArtist; //(ti.m_strArtist.empty() ? albumInfo.m_strArtist : ti.m_strArtist);
    m_strTrackNumber = ti.m_strPos;
    try
    {
        m_timeStamp = TagTimestamp(albumInfo.m_strReleased);
    }
    catch (const TagTimestamp::InvalidTime&)
    { //ttt2 perhaps log something ...
    }
    m_strGenre = albumInfo.m_strGenre;
    m_imageInfo = albumInfo.m_imageInfo;
    m_strAlbumName = albumInfo.m_strTitle;
    m_dRating = ti.m_dRating;
    m_strComposer = ti.m_strComposer; //(ti.m_strComposer.empty() ? albumInfo.m_strComposer : ti.m_strComposer);
    m_eVarArtists = albumInfo.m_eVarArtists;

    if (DiscogsDownloader::SOURCE_NAME == albumInfo.m_strSourceName)
    {
        m_bSuppGenre = true;
        m_bSuppComposer = true;
        m_bSuppVarArtists = false;
    }
    else if (MusicBrainzDownloader::SOURCE_NAME == albumInfo.m_strSourceName)
    {
        m_bSuppGenre = false;
        m_bSuppComposer = false;
        m_bSuppVarArtists = true;
    }
    else
    {
        CB_ASSERT (false);
    }
}



WebReader::~WebReader()
{
//    qDebug("destr %p", this);
}



/*override*/ WebReader::SuportLevel WebReader::getSupport(Feature eFeature) const
{
    switch (eFeature)
    {
    case TITLE:
    case ARTIST:
    case TRACK_NUMBER:
    case TIME:
    case IMAGE:
    case ALBUM:
        return READ_ONLY;

    case GENRE:
        return m_bSuppGenre ? READ_ONLY : NOT_SUPPORTED;

    case COMPOSER:
        return m_bSuppComposer ? READ_ONLY : NOT_SUPPORTED;

    case VARIOUS_ARTISTS:
        return m_bSuppVarArtists ? READ_ONLY : NOT_SUPPORTED;

    case RATING:
    default:
        return NOT_SUPPORTED;
    }
}


int WebReader::convVarArtists() const // converts m_eVarArtists to an int is either 0 or contains all VA-enabled values, based on configuration
{
    switch (m_eVarArtists)
    {
    case AlbumInfo::VA_NOT_SUPP:
    case AlbumInfo::VA_SINGLE:
        return TagReader::VA_NONE;
    case AlbumInfo::VA_VARIOUS:
        {
            const CommonData* p (getCommonData());
            int nRes (0);
            if (p->m_bItunesVarArtists) { nRes += TagReader::VA_ITUNES; }
            if (p->m_bWmpVarArtists) { nRes += TagReader::VA_WMP; }
            return nRes;
        }
    default: CB_ASSERT (false);
    }
}


//======================================================================================================================
//======================================================================================================================
//======================================================================================================================

Mp3HandlerTagData::Mp3HandlerTagData(TagWriter* pTagWriter, const Mp3Handler* pMp3Handler, int nCrtPos, int nOrigPos, const std::string& strPastedVal) : m_pTagWriter(pTagWriter), m_pMp3Handler(pMp3Handler), m_nCrtPos(nCrtPos), m_nOrigPos(nOrigPos), m_vValueInfo(TagReader::LIST_END), m_strPastedVal(strPastedVal)
{
    TRACER("Mp3HandlerTagData constr");
    refreshReaders();
    setUp();
    //qDebug("create %p", this);
}


Mp3HandlerTagData::~Mp3HandlerTagData()
{
    //qDebug("destr %p", this);
    TRACER("Mp3HandlerTagData destr");
}

static bool isId3V2(const TagReader* p)
{
    return 0 == strcmp(p->getName(), Id3V230Stream::getClassDisplayName()) || 0 == strcmp(p->getName(), Id3V240Stream::getClassDisplayName());
}


static const char* g_szImageFmt ("# %d");
extern const int PIC_FMT_HDR (2);


void Mp3HandlerTagData::adjustVarArtists(bool b) // if VARIOUS_ARTISTS is not ASSIGNED, sets m_strValue and m_eStatus
{
    ValueInfo& inf (m_vValueInfo[TagReader::VARIOUS_ARTISTS]);
    string strAll (TagReader::getVarArtistsValue());

    if (ASSIGNED != inf.m_eStatus)
    {
        inf.m_strValue.clear();

        string s (b ? strAll : "");
        inf.m_strValue = s;
        inf.m_eStatus = s.empty() ? EMPTY : NON_ID3V2_VAL;

        for (int i = 0, n = cSize(m_vpTagReaders); i < n; ++i)
        {
            TagReader* p (m_vpTagReaders[i]);

            if (isId3V2(p))
            {
                string strId3 (p->getValue(TagReader::VARIOUS_ARTISTS));
                string s1; // those letters from strId3 that are also in strAll; needed because strId3 contains all known letters, regardless of their being enabled
                for (int i = 0; i < cSize(strId3); ++i)
                {
                    if (string::npos != strAll.find(strId3[i]))
                    {
                        s1 += strId3[i];
                    }
                }

                inf.m_eStatus = s1 == s ? ID3V2_VAL : NON_ID3V2_VAL;

                break;
            }
        }
    }
}


// to be called initially and each time the priority of tag readers changes; (well, actually a new object will get constructed in the latter case, so only the first matters)
void Mp3HandlerTagData::setUp()
{

//cout << "reload " << this; for (int i = 0, n = cSize(m_vpTagReaders); i < n; ++i) { cout << " " << m_vpTagReaders[i]; } cout << endl; //qDebug("%s", p->getName());

    for (int f = 0; f < TagReader::LIST_END; ++f)
    {
        if (TagReader::IMAGE != f && TagReader::VARIOUS_ARTISTS != f) // special case needed because TagReader::getValue(TagReader::IMAGE) always returns an empty string; VARIOUS_ARTISTS is handled separately anyway
        {
            ValueInfo& inf (m_vValueInfo[f]);
            if (ASSIGNED != inf.m_eStatus)
            {
                inf.m_strValue.clear();
                inf.m_eStatus = EMPTY;
                bool bId3V2Found (false);

                for (int i = 0, n = cSize(m_vpTagReaders); i < n; ++i)
                {
                    TagReader* p (m_vpTagReaders[i]);
                    bId3V2Found = bId3V2Found || isId3V2(p);

                    string s (p->getValue((TagReader::Feature)f));
                    if (!s.empty())
                    {
                        bool bId3V2Val (true);

                        if ((TagReader::ARTIST == f || TagReader::COMPOSER == f) && TC_NONE != m_pTagWriter->m_eArtistCase)
                        {
                            string s1 (convStr(getCaseConv(convStr(s), m_pTagWriter->m_eArtistCase)));
                            if (s1 != s)
                            {
                                s = s1;
                                bId3V2Val = false;
                            }
                        }

                        if ((TagReader::TITLE == f || TagReader::ALBUM == f) && TC_NONE != m_pTagWriter->m_eTitleCase) //ttt2 perhaps include genre as well
                        {
                            string s1 (convStr(getCaseConv(convStr(s), m_pTagWriter->m_eTitleCase)));
                            if (s1 != s)
                            {
                                s = s1;
                                bId3V2Val = false;
                            }
                        }

                        inf.m_strValue = s;

                        if (bId3V2Val)
                        {
                            bId3V2Val = false; // !!!
                            if (isId3V2(p))
                            {
                                bId3V2Val = true;
                            }
                            else if (bId3V2Found)
                            { // !!! nothing: an ID3V2 was already found and, since it got here, the value was empty
                            }
                            else
                            { // find the first ID3V2 and see if it has the same value
                                for (; i < n; ++i)
                                {
                                    TagReader* p1 (m_vpTagReaders[i]);
                                    if (isId3V2(p1))
                                    {
                                        string s1 (p1->getValue((TagReader::Feature)f));
                                        bId3V2Val = (s1 == s);
                                        break;
                                    }
                                }
                            }
                        }

                        inf.m_eStatus = bId3V2Val ? ID3V2_VAL : NON_ID3V2_VAL;
                        break;
                    }
                }
            }
        }
    }


    {
        ValueInfo& inf (m_vValueInfo[TagReader::IMAGE]);
        if (ASSIGNED != inf.m_eStatus)
        {
            inf.m_strValue.clear();
            inf.m_eStatus = EMPTY;
            bool bId3V2Found (false);

            for (int i = 0, n = cSize(m_vpTagReaders); i < n; ++i)
            {
                TagReader* p (m_vpTagReaders[i]);
                bId3V2Found = bId3V2Found || isId3V2(p);

                if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::IMAGE))
                {
                    const ImageInfo& imageInfo (p->getImage());
                    int k (m_pTagWriter->getIndex(imageInfo));
                    if (-1 != k)
                    {
                        char a [10];
                        sprintf(a, g_szImageFmt, k + 1);
                        inf.m_strValue = a;

                        // most of the follwing code is pointless as long as the only image-holding tags are ID3V2, but would lead to hard-to-detect bugs if others are added;

                        bool bId3V2Val (false);
                        //if (isId3V2(p) && ImageInfo::OK == imageInfo.getStatus()) // !!! by comparing getStatus() we force the user to save the image with a "correct" type
                        if (isId3V2(p) && (ImageInfo::OK == imageInfo.getStatus() || ImageInfo::LOADED_NOT_COVER == imageInfo.getStatus())) // ttt2 not sure about LOADED_NOT_COVER, but it seems to make more sense; probably most players don't care about the image type
                        {
                            bId3V2Val = true;
                        }
                        else if (bId3V2Found)
                        { // !!! nothing: an ID3V2 was already found and, since it got here, the value was empty
                        }
                        else
                        { // find the first ID3V2 and see if it has the same value
                            for (; i < n; ++i)
                            {
                                TagReader* p1 (m_vpTagReaders[i]);
                                if (isId3V2(p1))
                                {
                                    CB_ASSERT (TagReader::NOT_SUPPORTED != p1->getSupport(TagReader::IMAGE));

                                    const ImageInfo& imageInfo (p1->getImage());
                                    int k1 (m_pTagWriter->getIndex(imageInfo));
                                    bId3V2Val = (k1 == k);
                                    break;
                                }
                            }
                        }

                        inf.m_eStatus = bId3V2Val ? ID3V2_VAL : NON_ID3V2_VAL;
                        break;
                    }
                }
            }
        }
    }


    /*for (int i = 0; i < TagReader::LIST_END; ++i)
    {
        cout << "(" << m_vValueInfo[i].m_strValue << ", " << (int)m_vValueInfo[i].m_eStatus << ") ";
    }
    cout << endl;*/
}


// may throw InvalidValue
void Mp3HandlerTagData::setData(int nField, const std::string& s)
{
    if (s == m_vValueInfo[nField].m_strValue) { return; }

    if (TagReader::ARTIST == nField)
    {
        m_pTagWriter->delayedAdjVarArtists();
    }

    if (TagReader::TIME == nField)
    {
        try
        {
            TagTimestamp t (s);
        }
        catch (const TagTimestamp::InvalidTime&)
        {
            throw InvalidValue();
        }
    }

    if (TagReader::TRACK_NUMBER == nField && !s.empty()) // !!! note that an empty track# gets removed when writing to ID3V2
    {
        if (!isdigit(s[0]) || !isdigit(s[s.size() - 1])) { throw InvalidValue(); }
        string::size_type n1 (s.find_first_not_of("0123456789")), n2 (s.find_last_not_of("0123456789"));
        if (string::npos != n1 && (n1 != n2 || s[n1] != '/')) { throw InvalidValue(); }
    }

    string s1 (s);
    if (TagReader::RATING == nField && !s.empty())
    {
        bool bOk (isdigit(s[0]));
        if (bOk)
        {
            double d (atof(s.c_str()));
            if (d > 5)
            {
                bOk = false;
            }
            else
            {
                char a [15];
                sprintf(a, "%0.1f", d);
                s1 = a;
            }
        }
        if (!bOk)
        {
            throw InvalidValue();
        }
    }

    m_vValueInfo[nField].m_strValue = s1;
    m_vValueInfo[nField].m_eStatus = ASSIGNED;
    //ttt2 a check for track numbers would make some sense, but there are many formats for track numbers, e.g. "3", "03", "3/9", "03/09", "A03" (for cassettes), others for vinyl
}


void Mp3HandlerTagData::setStatus(int nField, Status eStatus)
{
    m_vValueInfo[nField].m_eStatus = eStatus;
}


//ttt2 ??? ptr deallocated and then reallocated in the same place most of the time?
// returns the data corresponding to the k-th element in m_pTagWriter->m_vTagReaderInfo; returns "\1" if it doesn't have a corresponding stream (e.g. 2nd ID3V1 tag), "\2" if the given feature is not supported (e.g. picture in ID3V1) and "\3" if this particular tag doesn't have the requested frame
// nField is the "internal" row for which data is retrieved, so TagReader::FEATURE_ON_POS[] has to be used by the UI caller
std::string Mp3HandlerTagData::getData(int nField, int k) const
{
    if (k >= cSize(m_vpMatchingTagReaders)) // this may be true in transitory contexts (e.g. going to prev/next album)
    {
        return "\1";
    }

    TagReader* p (m_vpMatchingTagReaders.at(k));
    if (0 == p) { return "\1"; }

    if (m_pTagWriter->isFastSaving() && (m_pTagWriter->m_vTagReaderInfo.at(k).m_strName == Id3V230Stream::getClassDisplayName() || m_pTagWriter->m_vTagReaderInfo[k].m_strName == Id3V240Stream::getClassDisplayName()))
    {
        return "N/A";
    }
//qDebug("%s", p->getName());
    //nField = TagReader::FEATURE_ON_POS[nField];
//qDebug("deref %p", p);
    bool bFrameExists;
    switch (nField)
    {
    case TagReader::TITLE: { if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::TITLE)) { return "\2"; } string s (p->getTitle(&bFrameExists)); return bFrameExists ? s : "\3"; }
    case TagReader::ARTIST: { if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::ARTIST)) { return "\2"; } string s (p->getArtist(&bFrameExists)); return bFrameExists ? s : "\3"; }
    case TagReader::TRACK_NUMBER: { if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::TRACK_NUMBER)) { return "\2"; } string s (p->getTrackNumber(&bFrameExists)); return bFrameExists ? s : "\3"; }
    case TagReader::TIME: { if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::TIME)) { return "\2"; } string s (p->getTime(&bFrameExists).asString()); return bFrameExists ? s : "\3"; }
    case TagReader::GENRE: { if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::GENRE)) { return "\2"; } string s (p->getGenre(&bFrameExists)); return bFrameExists ? s : "\3"; }
    case TagReader::IMAGE:
        {
            string qq ((m_vstrImgCache[k]));
            if (qq != "\7")
            {
                return qq;
            }
            if (m_vstrImgCache[k] != "\7") { return m_vstrImgCache[k]; }
            if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::IMAGE)) { m_vstrImgCache[k] = "\2"; return m_vstrImgCache[k]; }
            ImageInfo imageInfo (p->getImage(&bFrameExists));
            if (!bFrameExists) { m_vstrImgCache[k] = "\3"; return m_vstrImgCache[k]; }
            if (!imageInfo.isNull())
            {
                int j (m_pTagWriter->getIndex(imageInfo));
                // CB_ASSERT (j >= 0); // !!! no need to assert; m_pTagWriter already does it
                char a [10];
                sprintf(a, g_szImageFmt, j + 1);
//cout << "   " << a << endl;
                m_vstrImgCache[k] = a; return m_vstrImgCache[k];
            }
            m_vstrImgCache[k] = ""; return m_vstrImgCache[k];
        }
    case TagReader::ALBUM: { if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::ALBUM)) { return "\2"; } string s (p->getAlbumName(&bFrameExists)); return bFrameExists ? s : "\3"; }
    case TagReader::RATING:
        {
            if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::RATING)) { return "\2"; }
            char a [15];
            double d (p->getRating(&bFrameExists));
            if (!bFrameExists) { return "\3"; }
            //ttt2 perhaps use this: return p->getValue(TagReader::RATING); see why there's no test for "<0" in TagReader::getValue()
            sprintf(a, "%0.1f", d);
            if ('-' == a[0]) { a[0] = 0; }
            return a;
        }
    case TagReader::COMPOSER: { if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::COMPOSER)) { return "\2"; } string s (p->getComposer(&bFrameExists)); return bFrameExists ? s : "\3"; }
    case TagReader::VARIOUS_ARTISTS:
        {
            if (TagReader::NOT_SUPPORTED == p->getSupport(TagReader::VARIOUS_ARTISTS)) { return "\2"; }
            p->getVariousArtists(&bFrameExists);
            if (!bFrameExists) { return "\3"; }
            return p->getValue(TagReader::VARIOUS_ARTISTS);
        }
    }

    CB_ASSERT(false); // all cases have been covered and have "return"
}


int Mp3HandlerTagData::getImage() const // 0-based; -1 if there-s no image;
{
    string s (getData(TagReader::IMAGE));
    int nPic (s.empty() ? -1 : atoi(s.c_str() + PIC_FMT_HDR) - 1); // "-1" because on screen numbering starts from 1, not 0
    return nPic;
}

double Mp3HandlerTagData::getRating() const
{
    string s (getData(TagReader::RATING));
    double d (s.empty() ? -1 : atof(s.c_str())); // !!! it's OK to use atof, regardless of the system's locale, because a program's locale is "C" unless changed explicitley ("locale::global(locale(""));" would switch to the system's locale)
    return d;
}

// updates m_vpTagReader to reflect m_pTagWriter->m_vTagReaderInfo, then updates unassigned values;
// called on constructor
void Mp3HandlerTagData::refreshReaders()
{
    m_vTrackTextReaders.clear();
    m_vpTagReaders.clear();
    vector<TagReaderInfo>& vRdInfo (m_pTagWriter->m_vTagReaderInfo);
    m_vpMatchingTagReaders.clear(); m_vpMatchingTagReaders.resize(vRdInfo.size());
    vector<std::string>(vRdInfo.size(), string("\7")).swap(m_vstrImgCache);//.clear(); m_vstrImgCache.resize();

    m_vTrackTextReaders.reserve(m_pTagWriter->getTrackTextParsersCnt()); // !!! without this pointers would get invalidated
    m_vWebReaders.reserve(m_pTagWriter->getAlbumInfoCnt()); // !!! without this pointers would get invalidated

    for (int i = 0, n = cSize(vRdInfo); i < n; ++i)
    {
        const TagReaderInfo& inf (vRdInfo[i]);
        if (inf.m_strName == TrackTextReader::getClassDisplayName())
        {
            SongInfoParser::TrackTextParser* pParser (m_pTagWriter->getTrackTextParser(inf.m_nPos));

            m_vTrackTextReaders.push_back(TrackTextReader(pParser, pParser->isFileNameBased() ? m_pMp3Handler->getName() : m_strPastedVal));
            m_vpTagReaders.push_back(&m_vTrackTextReaders.back());
            m_vpMatchingTagReaders[i] = &m_vTrackTextReaders.back();
            //qDebug("asgn %p", &m_vTrackTextReaders.back());
        }
        else if (inf.m_strName == WebReader::getClassDisplayName())
        {
            const AlbumInfo& albumInfo (m_pTagWriter->getAlbumInfo(inf.m_nPos));
            if (m_nCrtPos < cSize(albumInfo.m_vTracks))
            {
                m_vWebReaders.push_back(WebReader(albumInfo, m_nCrtPos));
                m_vpTagReaders.push_back(&m_vWebReaders.back());
                m_vpMatchingTagReaders[i] = &m_vWebReaders.back();
            }
        }
        else
        {
            TagReader* pLast (0);
            int nCount (0);
            const vector<DataStream*>& vStreams (m_pMp3Handler->getStreams());
            for (int j = 0, n = cSize(vStreams); j < n; ++j)
            {
                TagReader* pRd (dynamic_cast<TagReader*>(vStreams[j]));
                if (0 != pRd && pRd->getName() == inf.m_strName)
                {
                    if (inf.m_nPos == nCount)
                    {
                        m_vpTagReaders.push_back(pRd);
                        m_vpMatchingTagReaders[i] = pRd;
                        goto e1;
                    }
                    ++nCount;
                    pLast = pRd;
                }
            }
            if (0 != pLast)
            {
                m_vpTagReaders.push_back(pLast); // wasn't able to find the one on the specified position, so it uses a compatible one; // !!! this seems better than doing nothing: if TagWriter has the order "<ID3V2.3.0, 1>, <ID3V1, 0>, <ID3V2.3.0, 0>" and a file only has an ID3V2.3.0 and an ID3V1, doing nothing for the first (because there's no ID3V2.3.0 with a pos=1) would result in the order for that file being "<ID3V1, 0>, <ID3V2.3.0, 0>", making ID3V1 have priority; with the current implementation, the order is "<ID3V2.3.0, 0>, <ID3V1, 0>, <ID3V2.3.0, 0>", ID3V2 has priority; true, this introduces a needles duplicate but it's no big deal; //ttt3 remove duplicates
            }

            // !!! not an exact match, so don't touch m_vpMatchingTagReaders

e1:;
        }
    }

    //qDebug("1 %p", this); for (int i = 0, n = cSize(m_vpTagReaders); i < n; ++i) { qDebug("%p", m_vpTagReaders[i]); } //qDebug("%s", p->getName());

    //cout << "update " << this; for (int i = 0, n = cSize(m_vpTagReaders); i < n; ++i) { cout << " " << m_vpTagReaders[i]; } cout << endl;
}

void Mp3HandlerTagData::print(std::ostream& out) const
{
    out << "==========================================\n";
    for (int i = 0, n = cSize(m_vValueInfo); i < n; ++i)
    {
        out << m_vValueInfo[i].m_strValue << " <" << m_vValueInfo[i].m_eStatus << ">\n";
    }
}


// returns 0 if i is out of range
const TagReader* Mp3HandlerTagData::getMatchingReader(int i) const
{
    if (i < 0 || i > cSize(m_vpMatchingTagReaders)) { return 0; }
    return m_vpMatchingTagReaders[i];
}


//======================================================================================================================
//======================================================================================================================
//======================================================================================================================



TagWriter::TagWriter(CommonData* pCommonData, QWidget* pParentWnd, const bool& bIsFastSaving, const TextCaseOptions& eArtistCase, const TextCaseOptions& eTitleCase) : m_pCommonData(pCommonData), m_pParentWnd(pParentWnd), m_nCurrentFile(-1), m_bShowedNonSeqWarn(true), m_bIsFastSaving(bIsFastSaving), m_nFileToErase(-1), m_bVariousArtists(false), m_bAutoVarArtists(false), m_bDelayedAdjVarArtists(false), m_bWaitingChangeNotif(false), m_eArtistCase(eArtistCase),  m_eTitleCase(eTitleCase)
{
}


TagWriter::~TagWriter()
{
    clearPtrContainer(m_vpTrackTextParsers);
    clearPtrContainer(m_vpMp3HandlerTagData);
}


// should be called on startup by MainFormDlgImpl, to get config data; asserts that m_vSortedKnownTagReaders is empty;
void TagWriter::addKnownInf(const std::vector<TagReaderInfo>& v)
{
    CB_ASSERT (m_vSortedKnownTagReaders.empty());
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        vector<TagReaderInfo>::iterator it (std::find(m_vSortedKnownTagReaders.begin(), m_vSortedKnownTagReaders.end(), v[i]));
        if (m_vSortedKnownTagReaders.end() == it)
        {
            m_vSortedKnownTagReaders.insert(m_vSortedKnownTagReaders.end(), v[i]);
        }
        // else nothing (ignore duplicates)
    }
}


// sorts m_vTagReaderInfo so that it matches m_vSortedKnownTagReaders
void TagWriter::sortTagReaders()
{
    vector<TagReaderInfo> vSorted;
    //vector<TagReaderInfo> vSorted;
    for (int i = 0, n = cSize(m_vSortedKnownTagReaders); i < n; ++i)
    {
        vector<TagReaderInfo>::iterator it (std::find(m_vTagReaderInfo.begin(), m_vTagReaderInfo.end(), m_vSortedKnownTagReaders[i]));
        if (m_vTagReaderInfo.end() != it)
        {
            vSorted.push_back(*it);
            m_vTagReaderInfo.erase(it);
        }
    }

    m_vSortedKnownTagReaders.insert(m_vSortedKnownTagReaders.end(), m_vTagReaderInfo.begin(), m_vTagReaderInfo.end()); // by doing this we make sure m_vSortedKnownTagReaders has all the elements from m_vTagReaderInfo (and perhaps more)
    vSorted.insert(vSorted.end(), m_vTagReaderInfo.begin(), m_vTagReaderInfo.end());
    vSorted.swap(m_vTagReaderInfo);
}


void TagWriter::addAlbumInfo(const AlbumInfo& albumInfo)
{
    int nAlbInfCnt (getAlbumInfoCnt()); // excluding the new albumInfo to be added
    m_vAlbumInfo.push_back(albumInfo);
    int nKnownWebCnt (0);
    int n (cSize(m_vSortedKnownTagReaders));
    for (int i = 0; i < n; ++i)
    {
        if (WebReader::getClassDisplayName() == m_vSortedKnownTagReaders[i].m_strName)
        {
            ++nKnownWebCnt;
        }
    }

    m_vSortedKnownTagReaders.insert(m_vSortedKnownTagReaders.begin(), TagReaderInfo(WebReader::getClassDisplayName(), nAlbInfCnt, TagReaderInfo::ALONE)); // last param doesn't matter for m_vSortedKnownTagReaders

    if (nAlbInfCnt < nKnownWebCnt)
    { // we have another TagReaderInfo with the same m_nPos as the one just added; it needs to be removed;
        for (int i = 1; i < n; ++i) // !!! start from 1
        {
            if (WebReader::getClassDisplayName() == m_vSortedKnownTagReaders[i].m_strName && m_vSortedKnownTagReaders[i].m_nPos == nAlbInfCnt)
            {
                m_vSortedKnownTagReaders.erase(m_vSortedKnownTagReaders.begin() + i);
                break;
            }
        }
    }

    reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);
}



namespace
{
    struct SortByTrack
    {
        enum { NO_TRACK = 999999 };
        static double getTrack(const string& s) // returns -1 if it can't identify a track number
        {
            const char* p (s.c_str()); // requiring all chars to be digits isn't OK, because "5/12" should be handled as "5"
            for (; ' ' == *p; ++p) {}
            if (0 == *p || !isdigit(*p)) { return NO_TRACK; }
            char* pLast;
            double d (int(strtol(p, &pLast, 10)));
            for (; 0 != *pLast && !isdigit(*pLast); ++pLast) {}
            if (0 != *pLast)
            {
                d += atoi(pLast)/100000.0; // this sort of takes care of tracks in the format <total>/<track>
            }
            return d;
        }

        bool operator()(const Mp3HandlerTagData* p1, const Mp3HandlerTagData* p2) const
        {
            // !!! this defines a "strict partial order" (irreflexive and transitive) iff the codes for digits are grouped together (as is the case with ASCII)
            const string& s1 (p1->getData(TagReader::TRACK_NUMBER));
            const string& s2 (p2->getData(TagReader::TRACK_NUMBER));
            double d1 (getTrack(s1));
            double d2 (getTrack(s2));
            //if (d1 < NO_TRACK && d2 < NO_TRACK) { return d1 < d2; }
            if (d1 != d2) { return d1 < d2; }
            return s1 < s2;
        }
    };
}



void TagWriter::sortSongs() // sorts by track number; shows a warning if issues are detected (should be exactly one track number, from 1 to the track count)
{
    stable_sort(m_vpMp3HandlerTagData.begin(), m_vpMp3HandlerTagData.end(), SortByTrack());
    int n (cSize(m_vpMp3HandlerTagData));
    vector<pair<int, int> > v (n);
    for (int i = 0; i < n; ++i)
    {
        v[i] = make_pair(m_vpMp3HandlerTagData[i]->getOrigPos(), i);
    }
    std::sort(v.begin(), v.end());
    m_vnMovedTo.resize(n);
    for (int i = 0; i < n; ++i)
    {
        m_vnMovedTo[i] = v[i].second;
        //cout << "## " << m_vnMovedTo[i] << endl;
    }

    m_bNonStandardTrackNo = false;
    for (int i = 0; i < n; ++i)
    {
        if (int(SortByTrack::getTrack(m_vpMp3HandlerTagData[i]->getData(TagReader::TRACK_NUMBER))) != i + 1)
        {
            m_bNonStandardTrackNo = true;
            if (m_pCommonData->m_bWarnOnNonSeqTracks && !m_bShowedNonSeqWarn)
            {
                m_bShowedNonSeqWarn = true;
                QTimer::singleShot(1, this, SLOT(onDelayedTrackSeqWarn()));
            }
            break;
        }
    }

}

void TagWriter::onDelayedTrackSeqWarn()
{
    //CursorOverrider crs (Qt::ArrowCursor);
    QMessageBox::warning(m_pParentWnd, "Warning", "Track numbers are supposed to be consecutive numbers from 1 to the total number of tracks. This is not the case with the current album, which may lead to incorrect assignments when pasting information.");
}


// returns 0 if there's no current handler
const Mp3Handler* TagWriter::getCurrentHndl() const
{
    if (m_nCurrentFile < 0 || m_nCurrentFile >= cSize(m_vpMp3HandlerTagData)) { return 0; }
    return m_vpMp3HandlerTagData[m_nCurrentFile]->getMp3Handler();
}


string TagWriter::getCurrentName() const
{
    const Mp3Handler* p (getCurrentHndl());
    return 0 != p ? p->getName() : "";
}


const Mp3HandlerTagData* TagWriter::getCrtMp3HandlerTagData() const
{
    if (m_nCurrentFile < 0 || m_nCurrentFile >= cSize(m_vpMp3HandlerTagData)) { return 0; }
    return m_vpMp3HandlerTagData[m_nCurrentFile];
}

bool s_bToldAboutPatternsInCrtRun (false); // to limit to 1 per run the number of times the user is told about support


void TagWriter::onDelayedChangeNotif()
{
    m_bWaitingChangeNotif = false;
    QMessageBox::critical(m_pParentWnd, "Error", "Some files have been modified by an external tool after the last scan. You won't be able to save any changes to those files until you rescan them.");
}



//void TagWriter::reloadAll(string strCrt, ReloadOption eReloadOption/*, bool bKeepUnassgnImg*/)
void TagWriter::reloadAll(string strCrt, bool bClearData, bool bClearAssgn)
{
    TRACER("TagWriter::reloadAll()");
    CursorOverrider crs;

    if (strCrt.empty() && !bClearData)
    {
        strCrt = getCurrentName();
    }
//ttt2 ??? q ? m_vpMp3HandlerTagData gets destroyed then prev values used?" x
//ttt2 ??? ? push_back for m_vpMp3HandlerTagData ?
    vector<Mp3HandlerTagData*> v; v.swap(m_vpMp3HandlerTagData);
    const deque<const Mp3Handler*>& vpHndl (m_pCommonData->getCrtAlbum());
    if (!v.empty() && cSize(v) != cSize(vpHndl))
    {
        bClearData = true; // needed for the case when proc orig files are kept or when changed files are nor created
    }

//qDebug("------------------------- dir %s -------------------------", vpHndl[0]->getDir().c_str());
    if (bClearData)
    {
//qDebug(">>>>>>>>>>>>>>>>>>>>>>>>> initial >>>>>>>>>>>>>>>>>>>>>>>>>");
        clearPtrContainer(v);
//qDebug("<<<<<<<<<<<<<<<<<<<<<<<<< initial <<<<<<<<<<<<<<<<<<<<<<<<<");
        m_imageColl.clear();
        m_vnMovedTo.clear();
        m_vstrPastedValues.clear();
        m_vAlbumInfo.clear();
        m_snUnassignedImages.clear();
    }

    if (bClearAssgn)
    {
        m_bAutoVarArtists = true;
    }


    m_vTagReaderInfo.clear();

    set<pair<string, int> > sReaders;
    std::map<std::string, int> mReaderCount;

    if (vpHndl.empty())
    {
        m_nCurrentFile = -1;
        //emit fileChanged(); // just to resize the rows
        emit albumChanged(); // this will cause the window to close
        return; // !!! don't add a column for which data doesn't make sense (without this, "pattern" readers would get added, because they exist independently of songs)
    }

    CB_ASSERT (v.empty() || cSize(v) == cSize(vpHndl));

    int n (cSize(vpHndl));
    bool bFullReaderNotFound (false); // ID3V1 is not full, while the others are

    for (int i = 0; i < n; ++i)
    {
        const Mp3Handler* p (vpHndl[i]);
        std::map<std::string, int> m;

        bool bTrackHasFullReader (false);

        const vector<DataStream*>& vStreams (p->getStreams());
        for (int i = 0, n = cSize(vStreams); i < n; ++i)
        {
            TagReader* pRd (dynamic_cast<TagReader*>(vStreams[i]));
            if (0 != pRd)
            {
                sReaders.insert(make_pair(string(pRd->getName()), m[pRd->getName()]));
                ++m[pRd->getName()];// = m[pRd->getName()] + 1;

                if (pRd->getName() != Id3V1Stream::getClassDisplayName()) { bTrackHasFullReader = true; }

                mReaderCount[pRd->getName()] = max(mReaderCount[pRd->getName()], m[pRd->getName()]);

                if (bClearData)
                { // add images
                    try
                    {
                        const vector<ImageInfo> vImg (pRd->getImages());
                        for (int i = 0; i < cSize(vImg); ++i)
                        {
                            m_imageColl.addImage(vImg[i]); // doesn't matter if it's null or already exists; m_imageColl handles it correctly
                        }
                    }
                    catch (const NotSupportedOp&)
                    {
                    }
                }
            }
        }

        if (!bTrackHasFullReader) { bFullReaderNotFound = true; }
    }

    for (int i = 0, n = cSize(m_vpTrackTextParsers); i < n; ++i)
    {
        if (m_snActivePatterns.count(i) > 0)
        {
            sReaders.insert(make_pair(string(TrackTextReader::getClassDisplayName()), i));
        }
    }

    mReaderCount[TrackTextReader::getClassDisplayName()] = cSize(m_vpTrackTextParsers);

    for (int i = 0, n = cSize(m_vAlbumInfo); i < n; ++i)
    {
        sReaders.insert(make_pair(string(WebReader::getClassDisplayName()), i));
    }
    mReaderCount[WebReader::getClassDisplayName()] = cSize(m_vAlbumInfo);

    for (set<pair<string, int> >::iterator it = sReaders.begin(), end = sReaders.end(); it != end; ++it)
    {
        const pair<string, int>& rd (*it);
        m_vTagReaderInfo.push_back(TagReaderInfo(rd.first, rd.second, 1 == mReaderCount[rd.first]));
    }

    sortTagReaders();

    if (m_vnMovedTo.empty())
    { // needed both at the first run and after eReloadOption is CLEAR
        m_vnMovedTo.resize(vpHndl.size());
        for (int i = 0; i < n; ++i)
        {
            m_vnMovedTo[i] = i;
        }
    }

    bool bRescanNeeded (false);
    { // process handlers in the order in which they appear in vpHndl;
        m_vpMp3HandlerTagData.resize(n);
        int nPastedSize (cSize(m_vstrPastedValues));
        for (int i = 0; i < n; ++i)
        {
            int k (m_vnMovedTo[i]);
            const Mp3Handler* p (vpHndl[i]);
            bRescanNeeded = bRescanNeeded || p->needsReload(m_pCommonData->useFastSave());
            m_vpMp3HandlerTagData[k] = new Mp3HandlerTagData(this, p, k, i, k < nPastedSize ? m_vstrPastedValues[k] : "");
            //cout << "k=" << k << ", i=" << i << ", val=" << (k < nPastedSize ? m_vstrPastedValues[k] : "") << endl;
            if (!v.empty() && !bClearAssgn)
            { // copy existing value and status for "assigned" elements
                const Mp3HandlerTagData& src (*v[k]);
                Mp3HandlerTagData& dest (*m_vpMp3HandlerTagData[k]);
                //src.print();
                //dest.print();
                for (int j = 0; j < TagReader::LIST_END; ++j)
                {
                    if (Mp3HandlerTagData::ASSIGNED == src.getStatus(j)) // !!! TagReader::FEATURE_ON_POS[] doesn't matter, because it's just a permutation, which is applied to both src and dest
                    {
                        dest.setData(j, src.getData(j));
                        dest.setStatus(j, Mp3HandlerTagData::ASSIGNED);
                    }
                }
            }
        }

        adjustVarArtists();
    }

    if (bRescanNeeded && !m_bWaitingChangeNotif)
    {
        m_bWaitingChangeNotif = true;
        QTimer::singleShot(1, this, SLOT(onDelayedChangeNotif()));
    }


    m_bShouldShowPatternsNote = false;
    if (!m_pCommonData->m_bToldAboutPatterns && !s_bToldAboutPatternsInCrtRun)
    {
        bool bIncompleteInfo (false); // turns true if a track is found that has empty artist, track number, or title; or if it only has ID3V1

        if (!bFullReaderNotFound)
        {
            for (int i = 0; i < n; ++i)
            {
                if (m_vpMp3HandlerTagData[i]->getData(TagReader::TRACK_NUMBER).empty() || m_vpMp3HandlerTagData[i]->getData(TagReader::TITLE).empty() || m_vpMp3HandlerTagData[i]->getData(TagReader::ALBUM).empty() || m_vpMp3HandlerTagData[i]->getData(TagReader::ARTIST).empty())
                {
                    bIncompleteInfo = true;
                    break;
                }
            }
        }

        if (bIncompleteInfo || bFullReaderNotFound)
        {
            m_bShouldShowPatternsNote = true;
        }
    }


    if (v.empty())
    {
        sortSongs();
    }
//qDebug(">>>>>>>>>>>>>>>>>>>>>>>>> after merge >>>>>>>>>>>>>>>>>>>>>>>>>");
    clearPtrContainer(v);
//qDebug("<<<<<<<<<<<<<<<<<<<<<<<<< after merge <<<<<<<<<<<<<<<<<<<<<<<<<");

    if (bClearData)
    { // add images from crt dir
        if (n > 0)
        { // scan crt dir for images //ttt2 this needs rewrite if an album means something else than a directory
            CursorOverrider crs;
            string strDir (vpHndl[0]->getDir());
            FileSearcher fs (strDir);

            while (fs)
            {
                if (fs.isFile())
                {
                    string s (fs.getName());
                    QString qs (convStr(s));
                    if (qs.endsWith(".jpg", Qt::CaseInsensitive) || qs.endsWith(".jpeg", Qt::CaseInsensitive) || qs.endsWith(".png", Qt::CaseInsensitive))
                    {
                        addImgFromFile(qs, CONSIDER_ASSIGNED);
                    }
                }

                fs.findNext();
            }
        }

        //emit tagWriterChanged(); // !!! needed here to avoid an assert that would be triggered by pSelModel->setCurrentIndex() when detecting a mismatch between the number of images in m_pCommonData->m_imageColl and the widgets that show them
    }


//printVec(m_vTagReaderInfo);
//printVec(m_vSortedKnownTagReaders);

    emit albumChanged(/*DONT_CLEAR == eReloadOption*/);
    emit imagesChanged();
    setCrt(strCrt);
    emit varArtistsUpdated(m_bVariousArtists);
}


void TagWriter::adjustVarArtists()
{
    if (!m_bAutoVarArtists) { return; } // !!! this gets called from reloadAll(); without the test it would lead to overriding whatever the user set

    bool bPrev (m_bVariousArtists);
    m_bVariousArtists = false;
    int n (cSize(m_vpMp3HandlerTagData));

    if (n > 1)
    {
        string strArtist (m_vpMp3HandlerTagData[0]->getData(TagReader::ARTIST));

        for (int i = 1; i < n; ++i)
        {
            if (strArtist != m_vpMp3HandlerTagData[i]->getData(TagReader::ARTIST))
            {
                m_bVariousArtists = true;
                break;
            }
        }
    }

    for (int i = 0; i < n; ++i)
    {
        m_vpMp3HandlerTagData[i]->adjustVarArtists(m_bVariousArtists);
    }

    if (bPrev != m_bVariousArtists)
    {
        emit varArtistsUpdated(m_bVariousArtists);
    }
}


void TagWriter::toggleVarArtists()
{
    m_bAutoVarArtists = false;
    m_bVariousArtists = !m_bVariousArtists;
    string s (m_bVariousArtists ? TagReader::getVarArtistsValue() : "");

    set<int> snSelSongs;
    for (set<OrigValue>::iterator it = m_sSelOrigVal.begin(), end = m_sSelOrigVal.end(); it != end; ++it)
    {
        const OrigValue& o (*it);
        if (o.m_nField == (int)TagReader::VARIOUS_ARTISTS)
        {
            snSelSongs.insert(o.m_nSong);
        }
    }

    for (int i = 0; i < cSize(m_vpMp3HandlerTagData); ++i)
    {
        Mp3HandlerTagData* p (m_vpMp3HandlerTagData[i]);
        if (Mp3HandlerTagData::ASSIGNED != p->getStatus(TagReader::VARIOUS_ARTISTS) || m_nCurrentFile == i || snSelSongs.count(i) > 0)
        {
            p->setData(TagReader::VARIOUS_ARTISTS, s);
        }
    }

    emit albumChanged();
    emit varArtistsUpdated(m_bVariousArtists);
}


void TagWriter::delayedAdjVarArtists()
{
    if (m_bDelayedAdjVarArtists) { return; }

    m_bDelayedAdjVarArtists = true;
    QTimer::singleShot(1, this, SLOT(onDelayedAdjVarArtists()));
}

void TagWriter::onDelayedAdjVarArtists()
{
    m_bDelayedAdjVarArtists = false;
    adjustVarArtists();
    emit albumChanged();
}



void TagWriter::setCrt(const std::string& strCrt)
{
    m_nCurrentFile = 0;
    for (int i = 0, n = cSize(m_vpMp3HandlerTagData); i < n; ++i)
    {
        if (strCrt == m_vpMp3HandlerTagData[i]->getMp3Handler()->getName())
        {
            m_nCurrentFile = i;
            break;
        }
    }

    emit fileChanged();
}


void TagWriter::setCrt(int nCrt)
{
    CB_ASSERT (0 <= nCrt && nCrt < cSize(m_vpMp3HandlerTagData));
    m_nCurrentFile = nCrt;
//qDebug("2 m_nCurrentFile=%d", m_nCurrentFile);
    emit fileChanged();
}





/*

1) updatePatterns() is called, which updates m_vpTrackTextParsers and m_vSortedKnownTagReaders, then calls reloadAll()
2) reloadAll() gets all readers from current album and from m_vpTrackTextParsers and puts them in m_vTagReaderInfo
3) reloadAll() sorts m_vTagReaderInfo, so that it matches m_vSortedKnownTagReaders;
4) whatever elements remain in m_vTagReaderInfo without a corresponding element in m_vSortedKnownTagReaders will be at the right; they also get added to m_vSortedKnownTagReaders and at app exit they get saved in the conf file;

*/

// the int tells which position a given pattern occupied before; (it's -1 for new patterns);
// doesn't throw, but invalid patterns are discarded; it returns false if at least one pattern was discarded;
bool TagWriter::updatePatterns(const std::vector<std::pair<std::string, int> >& v)
{
    set<string> sstrActive;
    {
        vector<string> v (getPatterns());
        set<int> s (getActivePatterns());
        for (set<int>::const_iterator it = s.begin(); it != s.end(); ++it)
        {
            sstrActive.insert(v[*it]);
        }
    }

    clearPtrContainer(m_vpTrackTextParsers);
    bool bRes (true);

    for (int i = 0, n = cSize(v); i < n; ++i)
    {
//cout << v[i].first << " " << v[i].second << endl; ? ok, but
        try
        {
            m_vpTrackTextParsers.push_back(new SongInfoParser::TrackTextParser(v[i].first));
        }
        catch (const SongInfoParser::TrackTextParser::InvalidPattern&)
        {
            // ignore the pattern and report that there was an error
            bRes = false;
        }
    }

    {
        // update m_vSortedKnownTagReaders based on v[i].second
        vector<TagReaderInfo> vNew;
        for (int i = 0, n = cSize(m_vSortedKnownTagReaders); i < n; ++i)
        {
            bool bAdd (true);
            TagReaderInfo inf (m_vSortedKnownTagReaders[i]);
            if (TrackTextReader::getClassDisplayName() == inf.m_strName)
            {
                int j (0);
                const int m (cSize(v));
                for (; j < m; ++j)
                {
                    if (v[j].second == inf.m_nPos)
                    {
                        inf.m_nPos = j;
                        break;
                    }
                }

                if (m == j)
                {
                    bAdd = false;
                }
            }

            if (bAdd)
            {
                vNew.push_back(inf);
            }
        } // !!! elements from v with a pos of -1 don't get added to vNew (therefore m_vSortedKnownTagReaders), which is OK: they will be visible anyway (because they are added to m_vpTrackTextParsers), but they will be placed at the end of the list (as having no correspondence in m_vSortedKnownTagReaders when sortTagReaders() gets called);

        vNew.swap(m_vSortedKnownTagReaders);
    }

    {
        vector<string> v (getPatterns());
        m_snActivePatterns.clear();
        for (int i = 0; i < cSize(v); ++i)
        {
            if (sstrActive.count(v[i]) > 0)
            {
                m_snActivePatterns.insert(i); //ttt2 not quite right: for 2 identical patterns of which one was selected before, both will be selected now;
            }
        }
    }

    reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);

    return bRes;
}


vector<string> TagWriter::getPatterns() const
{
    vector<string> v;
    for (int i = 0, n = cSize(m_vpTrackTextParsers); i < n; ++i)
    {
        v.push_back(m_vpTrackTextParsers[i]->getPattern()); // !!! may throw
    }
    return v;
}


void TagWriter::setActivePatterns(const std::set<int>& s)
{
    m_snActivePatterns = s;
    reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);
}


int TagWriter::getIndex(const ImageInfo& img) const
{
//return 0;
    return m_imageColl.find(img);
}


void TagWriter::moveReader(int nOldVisualIndex, int nNewVisualIndex)
{
    int n (cSize(m_vTagReaderInfo));
    CB_ASSERT (nOldVisualIndex >=0 && nOldVisualIndex < n && nNewVisualIndex >=0 && nNewVisualIndex < n);

    TagReaderInfo inf (m_vTagReaderInfo[nOldVisualIndex]);
    m_vTagReaderInfo.erase(m_vTagReaderInfo.begin() + nOldVisualIndex);
    m_vTagReaderInfo.insert(m_vTagReaderInfo.begin() + nNewVisualIndex, inf);

//printVec(m_vTagReaderInfo);
//printVec(m_vSortedKnownTagReaders);

    vector<TagReaderInfo>::iterator it (find(m_vSortedKnownTagReaders.begin(), m_vSortedKnownTagReaders.end(), inf));
    CB_ASSERT (m_vSortedKnownTagReaders.end() != it);
    m_vSortedKnownTagReaders.erase(it);
    if (0 == nNewVisualIndex)
    {
        m_vSortedKnownTagReaders.insert(m_vSortedKnownTagReaders.begin(), inf);
    }
    else
    {
        if (nNewVisualIndex < n - 1)
        {
            it = find(m_vSortedKnownTagReaders.begin(), m_vSortedKnownTagReaders.end(), m_vTagReaderInfo[nNewVisualIndex + 1]);
            CB_ASSERT (m_vSortedKnownTagReaders.end() != it); //assert when move col 0 over 1
        }
        else
        {
            CB_ASSERT (nNewVisualIndex > 0); // well, it's n-1
            it = find(m_vSortedKnownTagReaders.begin(), m_vSortedKnownTagReaders.end(), m_vTagReaderInfo[nNewVisualIndex - 1]);
            CB_ASSERT (m_vSortedKnownTagReaders.end() != it);
            ++it;
        }

        m_vSortedKnownTagReaders.insert(it, inf);
    }

    reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);
}



// should be called when the selection changes; updates m_sSelOrigVal and returns the new state of m_pAssignedB;
AssgnBtnWrp::State TagWriter::updateAssigned(const vector<pair<int, int> >& vFields)
{
//cout << ">>> updateAssigned()\n"; printContainer(m_sSelOrigVal, cout, "\n");
    set<OrigValue> s;
    int nAsgnCount (0);
    for (int i = 0, n = cSize(vFields); i < n; ++i)
    {
        int nSong (vFields[i].first);
        int nField (vFields[i].second);
        //cout << "look at " << nSong << ":" << nField << endl;
        //nField = TagReader::FEATURE_ON_POS[nField];

        //const Mp3HandlerTagData& d (*m_pTagWriter->m_vpMp3HandlerTagData[nSong]);

        OrigValue val (OrigValue(nSong, nField, getData(nSong, nField), getStatus(nSong, nField)));
        set<OrigValue>::iterator it (m_sSelOrigVal.find(val));
        const OrigValue& insVal (it == m_sSelOrigVal.end() ? val : *it); // !!! the test is needed because after toggling from assigned to unassigned the shown value changes to reflect the first tag; then when toggling back we need the old value, to restore
        //cout << "   adding " << insVal.m_nSong << ":" << insVal.m_nField << " " << insVal.m_strVal << endl;
        s.insert(insVal);
        //if (insVal.m_eStatus nAsgnCount
        if (Mp3HandlerTagData::ASSIGNED == val.m_eStatus) // !!! val, not insVal
        {
            ++nAsgnCount;
        }
    }
    s.swap(m_sSelOrigVal);

    //Mp3HandlerTagData::Status eStatus (m_pTagWriter->getStatus(index.row(), index.column()));
    AssgnBtnWrp::State eRes (0 == nAsgnCount ? AssgnBtnWrp::NONE_ASSGN : (nAsgnCount < cSize(m_sSelOrigVal) ? AssgnBtnWrp::SOME_ASSGN : AssgnBtnWrp::ALL_ASSGN));
    //cout << nAsgnCount << " assgn, " << cSize(m_sSelOrigVal) << " total, " << listSel.size() << "listSel" << endl;
    m_bSomeSel = AssgnBtnWrp::SOME_ASSGN == eRes;

//cout << "<<< updateAssigned()\n"; printContainer(m_sSelOrigVal, cout, "\n");
    return eRes;
}


void TagWriter::eraseFields(const std::vector<std::pair<int, int> >& vFields)
{
    for (int i = 0, n = cSize(vFields); i < n; ++i)
    {
        int nSong (vFields[i].first);
        int nField (vFields[i].second);
        try
        {
            m_vpMp3HandlerTagData[nSong]->setData(nField, "");
        }
        catch (const Mp3HandlerTagData::InvalidValue&)
        { // nothing
        }
    }
}


// artist and album for the current song; empty if they don't exist
void TagWriter::getAlbumInfo(std::string& strArtist, std::string& strAlbum)
{
    if (m_vpMp3HandlerTagData.empty())
    {
        strArtist.clear(); strAlbum.clear(); return;
    }

    /*int nCrt (m_pCommonData->m_pCurrentAlbumG->currentIndex().row());
    const Mp3HandlerTagData& d (*m_vpMp3HandlerTagData[nCrt]);*/

/*for (int i = 0; i < 9; ++i)
{
    cout << d.getData(i) << endl;
}*/
    /*strArtist = d.getData2(TagReader::ARTIST);
    strAlbum = d.getData2(TagReader::ALBUM);*/
    //s = convStr(d.getData(TagReader::FEATURE_ON_POS[j - 1]));

    CB_ASSERT (m_nCurrentFile >= 0);
    //const Mp3HandlerTagData& d (*m_vpMp3HandlerTagData[m_nCurrentFile]);
    strArtist = getData(m_nCurrentFile, TagReader::ARTIST);
    strAlbum = getData(m_nCurrentFile, TagReader::ALBUM);
}


void TagWriter::onAssignImage(int nPos)
{
    char a [10];
    sprintf(a, g_szImageFmt, nPos + 1);
    set<int> snSelSongs;
    for (set<OrigValue>::iterator it = m_sSelOrigVal.begin(), end = m_sSelOrigVal.end(); it != end; ++it)
    {
        const OrigValue& o (*it);
        if (o.m_nField == (int)TagReader::IMAGE)
        {
            snSelSongs.insert(o.m_nSong);
        }
    }

    for (int i = 0; i < cSize(m_vpMp3HandlerTagData); ++i)
    {
        Mp3HandlerTagData* p (m_vpMp3HandlerTagData[i]);
        if (Mp3HandlerTagData::ASSIGNED != p->getStatus(TagReader::IMAGE) || m_nCurrentFile == i || snSelSongs.count(i) > 0)
        {
            p->setData(TagReader::IMAGE, a);
        }
    }
    m_snUnassignedImages.erase(nPos);

    //m_pCommonData->m_pCurrentAlbumG->repaint();
    reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);
}

void TagWriter::onEraseFile(int nPos)
{
    m_nFileToErase = nPos;
    QTimer::singleShot(1, this, SLOT(onEraseFileDelayed()));
}


void TagWriter::onEraseFileDelayed()
{
    const TagWrtImageInfo& inf (m_imageColl[m_nFileToErase]);
    CB_ASSERT (!inf.m_sstrFiles.empty());
    QString s;
    if (inf.m_sstrFiles.size() > 1)
    {
        s = "these files?";
        for (set<string>::const_iterator it = inf.m_sstrFiles.begin(); it != inf.m_sstrFiles.end(); ++it)
        {
            s += "\n" + convStr(toNativeSeparators(*it));
        }
    }
    else
    {
        s = "\"" + convStr(toNativeSeparators(*inf.m_sstrFiles.begin())) + "\"";
    }

    if (0 != showMessage(m_pParentWnd, QMessageBox::Question, 1, 1, "Confirm", "Do you want to erase " + s, "Erase", "Cancel")) { return; }

    bool bAssigned, bNonId3V2;
    hasUnsaved(bAssigned, bNonId3V2);

    if (bAssigned)
    {
        if (0 != showMessage(m_pParentWnd, QMessageBox::Critical, 1, 1, "Error", "You cannot erase image files if there are unsaved values. Do you want to save?", "Save, then erase file", "Cancel")) { return; }

        emit requestSave();

        hasUnsaved(bAssigned, bNonId3V2);
        if (bAssigned) { return; }
    }

    if (!m_vAlbumInfo.empty() || !m_vstrPastedValues.empty())
    {
        if (0 != showMessage(m_pParentWnd, QMessageBox::Critical, 1, 1, "Warning", "Erasing image files triggers a full reload, which results in downloaded and pasted data being lost. Erase anyway?", "Erase", "Cancel")) { return; }
    }

    for (set<string>::const_iterator it = inf.m_sstrFiles.begin(); it != inf.m_sstrFiles.end(); ++it)
    {
        if (!QFile(convStr(*it)).remove())
        {
            QMessageBox::critical(m_pParentWnd, "Error", QString("Couldn't erase file \"%1\"").arg(toNativeSeparators(convStr(*it))));
        }
    }

    reloadAll(getCurrentName(), CLEAR_DATA, CLEAR_ASSGN);
}


// should be called when the user clicks on the assign button; changes status of selected cells and returns the new state of m_pAssignedB
AssgnBtnWrp::State TagWriter::toggleAssigned(AssgnBtnWrp::State eCrtState)
{
    if (m_sSelOrigVal.empty()) { return eCrtState; } // !!! covers the case when only fields in the "file name" column are selected
//cout << " >>> toggleAssigned()\n"; printContainer(m_sSelOrigVal, cout, "\n");

    AssgnBtnWrp::State eRes;

    //AssgnBtnWrp::State eState (m_pCommonData->m_assgnBtnWrp.getState());
    switch (eCrtState)
    {
    case AssgnBtnWrp::ALL_ASSGN: // switch to "NONE"
        for (set<OrigValue>::iterator it = m_sSelOrigVal.begin(), end = m_sSelOrigVal.end(); it != end; ++it)
        {
            const OrigValue& val (*it);
            //setData(val.m_nSong, val.m_nField, val
            setStatus(val.m_nSong, val.m_nField, Mp3HandlerTagData::NON_ID3V2_VAL);
        }
        eRes = AssgnBtnWrp::NONE_ASSGN;
        break;

    case AssgnBtnWrp::NONE_ASSGN: // switch to "SOME" if it exists; otherwise switch to "ALL"
        if (m_bSomeSel)
        {
            for (set<OrigValue>::iterator it = m_sSelOrigVal.begin(), end = m_sSelOrigVal.end(); it != end; ++it)
            {
                const OrigValue& val (*it);
                setData(val.m_nSong, val.m_nField, val.m_strVal);
                setStatus(val.m_nSong, val.m_nField, val.m_eStatus);
            }
            eRes = AssgnBtnWrp::SOME_ASSGN;
            break;
        }

        // break; !!! don't "break"

    case AssgnBtnWrp::SOME_ASSGN: // switch to "ALL"
        for (set<OrigValue>::iterator it = m_sSelOrigVal.begin(), end = m_sSelOrigVal.end(); it != end; ++it)
        {
            const OrigValue& val (*it);
            setData(val.m_nSong, val.m_nField, val.m_strVal);
            setStatus(val.m_nSong, val.m_nField, Mp3HandlerTagData::ASSIGNED);
        }
        eRes = AssgnBtnWrp::ALL_ASSGN;
        break;

    default:
        CB_ASSERT(false);
    }

    reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);
//cout << " <<< toggleAssigned()\n"; printContainer(m_sSelOrigVal, cout, "\n");
    return eRes;
}


void TagWriter::copyFirst()
{
    if (m_sSelOrigVal.empty()) { return; }

    set<int> sDupCols;
    for (set<OrigValue>::iterator it = m_sSelOrigVal.begin(), end = m_sSelOrigVal.end(); it != end; ++it)
    {
        sDupCols.insert(it->m_nField);
    }

    sDupCols.erase(TagReader::TITLE);
    sDupCols.erase(TagReader::RATING);
    sDupCols.erase(TagReader::TRACK_NUMBER);

    for (set<int>::iterator it = sDupCols.begin(), end = sDupCols.end(); it != end; ++it)
    {
        int nField (*it);
        string s (getData(0, nField));
        for (int i = 1, n = cSize(m_vpMp3HandlerTagData); i < n; ++i) // !!! i starts at 1;
        {
            setData(i, nField, s);
        }
    }

    reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);
}


bool TagWriter::addImgFromFile(const QString& qs, bool bConsiderAssigned)
{
    QFile f (qs);
    bool bRes (false);

    if (f.open(QIODevice::ReadOnly))
    {
        CursorOverrider crsOv;

        int nSize ((int)f.size());
        QByteArray comprImg (f.read(nSize));
        QPixmap pic;

        if (pic.loadFromData(comprImg))
        {
            bRes = true;

            ImageInfo::Compr eCompr;
            int nWidth, nHeight;

            if (nSize <= ImageInfo::MAX_IMAGE_SIZE)
            {
                nWidth = pic.width(); nHeight = pic.height();
                eCompr = qs.endsWith(".png", Qt::CaseInsensitive) ? ImageInfo::PNG : ImageInfo::JPG; //ttt2 add more cases if supporting more image types
            }
            else
            {
                QPixmap scaledImg;
                ImageInfo::compress(pic, scaledImg, comprImg);
                nWidth = scaledImg.width(); nHeight = scaledImg.height();
                eCompr = ImageInfo::JPG;
            }

            ImageInfo img (-1, ImageInfo::OK, eCompr, comprImg, nWidth, nHeight);
            //int nSize (m_imageColl.size());
            int nPrevSize (m_imageColl.size());
            int nPos (m_imageColl.addImage(img, convStr(qs)));
            CB_ASSERT (-1 != nPos);

            if (nPrevSize != m_imageColl.size())
            {
                CB_ASSERT (m_imageColl.size() == nPos + 1);
                if (!bConsiderAssigned)
                {
                    m_snUnassignedImages.insert(nPos);
                }
            }

            /*if (nSize == m_imageColl.size()) // !!! it seemed to make sense to warn the user about duplicates; however, this is also triggered when entering a directory that has contains images after one of those images is assigned to a file;
            {
                CursorOverrider crs (Qt::ArrowCursor);
                QMessageBox::warning(m_pParentWnd, "Warning", QString("Image in file \"%1\" was already added to the image collection, so it won't be added a second time").arg(qs));
            }*/
        }
    }
    return bRes;
}


void TagWriter::paste()
{
    CursorOverrider crs;

    QClipboard* pClp (QApplication::clipboard());
    QPixmap pic (pClp->pixmap()); //ttt2 this leads in many cases to recompression; see how to avoid it; (probably ask the clipboard for other formats)
    if (!pic.isNull())
    {
        ImageInfo img (-1, ImageInfo::OK, pic);
        //ttt2 only reason to change image widget is because images were loaded, so perhaps adjust signals; (keep in mind first time, though)
        //emit imagesChanged();
        addImage(img, CONSIDER_UNASSIGNED);
        return;
    }

    //ttt2 text file name, including "http://", "ftp://", "file://" (see KIO::NetAccess::download() at http://developer.kde.org/documentation/books/kde-2.0-development/ch07lev1sec5.html )
    QString qs (pClp->text());

    if (qs.isEmpty())
    {
        const QMimeData* p (pClp->mimeData());
        QList<QUrl> lst (p->urls());
        bool bWarned (false);

        for (int i = 0; i < lst.size(); ++i)
        {
            QString qs1 (lst[i].toString());
            //qDebug("#%s#", qs1.toUtf8().data());
            if (!qs1.isEmpty())
            {
                if (qs.isEmpty())
                {
                    qs = qs1;
                }
                else
                {
                    if (!bWarned)
                    {
                        bWarned = true;
                        QMessageBox::warning(m_pParentWnd, "Warning", "Currently pasting multiple file names is not supported, so only the first one is considered.");
                    }
                }
            }
            else
            {
            }
        }

        if (lst.size() > 0)
        {
            qs = lst.front().toString();
        }
    }

    if (!qs.isEmpty())
    {
        if (-1 == qs.indexOf('\n'))
        {
            if (qs.startsWith("file://"))
            {
                qs.remove(0, 7);
            }

#ifndef WIN32
#else
            // when copying paths from explorer we get something like "file:///E:/Multimedia/Images/img1.jpg". After removing first 7 chars we are left with "/E:/Multimedia/Images/img1.jpg", so one more has to be removed
            if (qs.size() > 8 && qs[0] == getPathSep() && qs[1].isLetter() && qs[2] == ':' && qs[3] == getPathSep())
            {
                qs.remove(0, 1);
            }
#endif

#ifndef WIN32
            if (qs.startsWith(getPathSep())) // ttt2 see if it makes sense to open files without full name
#else
            qs = fromNativeSeparators(qs);
            //qDebug("qs=%s", qs.toUtf8().data());
            if (qs.size() > 7 && qs[0].isLetter() && qs[1] == ':' && qs[2] == getPathSep())
#endif
            {
                if (addImgFromFile(qs, CONSIDER_UNASSIGNED))
                {
                    emit imagesChanged();
                    return;
                }
            }


            {
                int nOk (0), nFail (0);
                for (set<OrigValue>::iterator it = m_sSelOrigVal.begin(); it != m_sSelOrigVal.end(); ++it)
                {
                    const OrigValue& val (*it);
                    //qDebug ("song %d, fld %d, val %s", val.m_nSong, val.m_nField, val.m_strVal.c_str());
                    if (TagReader::IMAGE != val.m_nField) // IMAGE is read-only from grid editing's point of view
                    {
                        Mp3HandlerTagData& dest (*m_vpMp3HandlerTagData[val.m_nSong]);
                        try
                        {
                            dest.setData(val.m_nField, convStr(qs));
                            dest.setStatus(val.m_nField, Mp3HandlerTagData::ASSIGNED);
                            ++nOk;
                        }
                        catch (const Mp3HandlerTagData::InvalidValue&)
                        {
                            ++nFail;
                        }
                    }
                }

                if (nFail > 0)
                {
                    QMessageBox::critical(m_pParentWnd, "Error", nOk > 0 ? "The pasted value couldn't be assigned to some fields" : "The pasted value couldn't be assigned to any fields"); //ttt2 some/one/several ...
                }

                reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);

                return;
            }
        }
        else
        {
            QStringList lst (qs.split("\n", QString::SkipEmptyParts));
            if (lst.size() != cSize(m_vpMp3HandlerTagData))
            {
                if (0 != showMessage(m_pParentWnd, QMessageBox::Warning, 1, 1, "Warning", "The number of lines in the clipboard is different from the number of files. Paste anyway?", "Paste", "Cancel")) { return; }
            }

            if (m_bNonStandardTrackNo && m_pCommonData->m_bWarnPastingToNonSeqTracks)
            {
                if (0 != showMessage(m_pParentWnd, QMessageBox::Warning, 1, 1, "Warning", "The track numbers aren't consecutive numbers starting at 1, so the pasted track information might not match the tracks. Paste anyway?", "Paste", "Cancel")) { return; }
            }

            sort();

            m_vstrPastedValues.clear();

            //int n (min(lst.size(), cSize(m_vpMp3HandlerTagData)));
            int n (lst.size());
            for (int i = 0; i < n; ++i)
            {
                //m_vpMp3HandlerTagData[i]->m_strPastedVal = string(lst[m_vnMovedTo[i]].toUtf8());
                //m_vstrPastedValues.push_back(string(lst[m_vnMovedTo[i]].toUtf8()));
                m_vstrPastedValues.push_back(convStr(lst[i]));
                //cout << "paste " << m_vstrPastedValues[i] << endl;
            }
            /*m_pCurrentAlbumModel->emitLayoutChanged();
            m_pCurrentFileModel->emitLayoutChanged();*/
            reloadAll("", DONT_CLEAR_DATA, DONT_CLEAR_ASSGN);

            return;
        }
    }

    QMessageBox::critical(m_pParentWnd, "Error", "Unrecognized clipboard content");
}


void TagWriter::sort()
{
    sortSongs();
    emit albumChanged(/*false*/);
}


int TagWriter::addImage(const ImageInfo& img, bool bConsiderAssigned) // returns the index of the image; if it already exists it's not added again; if it's invalid returns -1
{
    int nPrevSize (m_imageColl.size());
    int nPos (m_imageColl.addImage(img));
    if (nPrevSize != m_imageColl.size())
    {
        CB_ASSERT (m_imageColl.size() == nPos + 1);
        if (!bConsiderAssigned)
        {
            m_snUnassignedImages.insert(nPos);
        }
    }
    emit imagesChanged();
    return nPos;
}


void TagWriter::selectImg(int n)
{
    m_imageColl.select(n);
}

void TagWriter::addImgWidget(ImageInfoPanelWdgImpl* p)
{
    CB_ASSERT (0 != p);
    m_imageColl.addWidget(p);
}


void TagWriter::hasUnsaved(int nSong, bool& bAssigned, bool& bNonId3V2) // sets bAssigned and bNonId3V2 if at least one field has the corresponding status;
{
    bAssigned = false;
    bNonId3V2 = false;
    const Mp3HandlerTagData* p (m_vpMp3HandlerTagData.at(nSong));
    for (int i = 0; i < TagReader::LIST_END; ++i)
    {
        switch (p->getStatus(i))
        {
        case Mp3HandlerTagData::NON_ID3V2_VAL: bNonId3V2 = true; break;
        case Mp3HandlerTagData::ASSIGNED: bAssigned = true; break;
        default:; // nothing
        }
    }
}


void TagWriter::hasUnsaved(bool& bAssigned, bool& bNonId3V2) // sets bAssigned and bNonId3V2 if at least one field in at least a song has the corresponding status;
{
    bAssigned = false;
    bNonId3V2 = false;

    bool bAssignedSong, bNonId3V2Song;
    for (int i = 0; i < cSize(m_vpMp3HandlerTagData); ++i)
    {
        hasUnsaved(i, bAssignedSong, bNonId3V2Song);
        bAssigned |= bAssignedSong;
        bNonId3V2 |= bNonId3V2Song;
    }

}



// sets m_eState and changes the button icon accordingly
void AssgnBtnWrp::setState(State eState)
{
    static QPixmap picAll (":/images/assgn-all.svg");
    static QPixmap picSome (":/images/assgn-some.svg");
    static QPixmap picNone (":/images/assgn-none.svg");
    m_eState = eState;
    //const char* szCap (ALL_ASSGN == eState ? "A" : (SOME_ASSGN == eState ? "S" : "N")); m_pButton->setText(szCap);
    const QPixmap& pic (ALL_ASSGN == eState ? picAll : (SOME_ASSGN == eState ? picSome : picNone));
    m_pButton->setIcon(pic);
}


