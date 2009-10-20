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


#include  <map>

#include  <QHttp>

#include  "DiscogsDownloader.h"

#include  "Helpers.h"
#include  "SimpleSaxHandler.h"
#include  "StoredSettings.h"

////#include  <iostream> //ttt remove

using namespace std;
using namespace pearl;

using namespace Discogs;


ostream& operator<<(ostream& out, const DiscogsAlbumInfo& inf)
{
    out << "id: \"" << inf.m_strId << "\", artist: \"" << inf.m_strArtist << "\", title: \"" << inf.m_strTitle << "\", composer: \"" << inf.m_strComposer << "\", format: \"" << inf.m_strFormat << "\", genre: \"" << inf.m_strGenre << "\", style: \"" << inf.m_strStyle << "\", released: \"" << inf.m_strReleased << "\"\n\nnotes: " << inf.m_strNotes << "\n\nimages:\n";

    for (int i = 0, n = cSize(inf.m_vstrImageNames); i < n; ++i)
    {
        out << inf.m_vstrImageNames[i] << endl;
    }

    out << "\ntracks:" << endl;
    for (int i = 0, n = cSize(inf.m_vTracks); i < n; ++i)
    {
        out << "pos: \"" << inf.m_vTracks[i].m_strPos << "\", artist: \"" << inf.m_vTracks[i].m_strArtist << "\", title: \"" << inf.m_vTracks[i].m_strTitle << "\", composer: \"" << inf.m_vTracks[i].m_strComposer << "\"" << endl;
    }

    return out;
}



namespace Discogs
{

/*
    resp
        exactresults
            result
                uri
        searchresults
            result
                uri
*/
struct SearchXmlHandler : public SimpleSaxHandler<SearchXmlHandler>
{
    SearchXmlHandler(DiscogsDownloader& dlg) : SimpleSaxHandler<SearchXmlHandler>("resp"), m_dlg(dlg)
    {
        Node& resp (getRoot());
            Node& exactResults (makeNode(resp, "exactresults"));
                Node& excResult (makeNode(exactResults, "result")); excResult.onStart = &SearchXmlHandler::onResultStart;
                    Node& excUri (makeNode(excResult, "uri")); excUri.onChar = &SearchXmlHandler::onUriChar;
            Node& searchResults (makeNode(resp, "searchresults")); searchResults.onStart = &SearchXmlHandler::onSearchResultsStart;
                Node& srchResult (makeNode(searchResults, "result")); srchResult.onStart = &SearchXmlHandler::onResultStart;
                    Node& srchUri (makeNode(srchResult, "uri")); srchUri.onChar = &SearchXmlHandler::onUriChar;
    }


private:
    DiscogsDownloader& m_dlg;
    bool m_bIsRelease;

    void onSearchResultsStart(const QXmlAttributes& attrs)
    {
        int nStart (attrs.value("start").toInt());
        int nEnd (attrs.value("end").toInt());
        int nCount (attrs.value("numResults").toInt());
        CB_ASSERT (0 == nEnd % 20 || nEnd == nCount); //ttt2 see if this needs to be more flexible
        int nPageCnt ((nCount + 19) / 20);
        int nCrtPage ((nStart - 1)/20);
        if (0 == nPageCnt) { nCrtPage = -1; } // !!! needed for consistency
        m_dlg.m_nTotalPages = nPageCnt;
        m_dlg.m_nLastLoadedPage = nCrtPage;
        //cout << "total pages: " << nPageCnt << ", crt: " << nCrtPage << endl;
    }

    void onResultStart(const QXmlAttributes& attrs)
    {
        m_bIsRelease = "release" == attrs.value("type");
        if (m_bIsRelease)
        {
            m_dlg.m_vAlbums.push_back(DiscogsAlbumInfo(&m_dlg.m_eStyleOption));
        }
    }

