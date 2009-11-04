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


#include  <memory>

#include  <zlib.h>

#ifndef WIN32
    #include  <sys/time.h>
#else
    #include  <ctime>
#endif

#include  <QHttp>
#include  <QMessageBox>
#include  <QXmlSimpleReader>
#include  <QBuffer>
#include  <QPainter>
#include  <QScrollBar>
#include  <QHeaderView>
#include  <QTime>

#include  "AlbumInfoDownloaderDlgImpl.h"

#include  "Helpers.h"
#include  "ColumnResizer.h"


#include  "fstream_unicode.h"

using namespace std;
using namespace pearl;

// /*extern*/ int CELL_WIDTH (22);
extern int CELL_HEIGHT;




AlbumInfoDownloaderDlgImpl::AlbumInfoDownloaderDlgImpl(QWidget* pParent, SessionSettings& settings, bool bSaveResults) : QDialog(pParent, getDialogWndFlags()), Ui::AlbumInfoDownloaderDlg(), m_bSaveResults(bSaveResults), m_nLastCount(0), m_nLastTime(0), m_settings(settings)
{
    setupUi(this);

    m_pQHttp = new QHttp (this);

    connect(m_pQHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(onRequestFinished(int, bool)));

    m_pTrackListG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
    m_pTrackListG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
}


AlbumInfoDownloaderDlgImpl::~AlbumInfoDownloaderDlgImpl()
{
}


bool AlbumInfoDownloaderDlgImpl::getInfo(const std::string& strArtist, const std::string& strAlbum, int nTrackCount, AlbumInfo*& pAlbumInfo, ImageInfo*& pImageInfo)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::getInfo");
    m_nExpectedTracks = nTrackCount;

    pAlbumInfo = 0; pImageInfo = 0;

    if (initSearch(strArtist, strAlbum))
    {
        search();
    }

    bool bRes (QDialog::Accepted == exec());
    if (bRes)
    {
        saveSize();
        CB_ASSERT (0 != getAlbumCount());
        if (!m_bSaveImageOnly)
        {
            pAlbumInfo = new AlbumInfo();
            album(m_nCrtAlbum).copyTo(*pAlbumInfo);
            //pAlbumInfo->m_strReleased = convStr(m_pRealeasedE->text()); //ttt2 maybe allow users to overwrite fields in edit boxes, esp. genre; however, most of the cases it's almost as easy to make any changes in the tag editor (well, there's an F2 and then "copy form first"); there are several issues in implementing this: 1) going to prev/next album; 2) artist name gets copied to each track; 3) consistency: if the edit boxes are editable why not the table? so, better without

            if (m_pVolumeCbB->isEnabled() && m_pVolumeCbB->currentIndex() != m_pVolumeCbB->count() - 1)
            {
                vector<TrackInfo> vTracks;
                string s (convStr(m_pVolumeCbB->itemText(m_pVolumeCbB->currentIndex())));
                int k (cSize(s));
                for (int i = 0, n = cSize(pAlbumInfo->m_vTracks); i < n; ++i)
                {
                    if (beginsWith(pAlbumInfo->m_vTracks[i].m_strPos, s))
                    {
                        vTracks.push_back(pAlbumInfo->m_vTracks[i]);
                        vTracks.back().m_strPos.erase(0, k);
                    }
                }
                vTracks.swap(pAlbumInfo->m_vTracks);
            }
        }

        if (m_nCrtImage >= 0)
        {
            pImageInfo = new ImageInfo(*album(m_nCrtAlbum).m_vpImages[m_nCrtImage]);
        }
    }
    return bRes;
}


// clears pending HTTP requests, m_eNavigDir and m_eWaiting; restores the cursor if needed;
/*virtual*/ void AlbumInfoDownloaderDlgImpl::resetNavigation()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::resetNavigation");
    m_pQHttp->clearPendingRequests();
    setWaiting(NOTHING);
    m_eNavigDir = NONE;
}



string AlbumInfoDownloaderDlgImpl::replaceSymbols(string s) // replaces everything besides letters and digits with getReplacementChar()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::replaceSymbols");
    char c (getReplacementChar());
    for (int i = 0; i < cSize(s); ++i)
    {
        if ((unsigned char)(s[i]) < 128 && '\'' != s[i] && !isalnum(s[i]))
        {
            s[i] = c;
        }
    }

    return s;
}

/*static*/ const char* AlbumInfoDownloaderDlgImpl::NOT_FOUND_AT_AMAZON = "not found at amazon.com";

