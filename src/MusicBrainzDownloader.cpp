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


#include  "QHttp"
#include  <QDesktopServices>

#ifndef WIN32
    #include  <sys/time.h>
#else
    #include  <windows.h>
#endif

#include  <QDateTime>
#include  <QJsonParseError>
#include  <QJsonDocument>
#include  <QJsonArray>
#include  <QJsonObject>
#include  <QJsonValue>

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
#if 0
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
#endif
};


class SearchJsonHandler : public JsonHandler
{
    MusicBrainzDownloader& m_dlg;
    QString m_qstrError;
public:
    explicit SearchJsonHandler(MusicBrainzDownloader& dlg) : m_dlg(dlg) {}

    bool handle(const QString& qstrJson1) override
    {
        const QString& qstrJson (qstrJson1 + ""); //ttt9: Cause error here and make use of getError() to show the user what's wrong
        m_qstrError.clear();

        QByteArray arr = qstrJson.toUtf8();

        QJsonParseError parseError {};
        QJsonObject jsonObj;
        //try
        {
            QJsonDocument jsonDoc;
            jsonDoc = QJsonDocument::fromJson(arr, &parseError);
            jsonObj = jsonDoc.object();
        }
        /*catch (const exception& ex) //!!! It looks like no exceptions are thrown and parseError is set instead
        {
            m_qstrError = MusicBrainzDownloader::tr("JSON parse error: %1").arg(ex.what());
            return false;
        }*/
        if (parseError.error != QJsonParseError::ParseError::NoError)
        {
            m_qstrError = MusicBrainzDownloader::tr("JSON parse error: %1").arg(parseError.errorString());
            return false;
        }

        //try
        {
            const QJsonArray& relArr = jsonObj.value("releases").toArray(); //!!!: If no entry is found, we get an empty array, which is fine
            for (const auto& rel : relArr)
            {
                m_dlg.m_vAlbums.emplace_back();
                const QJsonObject& relObj = rel.toObject();
                m_dlg.m_vAlbums.back().m_strId = convStr(relObj.value("id").toString());
                m_dlg.m_vAlbums.back().m_nTrackCount = relObj.value("track-count").toInt();
            }
            m_dlg.m_nTotalEntryCnt = jsonObj.value("count").toInt();
            m_dlg.m_nLastLoadedEntry = jsonObj.value("offset").toInt() + relArr.size() - 1;
        }
        /*catch (const exception& ex) //!!! Looks like no exception is thrown on missing fields,  //ttt9: So store the retrieved
        // fields in some variables and compare against QJsonValue::Undefined
        {
            m_qstrError = MusicBrainzDownloader::tr("JSON expected field not found: %1").arg(ex.what());
            return false;
        }*/

        return true;
    }

