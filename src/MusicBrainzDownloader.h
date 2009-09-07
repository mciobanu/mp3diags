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


#ifndef MusicBrainzDownloaderH
#define MusicBrainzDownloaderH

#include  "AlbumInfoDownloaderDlgImpl.h"

namespace MusicBrainz
{
    struct SearchXmlHandler;

    struct MusicBrainzAlbumInfo : public WebAlbumInfoBase
    {
        MusicBrainzAlbumInfo() : m_nTrackCount(-1) {}

        //std::string m_strComposer; // !!! no Composer at MusicBrainz; see http://musicbrainz.org/doc/ClassicalMusicFAQ
        //std::string m_strGenre; // !!! no Genre at MusicBrainz: http://musicbrainz.org/doc/GeneralFAQ#head-6488c5c2fb7503b4ae26f3e234304e07c2b172fb
        //std::string m_strNotes; // !!! no Notes at MusicBrainz

        std::string m_strId;
        int m_nTrackCount;
        std::string m_strAsin;
        std::string m_strAmazonLink;

        /*override*/ void copyTo(AlbumInfo& dest);

    };
};


class MusicBrainzDownloader : public AlbumInfoDownloaderDlgImpl
{
    Q_OBJECT

    std::vector<MusicBrainz::MusicBrainzAlbumInfo> m_vAlbums;

    friend class MusicBrainz::SearchXmlHandler;

    void delay();
    long long getTime(); // time in milliseconds
    long long m_nLastReqTime;

    void clear();

    QHttp* m_pImageQHttp;

    /*override*/ bool initSearch(const std::string& strArtist, const std::string& strAlbum);
    /*override*/ std::string createQuery();

    /*override*/ void loadNextPage();
    /*override*/ void requestAlbum(int nAlbum);
    /*override*/ void requestImage(int nAlbum, int nImage);
    /*override*/ void reloadGui();

    /*override*/ QHttp* getWaitingHttp();
    /*override*/ void resetNavigation();

    /*override*/ WebAlbumInfoBase& album(int i);
    /*override*/ int getAlbumCount() const;

    /*override*/ QXmlDefaultHandler* getSearchXmlHandler();
    /*override*/ QXmlDefaultHandler* getAlbumXmlHandler(int nAlbum);

    /*override*/ const WebAlbumInfoBase* getCrtAlbum() const; // returns 0 if there's no album
    /*override*/ int getColumnCount() const { return 3; }

    /*override*/ void saveSize();
    /*override*/ char getReplacementChar() const { return ' '; }

    QString getAmazonText() const;
public:
    MusicBrainzDownloader(QWidget* pParent, SessionSettings& settings, bool bSaveResults);
    ~MusicBrainzDownloader();
    /*$PUBLIC_FUNCTIONS$*/

    static const char* SOURCE_NAME;

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/


    void on_m_pSearchB_clicked();
    void onAmazonLinkActivated(const QString&);


private:
};

#endif // #ifndef MusicBrainzDownloaderH