void AlbumInfoDownloaderDlgImpl::search()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::search");
    m_pResArtistE->setText("");
    m_pResAlbumE->setText("");
    m_pDownloadsM->setText("");
    m_pAlbumNotesM->setText("");
    m_pFormatE->setText("");
    m_pRealeasedE->setText("");
    m_pResultNoL->setText("");
    updateTrackList();
    m_pVolumeCbB->clear();
    m_pImageL->setPixmap(0);
    m_pImageL->setText("");
    m_pImgSizeL->setText("\n");
    m_pViewAtAmazonL->setText(NOT_FOUND_AT_AMAZON);

    m_strQuery = escapeHttp(createQuery()); // e.g. http://www.discogs.com/search?type=all&q=beatles&f=xml&api_key=f51e9c8f6c, without page number; to be used by loadNextPage();
    m_nTotalPages = 1; m_nLastLoadedPage = -1;
    m_nCrtAlbum = -1; m_nCrtImage = -1;
    resetNavigation();
    //m_eNavigDir = NEXT;
    m_eNavigDir = NEXT;
    m_bNavigateByAlbum = false;
    addNote("searching ...");
    m_pGenreE->setText("");
    //next(); //loadNextPage();
    retryNavigation();
}


void AlbumInfoDownloaderDlgImpl::on_m_pPrevB_clicked()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::on_m_pPrevB_clicked");
    if (0 == getAlbumCount() || NOTHING != m_eWaiting) { return; }

    m_bNavigateByAlbum = false;
    m_eNavigDir = PREV;
    retryNavigation();
}


void AlbumInfoDownloaderDlgImpl::on_m_pNextB_clicked()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::on_m_pNextB_clicked");
    if (0 == getAlbumCount() || NOTHING != m_eWaiting) { return; }

    m_bNavigateByAlbum = false;
    m_eNavigDir = NEXT;
    retryNavigation();
}


void AlbumInfoDownloaderDlgImpl::on_m_pPrevAlbumB_clicked()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::on_m_pPrevAlbumB_clicked");
    if (0 == getAlbumCount() || NOTHING != m_eWaiting) { return; }

    m_bNavigateByAlbum = true;
    m_eNavigDir = PREV;
    retryNavigation();
}


void AlbumInfoDownloaderDlgImpl::on_m_pNextAlbumB_clicked()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::on_m_pNextAlbumB_clicked");
    if (0 == getAlbumCount() || NOTHING != m_eWaiting) { return; }

    m_bNavigateByAlbum = true;
    m_eNavigDir = NEXT;
    retryNavigation();
}


void AlbumInfoDownloaderDlgImpl::on_m_pSaveAllB_clicked()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::on_m_pSaveAllB_clicked");
    if (NOTHING != m_eWaiting)
    {
        QMessageBox::critical(this, "Error", "You cannot save the results now, because a request is still pending");
        return;
    }

    if (0 == getAlbumCount())
    {
        QMessageBox::critical(this, "Error", "You cannot save the results now, because no album is loaded");
        return;
    }

    int nCnt (0);
    CB_ASSERT (m_nCrtAlbum >= 0 && m_nCrtAlbum < getAlbumCount());
    const WebAlbumInfoBase& albumInfo (album(m_nCrtAlbum));
    if (m_pVolumeCbB->isEnabled() && m_pVolumeCbB->currentIndex() != m_pVolumeCbB->count() - 1)
    {
        string s (convStr(m_pVolumeCbB->itemText(m_pVolumeCbB->currentIndex())));
        for (int i = 0, n = cSize(albumInfo.m_vTracks); i < n; ++i)
        {
            if (beginsWith(albumInfo.m_vTracks[i].m_strPos, s))
            {
                ++nCnt;
            }
        }
    }
    else
    {
        nCnt = cSize(albumInfo.m_vTracks);
    }

    if (nCnt != m_nExpectedTracks)
    {
        QString s;
        const char* szVolMsg (
            m_pVolumeCbB->isEnabled() &&
            (
                (nCnt > m_nExpectedTracks && m_pVolumeCbB->currentIndex() == m_pVolumeCbB->count() - 1) ||
                (nCnt < m_nExpectedTracks && m_pVolumeCbB->currentIndex() != m_pVolumeCbB->count() - 1)
            )
            ? "You may want to use a different volume selection on this multi-volume release.\n\n" : "");

        if (nCnt > m_nExpectedTracks)
        {
            s = QString("A number of %1 tracks were expected, but your selection contains %2. Additional tracks will be discarded.\n\n%3Save anyway?").arg(m_nExpectedTracks).arg(nCnt).arg(szVolMsg);
        }
        else
        {
            s = QString("A number of %1 tracks were expected, but your selection only contains %2. Remaining tracks will get null values.\n\n%3Save anyway?").arg(m_nExpectedTracks).arg(nCnt).arg(szVolMsg);
        }
        QMessageBox::StandardButton eRes (QMessageBox::question(this, "Count inconsistency", s, QMessageBox::Cancel | QMessageBox::Save));
        if (QMessageBox::Save != eRes) { return; }
    }

    m_bSaveImageOnly = false;
    accept();
}

