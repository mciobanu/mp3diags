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


#ifndef AlbumInfoDownloaderDlgImplH
#define AlbumInfoDownloaderDlgImplH

#include  <string>
#include  <vector>
#include  <unordered_set>

#include  <QDialog>
#include  <QNetworkReply>
#include  <QNetworkAccessManager>

#include  "ui_AlbumInfoDownloader.h"

#include  "CommonTypes.h"


class QPixmap;
//class QNetworkAccessManager;

class SessionSettings;

struct WebAlbumInfoBase
{
    WebAlbumInfoBase() : m_eVarArtists(AlbumInfo::VA_NOT_SUPP) {}
    virtual ~WebAlbumInfoBase() {}

    std::string m_strTitle;
    std::string m_strArtist;
    std::string m_strReleased;
    std::vector<VolumeInfo> m_vVolumes;
    std::vector<TrackInfo*> m_vpTracks; // Just a flattened m_vVolumes, with no ownership for pointers. Helps with the grid and other things

    std::vector<std::string> m_vstrImageNames; // for Discogs - IDs; for MusicBrainz - full URLs

    std::string m_strFormat; // CD, tape, ...
    std::vector<ImageInfo*> m_vpImages; // doesn't own the pointers; initially it contains 0s; this isn't used by MainFormDlgImpl, only internally by AlbumInfoDownloaderDlgImpl, which passes to MainFormDlgImpl at most an image, in a separate parameter

    std::vector<std::string> m_vstrImageInfo; // size in pixels and bytes

    AlbumInfo::VarArtists m_eVarArtists;

    virtual void copyTo(AlbumInfo& dest) = 0;

    /**
     * Looks at the total number of tracks and at those in the first volume, in an attempt to be more useful
     */
    bool checkTrackCountMatch(int nExpected) const;

    int getTotalTrackCount() const;

    bool isMultiVolume() const { return m_vVolumes.size() > 1; }
};


//class QXmlDefaultHandler;

class JsonHandler
{
public:
    virtual ~JsonHandler() {}
    virtual bool handle(const QString& qstrJson) = 0;
    virtual QString getError() = 0;
};

class WebDwnldModel;


// at any time there's at most 1 HTTP request pending
//
//class AlbumInfoDownloaderDlgImpl : public QDialog, private Ui::AlbumInfoDownloaderDlg
class AlbumInfoDownloaderDlgImpl : public QDialog, protected Ui::AlbumInfoDownloaderDlg
{
    Q_OBJECT

    void next(); // meant to be called only by retryNavigation()
    void previous();

    bool m_bSaveResults;
    int m_nLastCount;
    time_t m_nLastTime;
    std::string getTempName(); // time-based, with no extension; doesn't check for existing names, but uses a counter, so files shouldn't get removed (except during daylight saving time changes)
    void saveDownloadedData(const char*, int nSize, const char* szExt);
    virtual char getReplacementChar() const = 0;

protected:
    bool m_bSaveImageOnly;

    // Normally there are equally-sized pages, but we don't care about that, only how far we got in the list, which is
    //   used to determine when to ask for the next entries and as a visual cue that there are more entries.
    // For Discogs, among the entries there are artists, which should be ignored.
    int m_nTotalEntryCnt;
    int m_nLastLoadedEntry;
    std::vector<ImageInfo*> m_vpImages; // owns the pointers

    int m_nLoadingAlbum, m_nLoadingImage;
    ImageInfo::Compr m_eLoadingImageCompr;

    // Without page number; to be used by loadNextPage();
    std::string m_strQuery;

    int m_nCrtAlbum, m_nCrtImage;
    //std::string m_strArtist;
    //std::string m_strAlbum;

    enum NavigDir { NONE, NEXT, PREV }; // navigation direction
    enum Waiting { NOTHING, IMAGE, SEARCH, ALBUM };

    NavigDir m_eNavigDir;
    Waiting m_eWaiting;
    void setWaiting(Waiting eWaiting);
    bool m_bNavigateByAlbum;

    virtual std::string createQuery() = 0;
    void search();

