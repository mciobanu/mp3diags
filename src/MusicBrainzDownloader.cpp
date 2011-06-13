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


#include  <QHttp>
#include  <QDesktopServices>

#ifndef WIN32
    #include  <sys/time.h>
#else
    #include  <windows.h>
#endif

#include  <QDateTime>

#include  "MusicBrainzDownloader.h"

#include  "Helpers.h"
#include  "SimpleSaxHandler.h"
#include  "StoredSettings.h"



using namespace std;
using namespace pearl;

#if 1
using namespace MusicBrainz;

namespace MusicBrainz
{

/*
    metadata
        release-list
            release
                track-list
*/
struct SearchXmlHandler : public SimpleSaxHandler<SearchXmlHandler>
{
    SearchXmlHandler(MusicBrainzDownloader& dlg) : SimpleSaxHandler<SearchXmlHandler>("metadata"), m_dlg(dlg)
    {
        Node& meta (getRoot()); meta.onStart = &SearchXmlHandler::onMetaStart;
            Node& relList (makeNode(meta, "release-list"));
                Node& rel (makeNode(relList, "release")); rel.onStart = &SearchXmlHandler::onRelStart;
                    Node& trackList (makeNode(rel, "track-list")); trackList.onStart = &SearchXmlHandler::onTrackListStart;
    }

private:
    MusicBrainzDownloader& m_dlg;

    void onMetaStart(const QXmlAttributes& /*attrs*/)
    {
        m_dlg.m_nLastLoadedPage = 0;
    }

    void onRelStart(const QXmlAttributes& attrs)
    {
        m_dlg.m_vAlbums.push_back(MusicBrainzAlbumInfo());
        m_dlg.m_vAlbums.back().m_strId = convStr(attrs.value("id"));
    }

    void onTrackListStart(const QXmlAttributes& attrs)
    {
        m_dlg.m_vAlbums.back().m_nTrackCount = attrs.value("count").toInt();
    }
};


/*
    metadata
        release
            title
            asin
            artist
                name
            release-event-list
                event
            track-list
                track
                    title
                    artist
                        name
            relation-list
                relation
*/
struct AlbumXmlHandler : public SimpleSaxHandler<AlbumXmlHandler>
{
    AlbumXmlHandler(MusicBrainzAlbumInfo& albumInfo) : SimpleSaxHandler<AlbumXmlHandler>("metadata"), m_albumInfo(albumInfo), m_bTargetIsUrl(false)
    {
        Node& meta (getRoot()); meta.onEnd = &AlbumXmlHandler::onMetaEnd;
            Node& rel (makeNode(meta, "release")); rel.onStart = &AlbumXmlHandler::onRelStart;
                Node& albTitle (makeNode(rel, "title")); albTitle.onChar = &AlbumXmlHandler::onAlbTitleChar;
                Node& asin (makeNode(rel, "asin")); asin.onChar = &AlbumXmlHandler::onAsinChar;
                Node& albArtist (makeNode(rel, "artist"));
                    Node& albArtistName (makeNode(albArtist, "name")); albArtistName.onChar = &AlbumXmlHandler::onAlbArtistNameChar;
                Node& albRelEvents (makeNode(rel, "release-event-list"));
                    Node& albEvent (makeNode(albRelEvents, "event")); albEvent.onStart = &AlbumXmlHandler::onAlbEventStart;
                Node& albTrackList (makeNode(rel, "track-list"));
                    Node& track (makeNode(albTrackList, "track")); track.onStart = &AlbumXmlHandler::onTrackStart;
                        Node& trackTitle (makeNode(track, "title")); trackTitle.onChar = &AlbumXmlHandler::onTrackTitleChar;
                        Node& trackArtist (makeNode(track, "artist"));
                            Node& trackArtistName (makeNode(trackArtist, "name")); trackArtistName.onChar = &AlbumXmlHandler::onTrackArtistName;
                Node& relationList (makeNode(rel, "relation-list")); relationList.onStart = &AlbumXmlHandler::onRelationListStart;
                    Node& relation (makeNode(relationList, "relation")); relation.onStart = &AlbumXmlHandler::onRelationStart;

        m_albumInfo.m_eVarArtists = AlbumInfo::VA_SINGLE;
    }

private:
    MusicBrainzAlbumInfo& m_albumInfo;
    bool m_bTargetIsUrl;