void AlbumInfoDownloaderDlgImpl::on_m_pSaveImageB_clicked()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::on_m_pSaveImageB_clicked");
    if (NOTHING != m_eWaiting)
    {
        QMessageBox::critical(this, "Error", "You cannot save the results now, because a request is still pending");
        return;
    }

    if (0 == getAlbumCount() || -1 == m_nCrtImage)
    {
        QMessageBox::critical(this, "Error", "You cannot save any image now, because there is no image loaded");
        return;
    }
    // ttt2 perhaps shouldn't save an "error" image

    m_bSaveImageOnly = true;
    accept();
}

void AlbumInfoDownloaderDlgImpl::on_m_pCancelB_clicked()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::on_m_pCancelB_clicked");
    reject();
}



//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================

// what may happen:
//   1) there is a "next" element and it is in memory; then the GUI is updated and that is all (this calls resetNavigation() );
//   2) there is a "next" element but it is not loaded; album info or picture are missing, or there's another page with search results that wasn't loaded yet; then a request is made for what's missing to be downloaded, and the state is updated to reflect that; when the request completes, retryNavigation() will get called, which will call again next()
//   3) there is no "next"; nothing happens in this case;
//
// if there's nothing to wait for, retryNavigation() calls resetNavigation() after next() exits; (retryNavigation() is the only function that is supposed to call next() );
//
// if no new HTTP request is sent, either setWaiting(NOTHING) or reloadGui() should be called, to leave m_eWaiting in a consistent state;
//
void AlbumInfoDownloaderDlgImpl::next()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::next");
    //if (NONE != m_eNavigDir) { return; }
    CB_ASSERT (NEXT == m_eNavigDir);

    int nNextAlbum (m_nCrtAlbum);
    int nNextImage (m_nCrtImage);

    for (;;)
    {
        if (nNextAlbum >= 0 && nNextImage < cSize(album(nNextAlbum).m_vstrImageNames) - 1 && (-1 == nNextImage || !m_bNavigateByAlbum))
        {
            ++nNextImage;
        }
        else if (nNextAlbum + 1 < getAlbumCount())
        { // m_vAlbums contains a "next" album, which may or may not be loaded
            if (album(nNextAlbum + 1).m_strTitle.empty())
            {
                requestAlbum(nNextAlbum + 1);
                return;
            }
            else
            {
                ++nNextAlbum;
                nNextImage = album(nNextAlbum).m_vstrImageNames.empty() ? -1 : 0;
            }
        }
        else
        { // nothing left in m_vAlbums
            if (m_nLastLoadedPage == m_nTotalPages - 1)
            { // !!! there's no "next" at all; just exit;
                setWaiting(NOTHING);
            }
            else
            {
                loadNextPage();
            }
            return;
        }

        // if it got here, we have a loaded album (pictures might be missing, though)

        if ((m_pImageFltCkB->isChecked() && album(nNextAlbum).m_vpImages.empty()) ||
            (m_pCdFltCkB->isChecked() && string::npos == album(nNextAlbum).m_strFormat.find("CD")) ||
            (m_pTrackCntFltCkB->isChecked() && m_nExpectedTracks != cSize(album(nNextAlbum).m_vTracks))) // ttt2 MusicBrainz could perform better here, because it knows how many tracks are in an album without actually loading it; ttt2 redo the whole next() / prev() thing in a more logical way; perhaps separate next() from nextAlbum();
        {
            continue;
        }

        if (-1 != nNextImage && 0 == album(nNextAlbum).m_vpImages[nNextImage])
        {
            requestImage(nNextAlbum, nNextImage);
            return;
        }

        m_nCrtAlbum = nNextAlbum;
        m_nCrtImage = nNextImage;
        reloadGui();
        return;
    }
}
//ttt2 perhaps filter by durations (but see first how it works without it)