    void onUriChar(const string& s)
    {
        if (m_bIsRelease)
        {
            string::size_type n (s.rfind("/"));
            CB_ASSERT (string::npos != n);
            //string s1 (convStr(qstrCh.right(qstrCh.size() - n - 1)));
            string s1 (s.substr(n + 1));
            m_dlg.m_vAlbums.back().m_strId = s1;
            //cout << s << endl;
        }
    }
};




/*
    resp
        release
            images
                image
            artists
                artist
                    name
            title
            extraartists
                artist
                    name
                    role
                    tracks
            formats
                format
            genres
                genre
            styles
                style
            released
            notes
            tracklist
                track
                    position
                    artists
                        artist
                            name
                    title
                    extraartists
                        artist
                            name
                            role
*/
struct AlbumXmlHandler : public SimpleSaxHandler<AlbumXmlHandler>
{
    AlbumXmlHandler(DiscogsAlbumInfo& albumInfo) : SimpleSaxHandler<AlbumXmlHandler>("resp"), m_albumInfo(albumInfo)
    {
        Node& resp (getRoot()); resp.onEnd = &AlbumXmlHandler::onRespEnd;
            Node& rel (makeNode(resp, "release")); rel.onStart = &AlbumXmlHandler::onReleaseStart;
                Node& images (makeNode(rel, "images"));
                    Node& image (makeNode(images, "image")); image.onStart = &AlbumXmlHandler::onImageStart;
                Node& albArtists (makeNode(rel, "artists"));
                    Node& albArtist (makeNode(albArtists, "artist"));
                        Node& albArtistName (makeNode(albArtist, "name")); albArtistName.onChar = &AlbumXmlHandler::onAlbArtistNameChar;
                Node& albTitle (makeNode(rel, "title")); albTitle.onChar = &AlbumXmlHandler::onAlbTitleChar;
                Node& albExtraArtists (makeNode(rel, "extraartists"));
                    Node& albExtraArtist (makeNode(albExtraArtists, "artist")); albExtraArtist.onStart = &AlbumXmlHandler::onAlbExtraArtistStart; albExtraArtist.onEnd = &AlbumXmlHandler::onAlbExtraArtistEnd;
                        Node& albExtraArtistName (makeNode(albExtraArtist, "name")); albExtraArtistName.onChar = &AlbumXmlHandler::onExtraArtistNameChar;
                        Node& albExtraArtistRole (makeNode(albExtraArtist, "role")); albExtraArtistRole.onChar = &AlbumXmlHandler::onExtraArtistRoleChar;
                        Node& albExtraArtistTracks (makeNode(albExtraArtist, "tracks")); albExtraArtistTracks.onChar = &AlbumXmlHandler::onExtraArtistTracksChar;
                Node& formats (makeNode(rel, "formats"));
                    Node& format (makeNode(formats, "format")); format.onStart = &AlbumXmlHandler::onFormatStart;
                Node& genres (makeNode(rel, "genres"));
                    Node& genre (makeNode(genres, "genre")); genre.onChar = &AlbumXmlHandler::onGenreChar;
                Node& styles (makeNode(rel, "styles"));
                    Node& style (makeNode(styles, "style")); style.onChar = &AlbumXmlHandler::onStyleChar;
                Node& released (makeNode(rel, "released")); released.onChar = &AlbumXmlHandler::onReleasedChar;
                Node& notes (makeNode(rel, "notes")); notes.onChar = &AlbumXmlHandler::onNotesChar;
                Node& tracklist (makeNode(rel, "tracklist"));
                    Node& track (makeNode(tracklist, "track")); track.onStart = &AlbumXmlHandler::onTrackStart;
                        Node& position (makeNode(track, "position")); position.onChar = &AlbumXmlHandler::onPositionChar;
                        Node& trkArtists (makeNode(track, "artists"));
                            Node& trkArtist (makeNode(trkArtists, "artist"));
                                Node& trkArtistName (makeNode(trkArtist, "name")); trkArtistName.onChar = &AlbumXmlHandler::onTrkArtistNameChar;
                        Node& trackTitle (makeNode(track, "title")); trackTitle.onChar = &AlbumXmlHandler::onTrackTitleChar;
                        Node& trkExtraArtists (makeNode(track, "extraartists"));
                            Node& trkExtraArtist (makeNode(trkExtraArtists, "artist")); trkExtraArtist.onEnd = &AlbumXmlHandler::onTrkExtraArtistEnd;
                                Node& trkExtraArtistName (makeNode(trkExtraArtist, "name")); trkExtraArtistName.onChar = &AlbumXmlHandler::onExtraArtistNameChar;
                                Node& trkExtraArtistRole (makeNode(trkExtraArtist, "role")); trkExtraArtistRole.onChar = &AlbumXmlHandler::onExtraArtistRoleChar;

    }

private:
    DiscogsAlbumInfo& m_albumInfo;

