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


#ifndef DiscogsDownloaderH
#define DiscogsDownloaderH

#include  "AlbumInfoDownloaderDlgImpl.h"

namespace Discogs
{
    struct SearchXmlHandler;

    struct DiscogsAlbumInfo : public WebAlbumInfoBase
    {
        enum StyleOption { GENRE_ONLY, GENRE_COMMA_STYLE, GENRE_PAR_STYLE, STYLE_ONLY };
        const StyleOption* m_peStyleOption; // should be a reference, but that would make assignment fail

        DiscogsAlbumInfo(const StyleOption* peStyleOption) : m_peStyleOption(peStyleOption) {}

        std::string m_strComposer;
        std::string m_strGenre;
        std::string m_strStyle;
        std::string m_strNotes;

        std::string m_strId;

        /*override*/ void copyTo(AlbumInfo& dest);

        std::string getGenre() const; // combination of m_strGenre and m_strStyle
    };
};


class DiscogsDownloader : public AlbumInfoDownloaderDlgImpl
{
    Q_OBJECT

    std::vector<Discogs::DiscogsAlbumInfo> m_vAlbums;
    Discogs::DiscogsAlbumInfo::StyleOption m_eStyleOption;

    friend struct Discogs::SearchXmlHandler;

    void clear();

    /*override*/ bool initSearch(const std::string& strArtist, const std::string& strAlbum);
    /*override*/ std::string createQuery();

    /*override*/ void loadNextPage();
    /*override*/ void requestAlbum(int nAlbum);
    /*override*/ void requestImage(int nAlbum, int nImage);
    /*override*/ void reloadGui();

    /*override*/ QHttp* getWaitingHttp();

    /*override*/ WebAlbumInfoBase& album(int i);
    /*override*/ int getAlbumCount() const;

    /*override*/ QXmlDefaultHandler* getSearchXmlHandler();
    /*override*/ QXmlDefaultHandler* getAlbumXmlHandler(int nAlbum);

    /*override*/ const WebAlbumInfoBase* getCrtAlbum() const; // returns 0 if there's no album
    /*override*/ int getColumnCount() const { return 4; }

    /*override*/ void saveSize();
    /*override*/ char getReplacementChar() const { return '+'; }
protected:
    int m_nPageSize;
public:
    DiscogsDownloader(QWidget* pParent, SessionSettings& settings, bool bSaveResults);
    ~DiscogsDownloader();
    /*$PUBLIC_FUNCTIONS$*/

    static const char* SOURCE_NAME;

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pSearchB_clicked();
    void on_m_pStyleCbB_currentIndexChanged(int);

private:
};




#endif // #ifndef DiscogsDownloaderH