// like next(), but here there's no need to load result pages; another difference is that when m_bNavigateByAlbum is set and we are at the first album and some other picture than the first, it goes to the first picture; in the similar case, next doesn't do anything (going to the last picture doesn't feel right)
void AlbumInfoDownloaderDlgImpl::previous()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::previous");
    CB_ASSERT (PREV == m_eNavigDir);
    //if (NONE != m_eNavigDir) { return; }

    int nPrevAlbum (m_nCrtAlbum);
    int nPrevImage (m_nCrtImage);

    for (;;)
    {
        if (nPrevImage > 0 && !m_bNavigateByAlbum)
        {
            --nPrevImage;
        }
        else if (nPrevAlbum > 0)
        { // m_vAlbums contains a "prev" album, which is loaded
            CB_ASSERT (!album(nPrevAlbum - 1).m_strTitle.empty());

            --nPrevAlbum;
            int nImgCnt (cSize(album(nPrevAlbum).m_vstrImageNames));
            nPrevImage = (m_bNavigateByAlbum && nImgCnt > 0) ? 0 : nImgCnt - 1; // !!! it's OK if images don't exist for that album
        }
        else
        { // nothing left in m_vAlbums
            if (m_bNavigateByAlbum && nPrevImage > 0)
            {
                m_nCrtAlbum = 0;
                m_nCrtImage = 0;
                reloadGui();
            }
            else
            {
                setWaiting(NOTHING);
            }
            return;
        }

        // if it got here, we have a loaded album (pictures might be missing, though)

        if ((m_pImageFltCkB->isChecked() && album(nPrevAlbum).m_vpImages.empty()) ||
            (m_pCdFltCkB->isChecked() && string::npos == album(nPrevAlbum).m_strFormat.find("CD")) || // Discogs has one format, but MusicBrainz may have several, so find() is used instead of "==" 
            (m_pTrackCntFltCkB->isChecked() && m_nExpectedTracks != cSize(album(nPrevAlbum).m_vTracks)))
        {
            continue;
        }

        if (-1 != nPrevImage && 0 == album(nPrevAlbum).m_vpImages[nPrevImage])
        {
            requestImage(nPrevAlbum, nPrevImage);
            return;
        }

        m_nCrtAlbum = nPrevAlbum;
        m_nCrtImage = nPrevImage;
        reloadGui();
        return;
    }
}


void AlbumInfoDownloaderDlgImpl::setImageType(const string& strName)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::setImageType");
    m_eLoadingImageCompr = ImageInfo::INVALID;
    string::size_type m (strName.rfind('.'));
    if (string::npos != m)
    {
        string s;
        ++m;
        for (; m < strName.size(); ++m)
        {
            s += tolower(strName[m]);
        }

        if (s == "jpg" || s == "jpeg")
        {
            m_eLoadingImageCompr = ImageInfo::JPG;
        }
        else if (s == "png")
        {
            m_eLoadingImageCompr = ImageInfo::PNG;
        }
    }
}