    map<string, string> m_mComposers; // track-composer
    string m_strExtraArtistName;
    string m_strExtraArtistRole;
    string m_strExtraArtistTracks;

    static bool isNumber (const string& s)
    {
        int n (cSize(s));
        if (0 == n) { return false; }
        for (int i = 0; i < n; ++i)
        {
            if (!isdigit(s[i])) { return false; }
        }
        return true;
    }

    // since roles of "Composed By" and "Written-By" were found, it seems that these don't follow a strict syntax; so probably non-letters are best ignored; case might be better ignored too;
    static string transfRole(const string& s)
    {
        string res;
        for (int i = 0, n = cSize(s); i < n; ++i)
        {
            char c (s[i]);
            if (isalnum(c))
            {
                res += tolower(c);
            }
        }
        return res;
    }

    static bool isComposerRole(string s)
    {
        //qDebug(" orig role %s", s.c_str());
        s = transfRole(s);
        //qDebug(" transf role %s", s.c_str());
        return "composedby" == s || "writtenby" == s || "musicby" == s || "lyricsby" == s; // ttt2 review whether all these should be added to "composer"; see if some others should be put here
    }


    void onReleaseStart(const QXmlAttributes& attrs)
    {
        CB_ASSERT (m_albumInfo.m_strId == convStr(attrs.value("id")));
        m_mComposers.clear();
    }

    void onImageStart(const QXmlAttributes& attrs)
    {
        string strUri (convStr(attrs.value("uri")));
        string::size_type n (strUri.rfind('/'));
        CB_ASSERT (string::npos != n);
        strUri.erase(0, n + 1);
        m_albumInfo.m_vstrImageNames.push_back(strUri);
        //cout << strUri << endl;
    }

    void onAlbExtraArtistStart(const QXmlAttributes& /*attrs*/)
    {
        m_strExtraArtistName.clear();
        m_strExtraArtistRole.clear();
        m_strExtraArtistTracks.clear();
    }

    void onFormatStart(const QXmlAttributes& attrs)
    {
        string strFmt (convStr(attrs.value("name")));
        //cout << strFmt << endl;
        addIfMissing(m_albumInfo.m_strFormat, strFmt);
    }

    void onTrackStart(const QXmlAttributes& /*attrs*/)
    {
        m_albumInfo.m_vTracks.push_back(TrackInfo());
    }

    // called by onRespEnd(); assigns composers to tracks based on the content of m_mComposers
    void onRespEnd()
    {
        for (int i = 0, n = cSize(m_albumInfo.m_vTracks); i < n; ++i)
        {
            TrackInfo& t (m_albumInfo.m_vTracks[i]);
            addList(t.m_strComposer, m_mComposers[t.m_strPos]);
            addList(t.m_strComposer, m_albumInfo.m_strComposer);
            addList(t.m_strArtist, m_albumInfo.m_strArtist);
        }

        m_albumInfo.m_vpImages.resize(m_albumInfo.m_vstrImageNames.size());
        m_albumInfo.m_vstrImageInfo.resize(m_albumInfo.m_vstrImageNames.size());
    }


    // takes data from m_strExtraArtistName, m_strExtraArtistRole and m_strExtraArtistTracks and either adds it to m_mComposers (if some tracks were specified) or to m_albumInfo.m_strComposer (if no tracks are given)
    void onAlbExtraArtistEnd()
    {
        if (isComposerRole(m_strExtraArtistRole))
        {
            vector<string> v;
            if (m_strExtraArtistTracks.empty())
            {
                addIfMissing(m_albumInfo.m_strComposer, m_strExtraArtistName);
            }
            else
            {
                split(m_strExtraArtistTracks, ",", v);

                for (int i = 0, n = cSize(v); i < n; ++i)
                {
                    string t (v[i]);
                    string::size_type p (t.find("to"));
                    if (string::npos != p)
                    {
                        addIfMissing(m_mComposers[t], m_strExtraArtistName);
                    }
                    else
                    {
                        string s1 (t.substr(0, p)); trim(s1);
                        string s2 (t.substr(p + 2)); trim(s2);

                        if (isNumber(s1) && isNumber(s2))
                        {
                            int n1 (atoi(s1.c_str()));
                            int n2 (atoi(s2.c_str()));
                            char a [10];
                            const char* szFmt ('0' == s1[0] ? "%02d" : "%d");
                            for (int i = n1; i <= n2; ++i)
                            {
                                sprintf(a, szFmt, i);
                                addIfMissing(m_mComposers[a], m_strExtraArtistName);
                            }
                        }
                        else
                        {
                            qDebug("track range not supported for non-numeric positions"); // ttt2 maybe support
                        }
                    }
                }
            }
        }
    }

