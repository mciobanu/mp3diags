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


#ifndef CommonTypesH
#define CommonTypesH

#include  <string>
#include  <vector>

//#include  <QByteArray> // ttt2 see why this includes QByteRef, which gives warnings for lots of functions returning "const char" or "const bool", saying that "const" is going to be ignored; perhaps #define something before "//#include  <QByteArray>"; QtGui and QPixmap avoid this issue, but take longer
#include  <QPixmap>



//#include  <iosfwd>

// to avoid some dependencies and shorten the compilation time //ttt2 add more types here

struct ImageInfo
{
    enum Compr { INVALID, JPG, PNG };
    enum Status { OK, LOADED_NOT_COVER, USES_LINK, ERROR_LOADING, NO_PICTURE_FOUND };


    ImageInfo() : m_eCompr(INVALID), m_eStatus(NO_PICTURE_FOUND), m_nWidth(0), m_nHeight(0), m_nImageType(-1) {}
    //ImageInfo(Compr eCompr, QByteArray compressedImg) : m_eCompr(eCompr), m_eStatus(OK), m_compressedImg(compressedImg) {}
    ImageInfo(int nImageType, Status eStatus) : m_eCompr(INVALID), m_eStatus(eStatus), m_nWidth(0), m_nHeight(0), m_nImageType(nImageType) {}
    ImageInfo(int nImageType, Status eStatus, const QImage& pic);
    ImageInfo(int nImageType, Status eStatus, Compr eCompr, QByteArray compressedImg, int nWidth, int nHeight);

    bool isNull() const { return m_compressedImg.isEmpty(); }
    int getWidth() const { return m_nWidth; }
    int getHeight() const { return m_nHeight; }
    int getSize() const { return m_compressedImg.size(); }
    const char* getComprData() const { return m_compressedImg.constData(); }
    const char* getImageType() const;

    Status getStatus() const { return m_eStatus; }
    Compr getCompr() const { return m_eCompr; }

    // the picture is scaled down, keeping the aspect ratio, if the limits are exceeded; 0 and negative limits are ignored;
    // if nMaxWidth>0 and nMaxHeight<=0, nMaxHeight has the same value as nMaxWidth;
    QImage getImage(int nMaxWidth = -1, int nMaxHeight = -1) const;

    QString getTextDescr(const QString& qstrSep = "\n") const;
    void showFull(QWidget* pParent) const;

    //const QByteArray& getCompressedImg() const { return m_compressedImg; }
    bool operator==(const ImageInfo&) const;

    static int MAX_IMAGE_SIZE;
    static void compress(const QImage& origPic, QImage& scaledPic, QByteArray& comprImg); // scales down origPic and stores the pixmap in scaledPic, as well as a compressed version in comprImg; the algorithm coninues until comprImg becomes smaller than MAX_IMAGE_SIZE or until the width and the height of scaledPic get 150 or smaller; no scaling is done if comprImg turns out to be small enough for the original image;

    static const char* getImageType(int nImageType);
    static const char* getComprStr(Compr);

private:
    Compr m_eCompr;
    Status m_eStatus;
    int m_nWidth, m_nHeight;
    QByteArray m_compressedImg; // this normally contains a downloaded image, but it may contain a recompressed image, if it got scaled down
    int m_nImageType;
};



struct TrackInfo
{
    TrackInfo() : m_dRating(-1) {}
    std::string m_strTitle;
    std::string m_strArtist;
    std::string m_strPos;
    std::string m_strComposer;
    double m_dRating;
};


struct AlbumInfo
{
    std::string m_strTitle;
    //std::string m_strArtist; // in order to show the artist and composer in the grid whatever info is given at album level gets copied to each track, so there's no need for AlbumInfo to store them
    //std::string m_strComposer;
    //std::string m_strFormat; // CD, tape, ...
    std::string m_strGenre;
    std::string m_strReleased;
    std::string m_strNotes;
    enum VarArtists { VA_NOT_SUPP, VA_SINGLE, VA_VARIOUS };
    VarArtists m_eVarArtists;
    std::vector<TrackInfo> m_vTracks;

    std::string m_strSourceName; // Discogs, MusicBrainz, ... ; needed by MainFormDlgImpl;
    ImageInfo m_imageInfo; // a copy of an image in m_pCommonData->m_imageColl;

    AlbumInfo() : m_eVarArtists(VA_NOT_SUPP) {}
};

std::ostream& operator<<(std::ostream&, const AlbumInfo&);

enum TextCaseOptions { TC_NONE = -1, TC_LOWER = 0, TC_UPPER, TC_TITLE, TC_SENTENCE };

QString getCaseConv(const QString& s, TextCaseOptions eCase);

const char* getCaseAsStr(TextCaseOptions e);


struct ExternalToolInfo
{
    std::string m_strName;
    std::string m_strCommand;
    enum LaunchOption { DONT_WAIT, WAIT_THEN_CLOSE_WINDOW, WAIT_AND_KEEP_WINDOW_OPEN };
    LaunchOption m_eLaunchOption;
    bool m_bConfirmLaunch;

    ExternalToolInfo(const std::string& strName, const std::string& strCommand, LaunchOption eLaunchOption, bool bConfirmLaunch) : m_strName(strName), m_strCommand(strCommand), m_eLaunchOption(eLaunchOption), m_bConfirmLaunch(bConfirmLaunch) {}
    ExternalToolInfo(const std::string& strSerValue);
    std::string asString();
    static const char* launchOptionAsString(LaunchOption);

    struct InvalidExternalToolInfo {};
private:
    static char s_cSeparator;
};




#endif // #ifndef CommonTypesH