    void onRelStart(const QXmlAttributes& attrs)
    {
        CB_ASSERT (m_albumInfo.m_strId == convStr(attrs.value("id")));
    }

    void onAlbEventStart(const QXmlAttributes& attrs)
    {
        string strDate (convStr(attrs.value("date")));
        if (!strDate.empty())
        {
            m_albumInfo.m_strReleased = m_albumInfo.m_strReleased.empty() ? strDate : min(m_albumInfo.m_strReleased, strDate);
        }

        string strFormat (convStr(attrs.value("format")));
        if (!strFormat.empty() && string::npos == m_albumInfo.m_strFormat.find(strFormat))
        {
            addIfMissing(m_albumInfo.m_strFormat, strFormat);
        }
    }

    void onTrackStart(const QXmlAttributes&)
    {
        m_albumInfo.m_vTracks.push_back(TrackInfo());
        char a [10];
        sprintf(a, "%d", cSize(m_albumInfo.m_vTracks));
        m_albumInfo.m_vTracks.back().m_strPos = a;
    }

    void onRelationListStart(const QXmlAttributes& attrs)
    {
        m_bTargetIsUrl = "Url" == attrs.value("target-type");
    }

    void onRelationStart(const QXmlAttributes& attrs)
    {
        if (m_bTargetIsUrl)
        {
            QString qstrType (attrs.value("type"));
            if ("AmazonAsin" == qstrType)
            {
                m_albumInfo.m_strAmazonLink = convStr(attrs.value("target"));
            }
            else if ("CoverArtLink" == qstrType)
            {
                string strUrl (convStr(attrs.value("target")));
                if (beginsWith(strUrl, "http://"))
                {
                    m_albumInfo.m_vstrImageNames.push_back(strUrl);
                }
                else
                { //ttt2 perhaps tell the user
                    qDebug("Unsupported image link");
                }
            }
        }
    }


    void onMetaEnd()
    {
        m_albumInfo.m_vpImages.resize(m_albumInfo.m_vstrImageNames.size());
        m_albumInfo.m_vstrImageInfo.resize(m_albumInfo.m_vstrImageNames.size());
        if (m_albumInfo.m_strAmazonLink.empty() && !m_albumInfo.m_strAsin.empty())
        {
            m_albumInfo.m_strAmazonLink = "http://www.amazon.com/gp/product/" + m_albumInfo.m_strAsin;
        }

        for (int i = 0, n = cSize(m_albumInfo.m_vTracks); i < n; ++i)
        {
            TrackInfo& t (m_albumInfo.m_vTracks[i]);
            addList(t.m_strArtist, m_albumInfo.m_strArtist);
        }
    }

    void onAlbTitleChar(const string& s)
    {
        m_albumInfo.m_strTitle = s;
    }

    void onAsinChar(const string& s)
    {
        m_albumInfo.m_strAsin = s;
        m_albumInfo.m_vstrImageNames.push_back("http://images.amazon.com/images/P/" + s + ".01.LZZZZZZZ.jpg"); // ttt2 "01" is country code for US, perhaps try others //ttt2 perhaps check for duplicates
    }

    void onAlbArtistNameChar(const string& s)
    {
        if (0 == convStr(s).compare("VaRiOuS Artists", Qt::CaseInsensitive))
        {
            m_albumInfo.m_eVarArtists = AlbumInfo::VA_VARIOUS;
        }
        else
        {
            m_albumInfo.m_strArtist = s;
        }
    }

    void onTrackTitleChar(const string& s)
    {
        m_albumInfo.m_vTracks.back().m_strTitle = s;
    }

    void onTrackArtistName(const string& s)
    {
        m_albumInfo.m_vTracks.back().m_strArtist = s;
    }
};


/*override*/ void MusicBrainzAlbumInfo::copyTo(AlbumInfo& dest)
{

    dest.m_strTitle = m_strTitle;
    //dest.m_strArtist = m_strArtist;
    //dest.m_strComposer; // !!! missing
    //dest.m_strFormat = m_strFormat; // CD, tape, ...
    //dest.m_strGenre = m_strGenre; // !!! missing
    dest.m_strReleased = m_strReleased;
    //dest.m_strNotes; // !!! missing
    dest.m_vTracks = m_vTracks;
    dest.m_eVarArtists = m_eVarArtists;

    dest.m_strSourceName = MusicBrainzDownloader::SOURCE_NAME; // Discogs, MusicBrainz, ... ; needed by MainFormDlgImpl;
    //dest.m_imageInfo; // !!! not set
}


} // namespace MusicBrainz