    void onTrkExtraArtistEnd()
    {
        if (isComposerRole(m_strExtraArtistRole)) // ttt2 see if "Music By" should be compared too
        {
            addIfMissing(m_albumInfo.m_vTracks.back().m_strComposer, m_strExtraArtistName);
//inspect(m_strExtraArtistName);
        }
    }

    void onAlbArtistNameChar(const string& s)
    {
        addIfMissing(m_albumInfo.m_strArtist, s);
    }

    void onExtraArtistNameChar(const string& s)
    {
        m_strExtraArtistName = s;
        //addIfMissing(m_strExtraArtistName, s); // ttt2 review if this makes sense, given that for artists there is the option to join, but not sure about extraartists:
        /*
        <track>
            <position>5</position>
            <artists>
                <artist>
                    <name>Chloë Agnew</name>
                    <join>&</join>
                </artist>
                <artist>
                    <name>Lisa Kelly</name>
                    <join>&</join>
                </artist>
                <artist>
                    <name>Órla Fallon</name>
                    <join>&</join>
                </artist>
                <artist>
                    <name>Méav Ní Mhaolchatha</name>
                </artist>
            </artists>
            <title>One World</title>
            <duration>3:49</duration>
        </track>
        */
    }

    void onExtraArtistRoleChar(const string& s)
    {
        //addIfMissing(m_strExtraArtistRole, s);
        m_strExtraArtistRole = s;
    }

    void onExtraArtistTracksChar(const string& s)
    {
        //addIfMissing(m_albumInfo.m_strArtist, s);
        m_strExtraArtistTracks = s;
    }

    void onAlbTitleChar(const string& s)
    {
        m_albumInfo.m_strTitle = s;
    }

    void onGenreChar(const string& s)
    {
        addIfMissing(m_albumInfo.m_strGenre, s);
    }

    void onStyleChar(const string& s)
    {
        addIfMissing(m_albumInfo.m_strStyle, s);
    }

    void onReleasedChar(const string& s)
    {
        m_albumInfo.m_strReleased = s;
    }

    void onNotesChar(const string& s)
    {
        m_albumInfo.m_strNotes = s;
    }

    void onPositionChar(const string& s1)
    {
        string s (s1);
        //m_albumInfo.m_vTracks.back().m_nPos = qstrCh.toInt();
        if (endsWith(s, "."))
        {
            s.erase(s.size() - 1, 1);
        }
        m_albumInfo.m_vTracks.back().m_strPos = s;
    }

    void onTrkArtistNameChar(const string& s)
    {
        addIfMissing(m_albumInfo.m_vTracks.back().m_strArtist, s);
    }

    void onTrackTitleChar(const string& s)
    {
        m_albumInfo.m_vTracks.back().m_strTitle = s;
    }
};


/*override*/ void DiscogsAlbumInfo::copyTo(AlbumInfo& dest)
{
//cout << *this;
    dest.m_strTitle = m_strTitle;
    //dest.m_strArtist = m_strArtist;
    //dest.m_strComposer = m_strComposer;
    //dest.m_strFormat = m_strFormat; // CD, tape, ...
    dest.m_strGenre = getGenre();
    dest.m_strReleased = m_strReleased;
    dest.m_strNotes = m_strNotes;
    dest.m_vTracks = m_vTracks;
    //dest.m_eVarArtists  // !!! missing

    dest.m_strSourceName = DiscogsDownloader::SOURCE_NAME; // Discogs, MusicBrainz, ... ; needed by MainFormDlgImpl;
    //dest.m_imageInfo; // !!! not set
}


std::string DiscogsAlbumInfo::getGenre() const // combination of m_strGenre and m_strStyle
{
    switch (*m_peStyleOption)
    {
    case GENRE_ONLY: return m_strGenre;
    case GENRE_COMMA_STYLE: return m_strGenre.empty() || m_strStyle.empty() ? m_strGenre + m_strStyle : m_strGenre + ", " + m_strStyle;
    case GENRE_PAR_STYLE:
        {
            if (m_strStyle.empty()) { return m_strGenre; }
            if (m_strGenre.empty()) { return "(" + m_strStyle + ")"; } // ttt2 perhaps without "(/)"
            return m_strGenre + " (" + m_strStyle + ")";
        }
    case STYLE_ONLY: return m_strStyle;
    }

    CB_ASSERT (false);
}

}; // namespace Discogs