    QString getError() override
    {
        return m_qstrError;
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
#if 0
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
    {//copied
        CB_ASSERT (m_albumInfo.m_strId == convStr(attrs.value("id")));
    }

    void onAlbEventStart(const QXmlAttributes& attrs)
    {//copied
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
    {// not needed
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
    {// copied
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
                if (beginsWith(strUrl, "https://"))
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
    {// copied
        m_albumInfo.m_vpImages.resize(m_albumInfo.m_vstrImageNames.size());
        m_albumInfo.m_vstrImageInfo.resize(m_albumInfo.m_vstrImageNames.size());
        if (m_albumInfo.m_strAmazonLink.empty() && !m_albumInfo.m_strAsin.empty())
        {
            m_albumInfo.m_strAmazonLink = "https://www.amazon.com/gp/product/" + m_albumInfo.m_strAsin;
        }

        for (int i = 0, n = cSize(m_albumInfo.m_vTracks); i < n; ++i)
        {
            TrackInfo& t (m_albumInfo.m_vTracks[i]);
            addList(t.m_strArtist, m_albumInfo.m_strArtist);
        }
    }

    void onAlbTitleChar(const string& s)
    {// copied
        m_albumInfo.m_strTitle = s;
    }

    void onAsinChar(const string& s)
    {// copied
        m_albumInfo.m_strAsin = s;
        m_albumInfo.m_vstrImageNames.push_back("https://images.amazon.com/images/P/" + s + ".01.LZZZZZZZ.jpg"); // ttt2 "01" is country code for US, perhaps try others //ttt2 perhaps check for duplicates
    }

    void onAlbArtistNameChar(const string& s)
    {// copied
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
    {// copied
        m_albumInfo.m_vTracks.back().m_strTitle = s;
    }

    void onTrackArtistName(const string& s)
    {// copied
        m_albumInfo.m_vTracks.back().m_strArtist = s;
    }
#endif
};


// A pretty inclusive URL, with 7 areas of information: http://musicbrainz.org/ws/2/release/b4da1c5b-8d99-3289-b919-cb62fe412eff?fmt=json&inc=artist-credits+recordings+labels+recording-level-rels+work-rels+work-level-rels+artist-rels
//      - artist-credits
//      - recordings
//      - labels
//      - recording-level-rels
//      - work-rels
//      - work-level-rels
//      - artist-rels
//
// According to https://musicbrainz.org/doc/MusicBrainz_API, the additional info you can request for
//      an /ws/2/release consists of:
//      - artists
//      - collections
//      - labels
//      - recordings
//      - release-groups
//
// The only overlap is "labels", so these either mean something else, or they are obsolete, or there is
//      some mapping from the latter to the former
//
// Anyway, further down https://musicbrainz.org/doc/MusicBrainz_API talks about url-rels and other relationships, so the relevant information is there

// https://musicbrainz.org/doc/MusicBrainz_Database/Schema
// https://musicbrainz.org/doc/MusicBrainz_API
// https://musicbrainz.org/doc/MusicBrainz_API/Search
// https://musicbrainz.org/doc/Style/Relationships/URLs
// https://musicbrainz.org/doc/XML_Web_Service/Rate_Limiting
// https://musicbrainz.org/relationship/4f2e710d-166c-480c-a293-2e2c8d658d87

// http://musicbrainz.org/ws/2/release/b4da1c5b-8d99-3289-b919-cb62fe412eff?fmt=json&inc=artist-credits+recordings+url-rels


class AlbumJsonHandler : public JsonHandler
{
    MusicBrainzAlbumInfo& m_albumInfo;
    bool m_bTargetIsUrl;
    QString m_qstrError;
public:
    explicit AlbumJsonHandler(MusicBrainzAlbumInfo& albumInfo) : m_albumInfo(albumInfo), m_bTargetIsUrl(false) {}

    bool handle(const QString& qstrJson) override
    {
        m_qstrError.clear();

        QByteArray arr = qstrJson.toUtf8();

        QJsonParseError parseError {};
        QJsonObject jsonObj;
        //try
        {
            QJsonDocument jsonDoc;
            jsonDoc = QJsonDocument::fromJson(arr, &parseError);
            jsonObj = jsonDoc.object();
        }
        /*catch (const exception& ex)
        {
            m_qstrError = MusicBrainzDownloader::tr("JSON parse error: %1").arg(ex.what());
            return false;
        }*/
        if (parseError.error != QJsonParseError::ParseError::NoError)
        {
            m_qstrError = MusicBrainzDownloader::tr("JSON parse error: %1").arg(parseError.errorString());
            return false;
        }

        //try  //ttt9: Store fields in variables and compare them to QJsonValue::Undefined
        {
            const string& id = convStr(jsonObj.value("id").toString());
            CB_ASSERT (m_albumInfo.m_strId == id);

            {
                const QString& qstrAsin = jsonObj.value("asin").toString();
                if (!qstrAsin.isNull()) {
                    m_albumInfo.m_strAsin = convStr(qstrAsin);
                    m_albumInfo.m_vstrImageNames.push_back(
                            //"https://imagQQes.aQQmazon.com/images/P/" + m_albumInfo.m_strAsin +
                            "https://images.amazon.com/images/P/" + m_albumInfo.m_strAsin +
                            ".01.LZZZZZZZ.jpg"); // ttt2 "01" is country code for US, perhaps try others //ttt2 perhaps check for duplicates
                }
            }

            {
                const QString& qstrTitle = jsonObj.value("title").toString();
                if (!qstrTitle.isNull()) {
                    m_albumInfo.m_strTitle = convStr(qstrTitle);
                }
            }

            {
                const QJsonArray& artistsArr = jsonObj.value("artist-credit").toArray();
                artistsArr.size();
                if (!artistsArr.empty()) {
                    const QString& s = artistsArr[0].toObject().value("artist").toObject().value(
                            "name").toString(); //ttt9: Review [0] here and elsewhere, where we just look at the first entry
                    if (0 == s.compare("VaRiOuS Artists", Qt::CaseInsensitive)) {
                        m_albumInfo.m_eVarArtists = AlbumInfo::VA_VARIOUS;  //ttt9: test this
                    } else {
                        m_albumInfo.m_strArtist = convStr(s);
                    }
                }
            }

            {
                const QJsonArray& releasesArr = jsonObj.value("release-events").toArray();
                for (const auto& rel: releasesArr) {
                    const string& strDate = convStr(rel.toObject().value("date").toString());
                    if (!strDate.empty()) {
                        m_albumInfo.m_strReleased = m_albumInfo.m_strReleased.empty() ? strDate : min(
                                m_albumInfo.m_strReleased, strDate);
                    }
                }
            }

            {
                const QJsonArray& mediaArr = jsonObj.value("media").toArray();
                for (const auto& media: mediaArr) {
                    const QJsonObject& mediaObj = media.toObject();
                    const string& strFormat = convStr(mediaObj.value("format").toString());
                    if (!strFormat.empty() && string::npos == m_albumInfo.m_strFormat.find(strFormat)) {
                        addIfMissing(m_albumInfo.m_strFormat, strFormat);
                    }

                    // What we do here is add all tracks from a volume to an album, which isn't quite right, but not
                    // sure how this worked in the past. //ttt9 Check git log; this was handled for Discogs. Also, AlbumInfoDownloaderDlgImpl
                    // treats the subject. (Looks like the old interface didn't have multi-volume)
                    const QJsonArray& trackArr = mediaObj.value("tracks").toArray();
                    for (const auto& track: trackArr) {
                        TrackInfo inf;
                        const QJsonObject& trackObj = track.toObject();
                        inf.m_strTitle = convStr(trackObj.value("title").toString());
                        inf.m_strPos = convStr(trackObj.value(
                                "number").toString());  //ttt9: There's also "position". TBD if any is optional / which to use
                        const QJsonArray& artistCreditsArr = trackObj.value("artist-credit").toArray();
                        if (!artistCreditsArr.empty()) {
                            inf.m_strArtist = convStr(artistCreditsArr[0].toObject().value("name").toString());
                        }

                        m_albumInfo.m_vTracks.emplace_back(inf);
                    }
                }
            }

            {
                const QJsonArray& relationsArr = jsonObj.value("relations").toArray();
                for (const auto& rel: relationsArr) {

                    const QJsonObject& relObj = rel.toObject();
                    const string& tgtType = convStr(relObj.value("target-type").toString());
                    if (tgtType == "url")
                    {
                        const string& type = convStr(relObj.value("type").toString());
                        const string& url = convStr(relObj.value("url").toObject().value("resource").toString());
                        if (type == "amazon asin")
                        {
                            m_albumInfo.m_strAmazonLink = url;
                        }
                        else if (type == "cover art link") //ttt9: This is a guess based on how other fields changed from v1. Needs validation
                        {
                            if (beginsWith(url, "https://") || beginsWith(url, "http://"))
                            {
                                m_albumInfo.m_vstrImageNames.push_back(url);
                            }
                            else
                            { //ttt2 perhaps tell the user
                                qDebug("Unsupported image link: %s", url.c_str());
                            }
                        }
                    }
                }
            }

            {
                m_albumInfo.m_vpImages.resize(m_albumInfo.m_vstrImageNames.size());
                m_albumInfo.m_vstrImageInfo.resize(m_albumInfo.m_vstrImageNames.size());
                if (m_albumInfo.m_strAmazonLink.empty() && !m_albumInfo.m_strAsin.empty())
                {
                    m_albumInfo.m_strAmazonLink = "https://www.amazon.com/gp/product/" + m_albumInfo.m_strAsin; //ttt9: Not always right. Not all ASINs are US. OTOH if the dedicated link is filled in, we don't get here
                }
            }

            // Add artist info to each track, to the extent that it doesn't already have the album's artists
            for (int i = 0, n = cSize(m_albumInfo.m_vTracks); i < n; ++i)
            {
                TrackInfo& t (m_albumInfo.m_vTracks[i]);
                addList(t.m_strArtist, m_albumInfo.m_strArtist);
            }
        }
        /*catch (const exception& ex)
        {
            m_qstrError = MusicBrainzDownloader::tr("JSON expected field not found: %1").arg(ex.what());
            return false;
        }*/

        return true;
    }

    QString getError() override
    {
        return m_qstrError;
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

const char* HOST = "musicbrainz.org";

} // namespace MusicBrainz

//ttt9: See why MusicBrainzDownloader gets its own icon in task manager

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
    setWindowTitle(tr("Download album data from MusicBrainz.org"));

    int nWidth, nHeight;
    m_settings.loadMusicBrainzSettings(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 400) { resize(nWidth, nHeight); }

    m_pViewAtAmazonL->setText(getAmazonText());

    m_pGenreE->hide(); m_pGenreL->hide();
    m_pAlbumNotesM->hide();

    m_pVolumeL->hide(); m_pVolumeCbB->hide();

    m_pStyleL->hide(); m_pStyleCbB->hide();

    m_pImgSizeL->setMinimumHeight(m_pImgSizeL->height()*2);

    m_pModel = new WebDwnldModel(*this, *m_pTrackListG); // !!! in a way these would make sense to be in the base constructor, but that would cause calls to pure virtual methods
    m_pTrackListG->setModel(m_pModel); //ttt9 Make sure to call decreaseRowHeaderFont() and setHeaderColor(). Same for Discogs

    connect(m_pSearchB, SIGNAL(clicked()), this, SLOT(on_m_pSearchB_clicked()));

    connect(m_pViewAtAmazonL, SIGNAL(linkActivated(const QString&)), this, SLOT(onAmazonLinkActivated(const QString&)));
}


MusicBrainzDownloader::~MusicBrainzDownloader()
{
    resetNavigation();
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
    // https://musicbrainz.org/doc/MusicBrainz_API/Search - Uses Lucene. The relevant entry is near the end. To see details about available fields, search for "The Release index contains"

    vector<string> vstrParams;

    const QString& qstrArtist = m_pSrchArtistE->text().trimmed();
    if (!qstrArtist.isEmpty())
    {
        vstrParams.emplace_back("artist:" + replaceSymbols(convStr(qstrArtist)));
    }
    const QString& qstrAlbum = m_pSrchAlbumE->text().trimmed();
    if (!qstrAlbum.isEmpty())
    {
        vstrParams.emplace_back("release:" + replaceSymbols(convStr(qstrAlbum)));
    }
    //string s ("/ws/2/release/?query=artist:" + replaceSymbols(convStr(m_pSrchArtistE->text())) + " AND release=" + replaceSymbols(convStr(m_pSrchAlbumE->text())));
    // https://musicbrainz.org/ws/1/release/?type=xml&artist=Keane&title=Hopes%20and%20Fears
    // https://musicbrainz.org/ws/2/release/?query=release:Hopes%20and%20fears%20AND%20artist:Keane
    // https://musicbrainz.org/ws/2/release/?query=release:Hopes%20and%20fears%20AND%20artist:Keane&fmt=json
    // https://musicbrainz.org/ws/2/release/?query=release:Follow%20Your%20Own%20Heart%20AND%20artist:Maria%20Sangiolo&fmt=json&limit=3


    //ttt0: See why these have different results, given that just the order in an AND differs:
    // https://musicbrainz.org/ws/2/release/?query=artist:Maria Sangiolo AND release:Follow Your Own Heart&fmt=json&limit=3&offset=0
    // https://musicbrainz.org/ws/2/release/?query=release:Follow Your Own Heart AND artist:Maria Sangiolo&fmt=json&limit=3&offset=0

    if (vstrParams.empty())
    {
        return "";
    }
    if (m_pMatchCountCkB->isChecked())
    {
        vstrParams.emplace_back(convStr(QString("tracks:%1").arg(m_nExpectedTracks)));
    }
    /*for (string::size_type i = 0; i < s.size(); ++i)
    {
        if (' ' == s[i])
        {
            s[i] = '+';
        }
    }*/
    //s = "/ws/1/release/?type=xml&artist=Beatles&title=Help";
    const string& strQry = "/ws/2/release/?query=" + join(vstrParams, " AND ");
    return strQry;
}


void MusicBrainzDownloader::delay()
{
    long long t (getTime());
    long long nDiff (t - m_nLastReqTime);
    //qDebug("crt: %lld, prev: %lld, diff: %lld", t, m_nLastReqTime, nDiff);
    if (nDiff < 1000)
    {
        if (nDiff < 0) { nDiff = 0; }
        int nWait (999 - (int)nDiff);
        //qDebug("   wait: %d", nWait);
        addNote(tr("waiting %1ms").arg(nWait + 100));

        //ttt1 perhaps use PausableThread::usleep()
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
        addNote(tr("QQQ done waiting %1ms").arg(nWait + 100));
        //qDebug("waiting %d", nWait);
    }
    else
    {
        addNote(tr("QQQ won't wait: crt: %1, prev: %2, diff: %3").arg(t).arg(m_nLastReqTime).arg(nDiff));
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

namespace
{

void setUserAgent(QNetworkRequest& request)
{
    request.setRawHeader("User-Agent", "Mp3Diags/1.5 ( https://mp3diags.sourceforge.net/ )");
    //header.setValue("User-Agent", "Firefox 52");
}

void setAcceptGzip(QNetworkRequest& request)
{
    request.setRawHeader("Accept-Encoding", "gzip");
}

QString buildUrl(const string& strPathAndQuery)
{
    return QString("http://") + HOST + convStr(strPathAndQuery); // no good reason to use https
}
}



void MusicBrainzDownloader::loadNextPage()
{
LAST_STEP("MusicBrainzDownloader::loadNextPage");
    CB_ASSERT (m_spNetworkReplies.empty()); //ttt9: Review if this is really correct

    CB_ASSERT (m_nLastLoadedEntry < m_nTotalEntryCnt - 1);

    //m_eState = NEXT;
    setWaiting(SEARCH);
    //char a [20];
    //sprintf(a, "&page=%d", m_nLastLoadedPage + 1);
    //string s (m_strQuery + a);

    const int LIMIT = 20;
    QUrl url (buildUrl(m_strQuery) + QString("&fmt=json&limit=%1&offset=%2").arg(LIMIT).arg(m_nLastLoadedEntry + 1));

    QNetworkRequest req (url);
    setUserAgent(req);
    setAcceptGzip(req);

    delay();
    //qDebug("--------------\npath %s", header.path().toUtf8().constData());
    //qDebug("qry %s", m_strQuery.c_str());

    QNetworkReply* pReply = m_networkAccessManager.get(req);
    m_spNetworkReplies.insert(pReply);
    qDebug("%d, MusicBrainzDownloader::loadNextPage: added request and waiting reply for %p - %s", __LINE__, pReply, qPrintable(url.toString()));
    //2023.12.30-13:00 qDebug("%d, %s, MusicBrainzDownloader::loadNextPage: added request and waiting reply for %p - %s", __LINE__, getCurrentThreadInfo().c_str(), pReply, qPrintable(url.toString()));

    //cout << "sent search " << m_pQHttp->request(header) << " for page " << (m_nLastLoadedPage + 1) << endl;
    addNote("QQQ req " + url.toString()); //ttt9: Comment out QQQ addNote()
}


//ttt2 see if it is possible for a track to have its own genre

QString MusicBrainzDownloader::getAmazonText() const
{
LAST_STEP("MusicBrainzDownloader::getAmazonText");
    if (m_nCrtAlbum < 0 || m_nCrtAlbum >= cSize(m_vAlbums))
    {
        return AlbumInfoDownloaderDlgImpl::tr(NOT_FOUND_AT_AMAZON);
    }

    const MusicBrainzAlbumInfo& album (m_vAlbums[m_nCrtAlbum]);
    if (album.m_strAmazonLink.empty())
    {
        return AlbumInfoDownloaderDlgImpl::tr(NOT_FOUND_AT_AMAZON);
    }
    else
    {
        const string& s = album.m_strAmazonLink;
        string domain = "amazon.com";
        unsigned long k = s.find("amazon.");
        bool err = false;
        if (k != string::npos)
        {
            unsigned long h = s.find('/', k);
            if (h != string::npos)
            {
                domain = s.substr(k, h - k);
            }
            else
            {
                err = true;
            }
        }
        else
        {
            err = true;
        }
        if (err)
        {
            qDebug("Error processing Amazon URL %s", s.c_str());
            //ttt1: Do more
        }

        return tr("<a href=\"%1\">view at %2</a>").arg(s.c_str(), domain.c_str());
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
    //CB_ASSERT (!m_pQHttp->hasPendingRequests() && !m_pImageQHttp->hasPendingRequests());  // ttt1 triggered: https://sourceforge.net/p/mp3diags/tickets/36/ ?? perhaps might happen when MB returns errors ; see also DiscogsDownloader::requestAlbum
    CB_ASSERT (m_spNetworkReplies.empty());

    m_nLoadingAlbum = nAlbum;
    setWaiting(ALBUM);
    //string s ("/release/" + m_vAlbums[nAlbum].m_strId + "?f=xml&api_key=f51e9c8f6c");
    string s ("/ws/2/release/" + m_vAlbums[nAlbum].m_strId + "?fmt=json&inc=artist-credits+recordings+url-rels");

    QUrl url (buildUrl(s));
    QNetworkRequest req (url);
    setUserAgent(req);
    setAcceptGzip(req);

    delay();
    QNetworkReply* pReply = m_networkAccessManager.get(req);
    m_spNetworkReplies.insert(pReply);
    qDebug("%d, MusicBrainzDownloader::requestAlbum: added request and waiting reply for %p - %s", __LINE__, pReply, qPrintable(url.toString()));
    //qDebug("%d, %s, MusicBrainzDownloader::requestAlbum: added request and waiting reply for %p - %s", __LINE__, getCurrentThreadInfo().c_str(), pReply, qPrintable(url.toString()));
    //cout << "sent album " << m_vAlbums[nAlbum].m_strId << " - " << m_pQHttp->request(header) << endl;
    addNote(AlbumInfoDownloaderDlgImpl::tr("getting album info ..."));
    addNote("QQQ req " + convStr(s));
}



void MusicBrainzDownloader::requestImage(int nAlbum, int nImage)
{
LAST_STEP("MusicBrainzDownloader::requestImage");
    CB_ASSERT (m_spNetworkReplies.empty());

    m_nLoadingAlbum = nAlbum;
    m_nLoadingImage = nImage;
    setWaiting(IMAGE);
    const string& strUrl (m_vAlbums[nAlbum].m_vstrImageNames[nImage]);
    setImageType(strUrl);

    QUrl url (convStr(strUrl));
    QNetworkRequest req (url);
    //setUserAgent(req);
    //req.setRawHeader("User-Agent", "Mp3Diags/1.5 ( https://mp3diags.sourceforge.net/ )");
    req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64; rv:120.0) Gecko/20100101 Firefox/120.0"); //ttt9: review. The thing is, some sites (e.g. Amazon) won't accept an unknown user agent
    req.setTransferTimeout(5 * 1000); // 5 seconds //ttt0: try to get rid of this. It only exists because for unknown hosts the request usually doesn't return immediately, but gets stuck until it times out

    delay(); // probably not needed, because doesn't seem that MusicBrainz would want to store images
    //connect(m_pImageQHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(onRequestFinished(int, bool)));
    //qDebug("host: %s, path: %s", url.host().toLatin1().constData(), url.path().toLatin1().constData());
    //qDebug("%s", strUrl.c_str());
    QNetworkReply* pReply = m_networkAccessManager.get(req);
    //QNetworkReply::NetworkError error = pReply->error();
    //const string& errStr = pReply->errorString().toStdString();
    m_spNetworkReplies.insert(pReply);
    // With a breakpoint here (or several lines above) and an incorrect URL, the request more often than not used to finish with a "Host imagqqes.aqqmazon.com not found", rather than hanging until timing out. Threading didn't seem to be the issue, as everything happens on the UI thread. However, after making some changes that shouldn't matter and reverting them on 2023.12.30-13:00, this is no longer the case. Other breakpoints maybe mattered as well, but they were restored too. Trying to reproduce the issue in a standalone program (TestNetworkAccessManager) led to "mostly timeout" behavior, but once it returned immediately with "host not found". The workaround was to add setTransferTimeout().  //ttt9: Try to understand what's going on, in particular try TestNetworkAccessManager with Qt6
    qDebug("%d, MusicBrainzDownloader::requestImage: added request and waiting reply for %p - %s", __LINE__, pReply, qPrintable(url.toString()));
    // qDebug("%d, %s, MusicBrainzDownloader::requestImage: added request and waiting reply for %p - %s", __LINE__, getCurrentThreadInfo().c_str(), pReply, qPrintable(url.toString()));

    addNote(AlbumInfoDownloaderDlgImpl::tr("getting image ..."));
    addNote("getting image " + convStr(strUrl));
}


//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================


/*override*/ WebAlbumInfoBase& MusicBrainzDownloader::album(int i)
{
    return m_vAlbums.at(i);
}

/*override*/ int MusicBrainzDownloader::getAlbumCount() const
{
    return cSize(m_vAlbums);
}


/*override*/ JsonHandler* MusicBrainzDownloader::getSearchJsonHandler()
{
    return new SearchJsonHandler(*this);
}

/*override*/ JsonHandler* MusicBrainzDownloader::getAlbumJsonHandler(int nAlbum)
{
    return new AlbumJsonHandler(m_vAlbums.at(nAlbum));
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