void AlbumInfoDownloaderDlgImpl::onRequestFinished(int /*nId*/, bool bError)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::onRequestFinished");
//cout << "received ID = " << nId << endl;
//if (1 == nId) { return; } // some automatically generated request, which should be ignored
    if (bError)
    {
        addNote("request error");
        resetNavigation();
        return;
    }

    QHttp* pQHttp (getWaitingHttp());

    qint64 nAv (pQHttp->bytesAvailable());
    if (0 == nAv)
    {
        //addNote("received empty response");
        //cout << "empty request returned\n";

        //m_eState = NORMAL; // !!! DON'T set m_eWaiting to NOTHING; empty responses come for no identifiable requests and they should be just ignored
        return;
    }

    CB_ASSERT (NOTHING != m_eWaiting);

    { QString qstrMsg (QString("received %1 bytes").arg(nAv)); addNote(qstrMsg.toLatin1().constData()); }

    QString qstrXml;

    QByteArray b (pQHttp->readAll());
    CB_ASSERT (b.size() == nAv);

    if (nAv < 10)
    { // too short
        addNote("received very short response; aborting request ...");
        resetNavigation();
        return;
    }

    if (IMAGE == m_eWaiting)
    {
        QString qstrInfo;
        QPixmap img;
        QByteArray comprImg (b);
        ImageInfo::Compr eOrigCompr (m_eLoadingImageCompr);

        if (img.loadFromData(b)) //ttt2 not sure what happens for huge images;
        {
            qstrInfo = QString("Original: %1kB, %2x%3").arg(nAv/1024).arg(img.width()).arg(img.height());
            //cout << "image size " << img.width() << "x" << img.height() << endl;

            int nWidth (img.width()), nHeight (img.height());

            if (nAv > ImageInfo::MAX_IMAGE_SIZE || m_eLoadingImageCompr == ImageInfo::INVALID)
            {
                QPixmap scaledImg;
                ImageInfo::compress(img, scaledImg, comprImg);
                nWidth = scaledImg.width(); nHeight = scaledImg.height();
                m_eLoadingImageCompr = ImageInfo::JPG;

                //cout << "scaled image size " << img.width() << "x" << img.height() << endl;
                qstrInfo += QString("\nRecompressed to: %1kB, %2x%3").arg(comprImg.size()/1024).arg(img.width()).arg(img.height());
            }
            else
            {
                qstrInfo += "\nNot recompressed";
            }
            onImageLoaded(comprImg, nWidth, nHeight, qstrInfo);
        }
        else
        {
            QMessageBox::critical(this, "Error", "Failed to load the image");
            const int SIZE (150);
            QPixmap errImg (SIZE, SIZE);
            QPainter pntr (&errImg);
            pntr.fillRect(0, 0, SIZE, SIZE, QColor(255, 128, 128));
            pntr.drawRect(0, 0, SIZE - 1, SIZE - 1);
            pntr.drawText(QRectF(0, 0, SIZE, SIZE), Qt::AlignCenter, "Error");
            qstrInfo = "Error loading image\n";
            comprImg.clear();
            QBuffer bfr (&comprImg);
            errImg.save(&bfr, "png");
            m_eLoadingImageCompr = ImageInfo::PNG;
            onImageLoaded(comprImg, SIZE, SIZE, qstrInfo);
        }

        if (m_bSaveResults) { saveDownloadedData(b.constData(), b.size(), (ImageInfo::JPG == eOrigCompr ? "jpg" : (ImageInfo::PNG == eOrigCompr ? "png" : "unkn"))); }

        return;
    }

    if (0x1f == (unsigned char)b[0] && 0x8b == (unsigned char)b[1])
    { // gzip
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = 0;
        strm.next_in = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(b.constData()));
        strm.avail_in = nAv;

        vector<char> v (nAv);

        //int nRes (inflateInit(&strm));
        int nRes (inflateInit2(&strm, 16 + 15)); // !!! see libz.h for details; "32" makes this able to handle both gzip and zlib, by auto-detecting the format; 16 is used to force gzip
        if (Z_OK != nRes) { addNote("init error"); goto e2; }

        strm.next_out = reinterpret_cast<unsigned char*>(&v[0]);
        strm.avail_out = v.size();

        //cout << (void*)strm.next_in << " " << strm.avail_in << " " << (void*)strm.next_out << " " << strm.avail_out << endl;
        for (;;)
        {
            nRes = inflate(&strm, Z_SYNC_FLUSH); // Z_FINISH
            //cout << (void*)strm.next_in << " " << strm.avail_in << " " << (void*)strm.next_out << " " << strm.avail_out << endl;
            if (Z_STREAM_END == nRes) { break; }
            if (Z_OK == nRes)
            { // extend the buffer
                v.resize(v.size() + 128);
                strm.next_out = reinterpret_cast<unsigned char*>(&v[v.size() - 128]);
                strm.avail_out = 128;
                continue;
            }

            addNote("unexpected result"); goto e2;
        }

        {
            int nUncomprSize (reinterpret_cast<char*>(strm.next_out) - &v[0]);
            v.resize(nUncomprSize + 1);
            v[nUncomprSize] = 0;
            qstrXml = &v[0];
        }

    e2:
        inflateEnd(&strm);
    }
    else
    { // it's not gzip, so perhaps it is ASCII; //ttt2 check that it's ASCII
        qstrXml = b;
    }

    if (qstrXml.isEmpty())
    {
        addNote("empty string received");
    }
    else
    {
        if (m_bSaveResults)
        {
            QByteArray b1 (qstrXml.toUtf8());
            saveDownloadedData(b1.constData(), b1.size(), "xml");
        }
    }

    switch (m_eWaiting)
    {
    case ALBUM:
        onAlbumLoaded(qstrXml);
        break;

    case SEARCH:
        onSearchLoaded(qstrXml);
        break;

    default:
        CB_ASSERT (false);
    }
}


string AlbumInfoDownloaderDlgImpl::getTempName() // time-based, with no extension; doesn't check for existing names, but uses a counter, so files shouldn't get removed (except during daylight saving time changes)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::getTempName");
    time_t t (time(0));
    if (t == m_nLastTime)
    {
        ++m_nLastCount;
    }
    else
    {
        m_nLastTime = t;
        m_nLastCount = 0;
    }

    char a [50];
#ifndef WIN32
    ctime_r(&t, &a[0]);