//=============================================================================================================================
//=============================================================================================================================
//=============================================================================================================================

/*override*/ void DiscogsDownloader::saveSize()
{
    m_settings.saveDiscogsSettings(width(), height(), m_pStyleCbB->currentIndex());
}



/*static*/ const char* DiscogsDownloader::SOURCE_NAME ("Discogs");


DiscogsDownloader::DiscogsDownloader(QWidget* pParent, SessionSettings& settings, bool bSaveResults) : AlbumInfoDownloaderDlgImpl(pParent, settings, bSaveResults)
{
    setWindowTitle("Download album data from Discogs.com");

    int nWidth, nHeight, nStyleOption;
    m_settings.loadDiscogsSettings(nWidth, nHeight, nStyleOption);
    if (nStyleOption < 0 || nStyleOption > Discogs::DiscogsAlbumInfo::STYLE_ONLY)
    {
        nStyleOption = Discogs::DiscogsAlbumInfo::GENRE_ONLY;
    }
    m_eStyleOption = Discogs::DiscogsAlbumInfo::StyleOption(nStyleOption);

    if (nWidth > 400 && nHeight > 400) { resize(nWidth, nHeight); }

    delete m_pSrchArtistL;
    delete m_pSrchAlbumL;
    delete m_pSrchAlbumE;
    delete m_pSpacer01L;
    //delete m_pViewAtAmazonE;
    delete m_pMatchCountCkB;
    m_pViewAtAmazonL->hide();

    {
        QStringList l;
        //l << "< don't use >" << "Genre1, Genre2, ... , Style1, Style2, ..." << "Genre1, Genre2, ... (Style1, Style2, ...)" << "Style1, Style2, ...";
        l << "Genres" << "Genres, Styles" << "Genres (Styles)" << "Styles";
        m_pStyleCbB->addItems(l);
        m_pStyleCbB->setCurrentIndex(nStyleOption);
    }

    m_pQHttp->setHost("www.discogs.com");

    m_pModel = new WebDwnldModel(*this, *m_pTrackListG); // !!! in a way these would make sense to be in the base constructor, but that would cause calls to pure virtual methods
    m_pTrackListG->setModel(m_pModel);

    connect(m_pSearchB, SIGNAL(clicked()), this, SLOT(on_m_pSearchB_clicked()));
    connect(m_pStyleCbB, SIGNAL(currentIndexChanged(int)), this, SLOT(on_m_pStyleCbB_currentIndexChanged(int)));
}


DiscogsDownloader::~DiscogsDownloader()
{
    resetNavigation(); // !!! not in base class, because it calls virtual method resetNavigation()
    clear();
}

void DiscogsDownloader::clear()
{
    clearPtrContainer(m_vpImages);
    m_vAlbums.clear();
}


/*override*/ bool DiscogsDownloader::initSearch(const std::string& strArtist, const std::string& strAlbum)
{
    string s (!strArtist.empty() && !strAlbum.empty() ? strArtist + " " + strAlbum : strArtist + strAlbum);
    s = removeParentheses(s);

    m_pSrchArtistE->setText(convStr(s));
    return !s.empty();
}




// "/search?type=all&q=Beatles+Abbey+Road&f=xml&api_key=f51e9c8f6c"
/*override*/ std::string DiscogsDownloader::createQuery()
{
    //string s (strArtist + "+" + strAlbum);
    string s ("/search?type=all&q=" + replaceSymbols(convStr(m_pSrchArtistE->text())) + "&f=xml&api_key=f51e9c8f6c");
    for (string::size_type i = 0; i < s.size(); ++i)
    {
        if (' ' == s[i])
        {
            s[i] = '+';
        }
    }
    return s;
}


//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================

void DiscogsDownloader::on_m_pSearchB_clicked()
{
    clear();
    search();
}