    virtual void loadNextPage() = 0;
    virtual void requestAlbum(int nAlbum) = 0;
    virtual void requestImage(int nAlbum, int nImage) = 0;
    virtual void reloadGui();

    void onSearchLoaded(const QString& qstrJson);
    void onAlbumLoaded(const QString& qstrJson);
    void onImageLoaded(const QByteArray& comprImg, int nWidth, int nHeight, const QString& qstrInfo); // pPixmap may not be 0
    void handleImageError();

    void retryNavigation();

    virtual bool initSearch(const std::string& strArtist, const std::string& strAlbum) = 0;

    static std::string removeParentheses(const std::string& s);

    void addNote(const QString& qstrNote);
    void setImageType(const std::string& strName);
    void resetNavigation(); // clears pending HTTP requests, m_eNavigDir and m_eWaiting; restores the cursor if needed;

    virtual WebAlbumInfoBase& album(int i) = 0;
    virtual int getAlbumCount() const = 0;

    virtual JsonHandler* getSearchJsonHandler() = 0;
    virtual JsonHandler* getAlbumJsonHandler(int nAlbum) = 0;

    void updateTrackList();

    virtual void saveSize() = 0;

    std::string replaceSymbols(std::string); // replaces everything besides letters and digits with getReplacementChar()

    WebDwnldModel* m_pModel;

    SessionSettings& m_settings;

protected:
    QNetworkAccessManager m_networkAccessManager;
    std::unordered_set<QNetworkReply*> m_spNetworkReplies;

    int m_nExpectedTracks;

    AlbumInfoDownloaderDlgImpl(QWidget* pParent, SessionSettings& settings, bool bSaveResults);

    static const char* NOT_FOUND_AT_AMAZON;
public:
    ~AlbumInfoDownloaderDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    // pAlbumInfo is non-0 upon exit iff the return is true and the user closed with "Save All";
    // pQPixmap is non-0 upon exit iff the return is true and there is a picture;
    // the pointers must be deallocated by the caller;
    // pAlbumInfo->m_imageInfo is not set; (the caller manually links the result returned in pImageInfo)
    // for Discogs: the other fields are set;
    // for MusicBrainz: pAlbumInfo->m_strComposer and pAlbumInfo->m_strNotes are not set; (they are not available)
    bool getInfo(const std::string& strArtist, const std::string& strAlbum, int nTrackCount, AlbumInfo*& pAlbumInfo, ImageInfo*& pImageInfo);

    virtual const WebAlbumInfoBase* getCrtAlbum() const = 0; // returns 0 if there's no album
    virtual int getColumnCount() const = 0; // really just for composer; 4 if the field exists and 4 if it doesn't;

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pPrevB_clicked();
    void on_m_pNextB_clicked();
    void on_m_pPrevAlbumB_clicked();
    void on_m_pNextAlbumB_clicked();

    void on_m_pSaveAllB_clicked();
    void on_m_pSaveImageB_clicked();
    void on_m_pCancelB_clicked();

    //void onDone(bool bError);
    void onRequestFinished(QNetworkReply*);

    void onHelp();

private:
};





void addIfMissing(std::string& strDest, const std::string& strSrc);

// splits a string based on a separator, putting the components in a vector; trims the substrings; discards empty components;
void split(const std::string& s, const std::string& sep, std::vector<std::string>& v); //ttt2 move to Helpers, improve interface ...

void addList(std::string& strDest, const std::string& strSrc);



class WebDwnldModel : public QAbstractTableModel
{
    Q_OBJECT

    AlbumInfoDownloaderDlgImpl& m_dwnld;
    QTableView& m_grid;
    //typedef Discogs::DiscogsAlbumInfo AlbumInfo;
public:
    WebDwnldModel(AlbumInfoDownloaderDlgImpl&, QTableView&);
    /*override*/ int rowCount(const QModelIndex&) const;
    /*override*/ int columnCount(const QModelIndex&) const;
    /*override*/ QVariant data(const QModelIndex&, int nRole) const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;

    void emitLayoutChanged() { emit layoutChanged(); }
};




#endif // #ifndef AlbumInfoDownloaderDlgImplH