#else
    strcpy(a, ctime(&t)); //ttt2 try to get rid of ctime
#endif
    string s;
    const char* p (&a[0]);
    for (; 0 != *p; ++p)
    {
        char c (*p);
        if ('\n' == c || '\r' == c) { break; }
        s += ':' == c ? '.' : c;
    }
    sprintf(a, ".%03d", m_nLastCount);
    return s;
}



void AlbumInfoDownloaderDlgImpl::retryNavigation()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::retryNavigation");
    if (NEXT == m_eNavigDir)
    {
        next();
    }
    else if (PREV == m_eNavigDir)
    {
        previous();
    }
    else
    {
        CB_ASSERT (false);
    }

    if (NOTHING == m_eWaiting)
    {
        resetNavigation();
    }
    // ttt2 perhaps add assert that either there are no pending requests and NOTHING==m_eWaiting or there is 1 pending request and NOTHING!=m_eWaiting (keep in mind that when the connection is opened there is a system-generated request);
}


void AlbumInfoDownloaderDlgImpl::onSearchLoaded(const QString& qstrXml)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::onSearchLoaded");
    addNote("search results received");
    QByteArray b (qstrXml.toLatin1());
    QBuffer bfr (&b);
    //SearchXmlHandler hndl (*this);
    auto_ptr<QXmlDefaultHandler> pHndl (getSearchXmlHandler());
    QXmlDefaultHandler& hndl (*pHndl);
    QXmlSimpleReader rdr;

    rdr.setContentHandler(&hndl);
    rdr.setErrorHandler(&hndl);
    QXmlInputSource src (&bfr);
    if (!rdr.parse(src))
    {
        QMessageBox::critical(this, "Error", "Couldn't process the search result. (Usually this means that the server is busy, so trying later might work.)");
        if (0 == getAlbumCount())
        {
            m_nTotalPages = 0;
            m_nLastLoadedPage = -1;
        }
        resetNavigation();
        return;
    }

    if (0 == getAlbumCount() && m_nLastLoadedPage == m_nTotalPages - 1)
    {
        QMessageBox::critical(this, "Error", "No results found");
    }

    retryNavigation();
}


void AlbumInfoDownloaderDlgImpl::onAlbumLoaded(const QString& qstrXml)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::onAlbumLoaded");
    addNote("album info received");
    QByteArray b (qstrXml.toLatin1());
    QBuffer bfr (&b);
    //AlbumXmlHandler hndl (album(m_nLoadingAlbum));
    auto_ptr<QXmlDefaultHandler> pHndl (getAlbumXmlHandler(m_nLoadingAlbum));
    QXmlDefaultHandler& hndl (*pHndl);
    QXmlSimpleReader rdr;

    rdr.setContentHandler(&hndl);
    rdr.setErrorHandler(&hndl);
    QXmlInputSource src (&bfr);
    if (!rdr.parse(src))
    {
        //CB_ASSERT (false);
        QMessageBox::critical(this, "Error", "Couldn't process the album information. (Usually this means that the server is busy, so trying later might work.)");
        /*if (0 == getAlbumCount())
        {
            m_nTotalPages = 0;
            m_nLastLoadedPage = -1;
        }*/
        resetNavigation();
        return;
    }
    //cout << album(m_nLoadingAlbum);

    retryNavigation();
}


void AlbumInfoDownloaderDlgImpl::onImageLoaded(const QByteArray& comprImg, int nWidth, int nHeight, const QString& qstrInfo)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::onImageLoaded");
    addNote("image received");
    CB_ASSERT (0 == album(m_nLoadingAlbum).m_vpImages[m_nLoadingImage]);
    CB_ASSERT (ImageInfo::INVALID != m_eLoadingImageCompr);
    album(m_nLoadingAlbum).m_vpImages[m_nLoadingImage] = new ImageInfo(-1, ImageInfo::OK, m_eLoadingImageCompr, comprImg, nWidth, nHeight);
    album(m_nLoadingAlbum).m_vstrImageInfo[m_nLoadingImage] = convStr(qstrInfo);

    m_vpImages.push_back(album(m_nLoadingAlbum).m_vpImages[m_nLoadingImage]);

    retryNavigation();
}



void AlbumInfoDownloaderDlgImpl::addNote(const char* szNote)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::addNote");
    QString q (m_pDownloadsM->toPlainText());
    if (!q.isEmpty()) { q += "\n"; }
    {
        QTime t (QTime::currentTime());
        char a [15];
        sprintf(a, "%02d:%02d:%02d.%03d ", t.hour(), t.minute(), t.second(), t.msec());
        q += a;
    }
    q += szNote;
    m_pDownloadsM->setText(q);

    QScrollBar* p (m_pDownloadsM->verticalScrollBar());
    if (p->isVisible())
    {
        p->setValue(p->maximum());
    }
}