//=============================================================================================================================
//=============================================================================================================================
//=============================================================================================================================


/*override*/ void MusicBrainzDownloader::saveSize()
{
    m_settings.saveMusicBrainzSettings(width(), height());
}


/*static*/ const char* MusicBrainzDownloader::SOURCE_NAME ("MusicBrainz");



MusicBrainzDownloader::MusicBrainzDownloader(QWidget* pParent, SessionSettings& settings, bool bSaveResults) : AlbumInfoDownloaderDlgImpl(pParent, settings, bSaveResults), m_nLastReqTime(0)
{
    setWindowTitle("Download album data from MusicBrainz.org");

    int nWidth, nHeight;
    m_settings.loadMusicBrainzSettings(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 400) { resize(nWidth, nHeight); }

    m_pViewAtAmazonL->setText(getAmazonText());

    m_pGenreE->hide(); m_pGenreL->hide();
    m_pAlbumNotesM->hide();

    m_pVolumeL->hide(); m_pVolumeCbB->hide();

    m_pStyleL->hide(); m_pStyleCbB->hide();

    m_pImgSizeL->setMinimumHeight(m_pImgSizeL->height()*2);

    m_pQHttp->setHost("musicbrainz.org");
    m_pImageQHttp = new QHttp (this);

    m_pModel = new WebDwnldModel(*this, *m_pTrackListG); // !!! in a way these would make sense to be in the base constructor, but that would cause calls to pure virtual methods
    m_pTrackListG->setModel(m_pModel);

    connect(m_pImageQHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(onRequestFinished(int, bool)));

    connect(m_pSearchB, SIGNAL(clicked()), this, SLOT(on_m_pSearchB_clicked()));

    connect(m_pViewAtAmazonL, SIGNAL(linkActivated(const QString&)), this, SLOT(onAmazonLinkActivated(const QString&)));
}


MusicBrainzDownloader::~MusicBrainzDownloader()
{
    resetNavigation(); // !!! not in base class, because it calls virtual method resetNavigation()
    clear();
}

void MusicBrainzDownloader::clear()
{
LAST_STEP("MusicBrainzDownloader::clear");
    clearPtrContainer(m_vpImages);
    m_vAlbums.clear();
}




/*override*/ bool MusicBrainzDownloader::initSearch(const std::string& strArtist, const std::string& strAlbum)
{
LAST_STEP("MusicBrainzDownloader::initSearch");
    m_pSrchArtistE->setText(convStr((removeParentheses(strArtist))));
    m_pSrchAlbumE->setText(convStr((removeParentheses(strAlbum))));
    return !strArtist.empty() || !strAlbum.empty();
}




/*override*/ std::string MusicBrainzDownloader::createQuery()
{
LAST_STEP("MusicBrainzDownloader::createQuery");
    string s ("/ws/1/release/?type=xml&artist=" + replaceSymbols(convStr(m_pSrchArtistE->text())) + "&title=" + replaceSymbols(convStr(m_pSrchAlbumE->text())));
    //qDebug("qry: %s", s.c_str());
    if (m_pMatchCountCkB->isChecked())
    {
        s += convStr(QString("&count=%1").arg(m_nExpectedTracks));
    }
    /*for (string::size_type i = 0; i < s.size(); ++i)
    {
        if (' ' == s[i])
        {
            s[i] = '+';
        }
    }*/
    //s = "/ws/1/release/?type=xml&artist=Beatles&title=Help";
    return s;
}