//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================


void DiscogsDownloader::loadNextPage()
{
    CB_ASSERT (!m_pQHttp->hasPendingRequests());
    ++m_nLastLoadedPage;
    CB_ASSERT (m_nLastLoadedPage <= m_nTotalPages - 1);

    //m_eState = NEXT;
    setWaiting(SEARCH);
    char a [20];
    sprintf(a, "&page=%d", m_nLastLoadedPage + 1);
    string s (m_strQuery + a);

    QHttpRequestHeader header ("GET", convStr(s));
    header.setValue("Host", "www.discogs.com");
    header.setValue("Accept-Encoding", "gzip");

    m_pQHttp->request(header);
    //cout << "sent search " << m_pQHttp->request(header) << " for page " << (m_nLastLoadedPage + 1) << endl;
}


void DiscogsDownloader::reloadGui()
{
    AlbumInfoDownloaderDlgImpl::reloadGui();

    const DiscogsAlbumInfo& album (m_vAlbums[m_nCrtAlbum]);
    m_pAlbumNotesM->setText(convStr(album.m_strNotes));
    m_pGenreE->setText(convStr(album.getGenre()));
}



void DiscogsDownloader::requestAlbum(int nAlbum)
{
    CB_ASSERT (!m_pQHttp->hasPendingRequests());
    m_nLoadingAlbum = nAlbum;
    setWaiting(ALBUM);
    string s ("/release/" + m_vAlbums[nAlbum].m_strId + "?f=xml&api_key=f51e9c8f6c");

    QHttpRequestHeader header ("GET", convStr(s));
    header.setValue("Host", "www.discogs.com");
    header.setValue("Accept-Encoding", "gzip");
    m_pQHttp->request(header);
    //cout << "sent album " << m_vAlbums[nAlbum].m_strId << " - " << m_pQHttp->request(header) << endl;
    addNote("getting album info ...");
}


void DiscogsDownloader::requestImage(int nAlbum, int nImage)
{
    CB_ASSERT (!m_pQHttp->hasPendingRequests());
    m_nLoadingAlbum = nAlbum;
    m_nLoadingImage = nImage;
    setWaiting(IMAGE);
    const string& strName (m_vAlbums[nAlbum].m_vstrImageNames[nImage]);
    setImageType(strName);

    string s ("/image/" + strName + "?api_key=f51e9c8f6c");
//cout << "  get img " << s << endl;

    QHttpRequestHeader header ("GET", convStr(s));
    header.setValue("Host", "www.discogs.com");
    //header.setValue("Accept-Encoding", "gzip");
    m_pQHttp->request(header);
    //cout << "sent img " <<  m_vAlbums[nAlbum].m_vstrImageNames[nImage] << " - " << m_pQHttp->request(header) << endl;
    addNote("getting image ...");
}


void DiscogsDownloader::on_m_pStyleCbB_currentIndexChanged(int k)
{
    if (m_nCrtAlbum < 0 || m_nCrtAlbum >= cSize(m_vAlbums)) { return; }
    DiscogsAlbumInfo& album (m_vAlbums[m_nCrtAlbum]);
    m_eStyleOption = Discogs::DiscogsAlbumInfo::StyleOption(k);
    m_pGenreE->setText(convStr(album.getGenre()));
}


//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================



/*override*/ QHttp* DiscogsDownloader::getWaitingHttp()
{
    return m_pQHttp;
}

/*override*/ WebAlbumInfoBase& DiscogsDownloader::album(int i)
{
    return m_vAlbums.at(i);
}

/*override*/ int DiscogsDownloader::getAlbumCount() const
{
    return cSize(m_vAlbums);
}

/*override*/ QXmlDefaultHandler* DiscogsDownloader::getSearchXmlHandler()
{
    return new SearchXmlHandler(*this);
}

/*override*/ QXmlDefaultHandler* DiscogsDownloader::getAlbumXmlHandler(int nAlbum)
{
    return new AlbumXmlHandler(m_vAlbums.at(nAlbum));
}


/*override*/ const WebAlbumInfoBase* DiscogsDownloader::getCrtAlbum() const // returns 0 if there's no album
{
    if (m_nCrtAlbum < 0 || m_nCrtAlbum >= cSize(m_vAlbums)) { return 0; }
    return &m_vAlbums[m_nCrtAlbum];
}