void AlbumInfoDownloaderDlgImpl::setWaiting(Waiting eWaiting)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::setWaiting");
    if (NOTHING == m_eWaiting && NOTHING != eWaiting)
    {
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    }

    if (NOTHING != m_eWaiting && NOTHING == eWaiting)
    {
        QApplication::restoreOverrideCursor();
    }

    m_eWaiting = eWaiting;
}


void AlbumInfoDownloaderDlgImpl::reloadGui()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::reloadGui");
    resetNavigation();

    if (0 == getAlbumCount()) { return; }
    CB_ASSERT (m_nCrtAlbum >= 0 && m_nCrtAlbum < getAlbumCount());

    //const MusicBrainzAlbumInfo& album (m_vAlbums[m_nCrtAlbum]);
    const WebAlbumInfoBase& albumInfo (album(m_nCrtAlbum));
    m_pFormatE->setText(convStr(albumInfo.m_strFormat));
    m_pRealeasedE->setText(convStr(albumInfo.m_strReleased));
    //m_pTrackListL->clear();
    updateTrackList();
    set<string> sstrPrefixes;
    for (int i = 0, n = cSize(albumInfo.m_vTracks); i < n; ++i)
    {
        const TrackInfo& trk (albumInfo.m_vTracks[i]);

        const string& s1 (trk.m_strPos);
        string::size_type k (s1.find_last_not_of("0123456789"));
        if (string::npos != k && s1.size() - 1 != k)
        {
            sstrPrefixes.insert(s1.substr(0, k + 1));
        }
    }

    m_pVolumeCbB->clear();
    if (sstrPrefixes.empty() || sstrPrefixes.size() == albumInfo.m_vTracks.size())
    {
        m_pVolumeCbB->setEnabled(false);
        m_pVolumeL->setEnabled(false);
    }
    else
    {
        m_pVolumeCbB->setEnabled(true);
        m_pVolumeL->setEnabled(true);
        for (set<string>::iterator it = sstrPrefixes.begin(); it != sstrPrefixes.end(); ++it)
        {
            m_pVolumeCbB->addItem(convStr(*it));
        }
        m_pVolumeCbB->addItem("<All>");
    }


    QString q1 (m_nTotalPages == m_nLastLoadedPage + 1 ? "" : "+");
    QString s (QString("Album %1/%2%3, image %4/%5").arg(m_nCrtAlbum + 1).arg(getAlbumCount()).arg(q1).arg(m_nCrtImage + 1).arg(albumInfo.m_vpImages.size()));
    m_pResultNoL->setText(s);

    m_pResArtistE->setText(convStr(albumInfo.m_strArtist));
    m_pResAlbumE->setText(convStr(albumInfo.m_strTitle));

    if (-1 == m_nCrtImage || 0 == albumInfo.m_vpImages[m_nCrtImage])
    {
        m_pImageL->setPixmap(0);
        m_pImageL->setText("");
        m_pImgSizeL->setText("No image\n");
    }
    else
    {
        m_pImageL->setPixmap(albumInfo.m_vpImages[m_nCrtImage]->getPixmap(m_pImageL->width(), m_pImageL->height()));
        m_pImgSizeL->setText(convStr(albumInfo.m_vstrImageInfo[m_nCrtImage]));
    }
}


/*override*/ void AlbumInfoDownloaderDlgImpl::updateTrackList()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::updateTrackList");
    m_pModel->emitLayoutChanged();

    SimpleQTableViewWidthInterface intf (*m_pTrackListG);
    ColumnResizer rsz (intf, 100, ColumnResizer::DONT_FILL, ColumnResizer::CONSISTENT_RESULTS);
}



/*static*/ string AlbumInfoDownloaderDlgImpl::removeParentheses(const string& s)
{
    string r;
    int k1 (0), k2 (0), k3 (0), k4 (0);
    for (int i = 0, n = cSize(s); i < n; ++i)
    {
        char c (s[i]);
        if ('(' == c)
        {
            ++k1;
        }
        if ('[' == c)
        {
            ++k2;
        }
        if ('{' == c)
        {
            ++k3;
        }
        if ('<' == c)
        {
            ++k3;
        }
        if (0 == k1 && 0 == k2 && 0 == k3 && 0 == k4)
        {
            r += c;
        }
        if (')' == c)
        {
            --k1;
        }
        if (']' == c)
        {
            --k2;
        }
        if ('}' == c)
        {
            --k3;
        }
        if ('>' == c)
        {
            --k4;
        }
    }
    trim(r);
    return r;
}