void MusicBrainzDownloader::delay()
{
    long long t (getTime());
    long long nDiff (t - m_nLastReqTime);
    //qDebug("crt: %lld, prev: %lld, diff: %lld", t, m_nLastReqTime, t - m_nLastReqTime);
    if (nDiff < 1000)
    {
        if (nDiff < 0) { nDiff = 0; }
        int nWait (999 - (int)nDiff);
        //qDebug("   wait: %d", nWait);
        char a [15];
        sprintf(a, "waiting %dms", nWait + 100);
        addNote(a);

#ifndef WIN32
        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 100000000; // 0.1s, to be sure
        nanosleep(&ts, 0);
        ts.tv_nsec = nWait*1000000;
        nanosleep(&ts, 0);
#else
        Sleep(nWait + 100);
#endif
        //qDebug("waiting %d", nWait);
    }

    m_nLastReqTime = t;
}


long long MusicBrainzDownloader::getTime() // time in milliseconds
{
    QDateTime t (QDateTime::currentDateTime());
    long long nRes (t.toTime_t()); //ttt3 32bit
    nRes *= 1000;
    nRes += t.time().msec();
    return nRes;
#if 0
    qDebug("t1 %lld", nRes);
#ifndef WIN32
    timeval tv;
    gettimeofday(&tv, 0);
qDebug("t2 %lld", tv.tv_sec*1000LL + tv.tv_usec/1000);
    return tv.tv_sec*1000LL + tv.tv_usec/1000;
#else
    QDateTime t (QDateTime::currentDateTime());
    return t.toTime_t(); //ttt3 32bit
#endif
#endif
}

//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================

void MusicBrainzDownloader::on_m_pSearchB_clicked()
{
LAST_STEP("MusicBrainzDownloader::on_m_pSearchB_clicked");
    clear();
    search();
}





//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================


void MusicBrainzDownloader::loadNextPage()
{
LAST_STEP("MusicBrainzDownloader::loadNextPage");
    CB_ASSERT (!m_pQHttp->hasPendingRequests());
    CB_ASSERT (!m_pImageQHttp->hasPendingRequests());

    ++m_nLastLoadedPage;
    CB_ASSERT (m_nLastLoadedPage <= m_nTotalPages - 1);

    //m_eState = NEXT;
    setWaiting(SEARCH);
    //char a [20];
    //sprintf(a, "&page=%d", m_nLastLoadedPage + 1);
    //string s (m_strQuery + a);

    QHttpRequestHeader header ("GET", convStr(m_strQuery));
    //header.setValue("Host", "www.musicbrainz.org");
    header.setValue("Host", "musicbrainz.org");
    //header.setValue("Accept-Encoding", "gzip");
    delay();
    //qDebug("--------------\npath %s", header.path().toUtf8().constData());
    //qDebug("qry %s", m_strQuery.c_str());
    m_pQHttp->request(header);
    //cout << "sent search " << m_pQHttp->request(header) << " for page " << (m_nLastLoadedPage + 1) << endl;
}


//ttt2 see if it is possible for a track to have its own genre

QString MusicBrainzDownloader::getAmazonText() const
{
LAST_STEP("MusicBrainzDownloader::getAmazonText");
    if (m_nCrtAlbum < 0 || m_nCrtAlbum >= cSize(m_vAlbums))
    {
        return NOT_FOUND_AT_AMAZON;
    }

    const MusicBrainzAlbumInfo& album (m_vAlbums[m_nCrtAlbum]);
    if (album.m_strAmazonLink.empty())
    {
        return NOT_FOUND_AT_AMAZON;
    }
    else
    {
        return convStr("<a href=\"" + album.m_strAmazonLink + "\">view at amazon.com</a>");
    }
}

void MusicBrainzDownloader::onAmazonLinkActivated(const QString& qstrLink) // !!! it's possible to set openExternalLinks on a QLabel to open links automatically, but this leaves an ugly frame around the text, with it right side missing; also, manual handling is needed to open a built-in browser;
{
LAST_STEP("MusicBrainzDownloader::onAmazonLinkActivated");
    m_pViewAtAmazonL->setText("qq"); // !!! the text needs to CHANGE to make the frame disappear
    m_pViewAtAmazonL->setText(getAmazonText());

    QDesktopServices::openUrl(qstrLink);
}



void MusicBrainzDownloader::reloadGui()
{
LAST_STEP("MusicBrainzDownloader::reloadGui");
    AlbumInfoDownloaderDlgImpl::reloadGui();
    m_pViewAtAmazonL->setText(getAmazonText());
}


void MusicBrainzDownloader::requestAlbum(int nAlbum)
{
LAST_STEP("MusicBrainzDownloader::requestAlbum");
    //CB_ASSERT (!m_pQHttp->hasPendingRequests() && !m_pImageQHttp->hasPendingRequests());  // ttt1 triggered: https://sourceforge.net/apps/mantisbt/mp3diags/view.php?id=36 ?? perhaps might happen when MB returns errors ; see also DiscogsDownloader::requestAlbum
    CB_ASSERT (!m_pQHttp->hasPendingRequests());
    CB_ASSERT (!m_pImageQHttp->hasPendingRequests());

    m_nLoadingAlbum = nAlbum;
    setWaiting(ALBUM);
    //string s ("/release/" + m_vAlbums[nAlbum].m_strId + "?f=xml&api_key=f51e9c8f6c");
    string s ("/ws/1/release/" + m_vAlbums[nAlbum].m_strId + "?type=xml&inc=tracks+artist+release-events+url-rels");

    QHttpRequestHeader header ("GET", convStr(s));
    header.setValue("Host", "musicbrainz.org");
    //header.setValue("Accept-Encoding", "gzip");
    delay();
    m_pQHttp->request(header);
    //cout << "sent album " << m_vAlbums[nAlbum].m_strId << " - " << m_pQHttp->request(header) << endl;
    addNote("getting album info ...");
}



void MusicBrainzDownloader::requestImage(int nAlbum, int nImage)
{
LAST_STEP("MusicBrainzDownloader::requestImage");
    CB_ASSERT (!m_pQHttp->hasPendingRequests());
    CB_ASSERT (!m_pImageQHttp->hasPendingRequests());

    m_nLoadingAlbum = nAlbum;
    m_nLoadingImage = nImage;
    setWaiting(IMAGE);
    const string& strUrl (m_vAlbums[nAlbum].m_vstrImageNames[nImage]);
    setImageType(strUrl);

    QUrl url (convStr(strUrl));
    m_pImageQHttp->setHost(url.host());

    delay(); // probably not needed, because doesn't seem that MusicBrainz would want to store images
    //connect(m_pImageQHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(onRequestFinished(int, bool)));
    //qDebug("host: %s, path: %s", url.host().toLatin1().constData(), url.path().toLatin1().constData());
    //qDebug("%s", strUrl.c_str());
    m_pImageQHttp->get(url.path());

    addNote("getting image ...");
}


//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================


/*override*/ QHttp* MusicBrainzDownloader::getWaitingHttp()
{
LAST_STEP("MusicBrainzDownloader::getWaitingHttp");
    return IMAGE == m_eWaiting ? m_pImageQHttp : m_pQHttp;
}

/*override*/ void MusicBrainzDownloader::resetNavigation()
{
LAST_STEP("MusicBrainzDownloader::resetNavigation");
    m_pImageQHttp->clearPendingRequests();
    AlbumInfoDownloaderDlgImpl::resetNavigation();
}


/*override*/ WebAlbumInfoBase& MusicBrainzDownloader::album(int i)
{
    return m_vAlbums.at(i);
}

/*override*/ int MusicBrainzDownloader::getAlbumCount() const
{
    return cSize(m_vAlbums);
}


/*override*/ QXmlDefaultHandler* MusicBrainzDownloader::getSearchXmlHandler()
{
    return new SearchXmlHandler(*this);
}

/*override*/ QXmlDefaultHandler* MusicBrainzDownloader::getAlbumXmlHandler(int nAlbum)
{
    //return new AlbumXmlHandler(m_vAlbums.at(nAlbum));
    return new AlbumXmlHandler(m_vAlbums.at(nAlbum));
}


/*override*/ const WebAlbumInfoBase* MusicBrainzDownloader::getCrtAlbum() const // returns 0 if there's no album
{
    if (m_nCrtAlbum < 0 || m_nCrtAlbum >= cSize(m_vAlbums)) { return 0; }
    return &m_vAlbums[m_nCrtAlbum];
}





/*


*/


//ttt2 perhaps look at Last.fm for more pictures (see Cover Fetcher for AmaroK 1.4; a brief look at the API seems to indicate that a generic "search" is not possible)


//ttt2 detect Qt 4.4 and use QWebView
#endif