void AlbumInfoDownloaderDlgImpl::saveDownloadedData(const char* p, int nSize, const char* szExt)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::saveDownloadedData");
    string s (getTempName() + "." + szExt);
    ofstream_utf8 out (s.c_str(), ios::binary);
    out.write(p, nSize);
}


void AlbumInfoDownloaderDlgImpl::onHelp()
{
    openHelp("200_discogs_query.html");
}




//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================



void addIfMissing(string& strDest, const string& strSrc)
{
    if (strDest.empty())
    {
        strDest = strSrc;
    }
    else
    {
        if (string::npos == strDest.find(strSrc))
        {
            strDest += ", " + strSrc;
        }
    }
}

// splits a string based on a separator, putting the components in a vector; trims the substrings; discards empty components;
void split(const string& s, const string& sep, vector<string>& v)
{
    v.clear();
    string::size_type j (0), k;
    int n (cSize(s));
    do
    {
        k = s.find(sep, j);
        if (string::npos == k)
        {
            k = n;
        }
        string s1 (s.substr(j, k - j));
        trim(s1);
        if (!s1.empty())
        {
            v.push_back(s1);
        }
        j = k + 1;
    }
    while ((int)k < n);
}


void addList(string& strDest, const string& strSrc)
{
    if (strDest.empty())
    {
        strDest = strSrc;
    }
    else
    {
        vector<string> v;
        split (strSrc, ",", v);
        for (int i = 0, n = cSize(v); i < n; ++i)
        {
            addIfMissing(strDest, v[i]);
        }
    }
}


//====================================================================================================================
//====================================================================================================================
//====================================================================================================================


WebDwnldModel::WebDwnldModel(AlbumInfoDownloaderDlgImpl& dwnld, QTableView& grid) : QAbstractTableModel(&dwnld), m_dwnld(dwnld), m_grid(grid)
{
}


/*override*/ int WebDwnldModel::rowCount(const QModelIndex&) const
{
    const WebAlbumInfoBase* p (m_dwnld.getCrtAlbum());
    if (0 == p) { return 0; }
    return cSize(p->m_vTracks);
}

/*override*/ int WebDwnldModel::columnCount(const QModelIndex&) const
{
    return m_dwnld.getColumnCount();
}


/*override*/ QVariant WebDwnldModel::data(const QModelIndex& index, int nRole) const
{
//LAST_STEP("WebDwnldModel::data()");
    if (!index.isValid()) { return QVariant(); }
    if (nRole != Qt::DisplayRole && nRole != Qt::ToolTipRole) { return QVariant(); }
    int i (index.row()), j (index.column());

    //if (nRole == Qt::CheckStateRole && j == 2) { return Qt::Checked; }

    const WebAlbumInfoBase* p (m_dwnld.getCrtAlbum());
    if (0 == p || i >= cSize(p->m_vTracks))
    {
        if (nRole == Qt::ToolTipRole) { return ""; }
        return QVariant();
    }

    const TrackInfo& t (p->m_vTracks[i]);
    string s;
    switch (j)
    {
    case 0: s = t.m_strPos; break;
    case 1: s = t.m_strTitle; break;
    case 2: s = t.m_strArtist; break;
    case 3: s = t.m_strComposer; break;
    default:
        CB_ASSERT (false);
    }
    QString qs (convStr(s));

    if (nRole == Qt::ToolTipRole)
    {
        QFontMetrics fm (m_grid.fontMetrics()); // !!! no need to use "QApplication::fontMetrics()"
        int nWidth (fm.width(qs));

        if (nWidth + 10 < m_grid.horizontalHeader()->sectionSize(j)) // ttt2 "10" is hard-coded
        {
            //return QVariant();
            return ""; // !!! with "return QVariant()" the previous tooltip remains until the cursor moves over another cell that has a tooltip
        }//*/

        return qs;
    }

    return qs;
}


/*override*/ QVariant WebDwnldModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /*= Qt::DisplayRole*/) const
{
    if (nRole != Qt::DisplayRole) { return QVariant(); }
    if (Qt::Horizontal == eOrientation)
    {
        switch (nSection)
        {
        case 0: return "Pos";
        case 1: return "Title";
        case 2: return "Artist";
        case 3: return "Composer";
        default:
            CB_ASSERT (false);
        }
    }

    return nSection + 1;
}


