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

#include  <QMessageBox>
#include  <QBuffer>
#include  <QPainter>
#include  <QScrollBar>
#include  <QHeaderView>
#include  <QTime>
#include  <QPixmap>

#include  "AlbumInfoDownloaderDlgImpl.h"

#include  "Helpers.h"
#include  "ColumnResizer.h"
#include  "Widgets.h"


#include  "fstream_unicode.h"

using namespace std;
using namespace pearl;

// /*extern*/ int CELL_WIDTH (22);
extern int CELL_HEIGHT;




bool WebAlbumInfoBase::checkTrackCountMatch(int nExpected) const
{
    if (cSize(m_vVolumes[0].m_vTracks) == nExpected)
    {
        return true;
    }
    return getTotalTrackCount() == nExpected;
}


int WebAlbumInfoBase::getTotalTrackCount() const
{
    return cSize(m_vpTracks);
}


AlbumInfoDownloaderDlgImpl::AlbumInfoDownloaderDlgImpl(QWidget* pParent, SessionSettings& settings, bool bSaveResults) : QDialog(pParent, getDialogWndFlags()), Ui::AlbumInfoDownloaderDlg(), m_bSaveResults(bSaveResults), m_nLastCount(0), m_nLastTime(0), m_settings(settings)
{
    setupUi(this);

    connect(&m_networkAccessManager, &QNetworkAccessManager::finished, this, &AlbumInfoDownloaderDlgImpl::onRequestFinished);

    m_pTrackListG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
    m_pTrackListG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);
    decreaseRowHeaderFont(*m_pTrackListG);
    setHeaderColor(m_pTrackListG);

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

            if (m_pVolumeCbB->isEnabled())
            {
                if (m_pVolumeCbB->currentIndex() == m_pVolumeCbB->count() - 1)
                { // just give sequential numbers when "<All>" in a multivolume is used - see https://sourceforge.net/projects/mp3diags/forums/forum/947206/topic/4503061/index/page/1 - //ttt0 perhaps can be improved, maybe have a checkbox to keep the numbers
                    char a [15];
                    int crtPos = 1;
                    for (auto& vol : pAlbumInfo->m_vVolumes)
                    {
                        for (auto& trk : vol.m_vTracks)
                        {
                            sprintf(a, "%02d", crtPos);
                            crtPos++;
                            trk.m_strPos = a;
                        }
                    }
                }
                else
                {
                    // Just keep the current volume
                    vector<VolumeInfo> v;
                    v.emplace_back(pAlbumInfo->m_vVolumes[m_pVolumeCbB->currentIndex()]);
                    pAlbumInfo->m_vVolumes.swap(v);
                    pAlbumInfo->m_vpTracks.clear();
                    for (auto& trk : pAlbumInfo->m_vVolumes[0].m_vTracks)
                    {
                        pAlbumInfo->m_vpTracks.emplace_back(&trk);
                    }
                }
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
void AlbumInfoDownloaderDlgImpl::resetNavigation()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::resetNavigation");
    unordered_set<QNetworkReply*> spNetworkReplies (m_spNetworkReplies); // !!! Calling pReply->abort() triggers its own call to resetNavigation(), leading to changing a set that is being processed
    m_spNetworkReplies.clear();
    // qDebug("%s, m_spNetworkReplies size: %zu", getCurrentThreadInfo().c_str(), spNetworkReplies.size());
    for (QNetworkReply* pReply : spNetworkReplies)
    {
        qDebug("Aborting reply %p", pReply);
        //const string& s1 = convStr(pReply->errorString());
        //QNetworkReply::NetworkError error = pReply->error();
        pReply->abort();
        //pReply->deleteLater(); //!!! Don't call this, as the call to abort() triggers onRequestFinished(), which has its own call to deleteLater()
    }

    setWaiting(NOTHING);
    m_eNavigDir = NONE;
}
//ttt9: See about thread synchronization


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

/*static*/ const char* AlbumInfoDownloaderDlgImpl::NOT_FOUND_AT_AMAZON = QT_TRANSLATE_NOOP("AlbumInfoDownloaderDlgImpl", "not found at amazon.com");

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
    m_pImageL->setPixmap(QPixmap());
    m_pImageL->setText("");
    m_pImgSizeL->setText("\n");
    m_pViewAtAmazonL->setText(tr(NOT_FOUND_AT_AMAZON));

    m_strQuery = createQuery(); // e.g. ws/2/release/?query=release:Help AND artist:Beatles&fmt=json, without page number, site, protocol; to be used by loadNextPage();

    if (m_strQuery.empty())
    {
        showCritical(this, tr("Error"), tr("You must specify at least an artist or an album"));
        return;
    }

    m_nTotalEntryCnt = 1; //!!! This is set to 1 so "next()" thinks there are additional entries
    m_nLastLoadedEntry = -1;
    m_nCrtAlbum = -1; m_nCrtImage = -1;
    resetNavigation();
    //m_eNavigDir = NEXT;
    m_eNavigDir = NEXT;
    m_bNavigateByAlbum = false;
    addNote(tr("searching ..."));
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
        showCritical(this, tr("Error"), tr("You cannot save the results now, because a request is still pending"));
        return;
    }

    if (0 == getAlbumCount())
    {
        showCritical(this, tr("Error"), tr("You cannot save the results now, because no album is loaded"));
        return;
    }

    int nCnt (0);
    CB_ASSERT (m_nCrtAlbum >= 0 && m_nCrtAlbum < getAlbumCount());// ttt0 triggered according to mail on 2015.12.13
    const WebAlbumInfoBase& albumInfo (album(m_nCrtAlbum));
    int crtVol = m_pVolumeCbB->currentIndex();
    if (m_pVolumeCbB->isEnabled() && crtVol != m_pVolumeCbB->count() - 1)
    {
        nCnt = cSize(albumInfo.m_vVolumes[crtVol].m_vTracks);
    }
    else
    {
        nCnt = albumInfo.getTotalTrackCount();
    }

    if (nCnt != m_nExpectedTracks)
    {
        QString s;
        const QString& qstrVolMsg (
            m_pVolumeCbB->isEnabled() &&
            (
                (nCnt > m_nExpectedTracks && crtVol == m_pVolumeCbB->count() - 1) ||
                (nCnt < m_nExpectedTracks && crtVol != m_pVolumeCbB->count() - 1)
            )
            ? tr("You may want to use a different volume selection on this multi-volume release.\n\n") : "");

        if (nCnt > m_nExpectedTracks)
        {
            s = tr("A number of %1 tracks were expected, but your selection contains %2. Additional tracks will be discarded.\n\n%3Save anyway?").arg(m_nExpectedTracks).arg(nCnt).arg(qstrVolMsg);
        }
        else
        {
            s = tr("A number of %1 tracks were expected, but your selection only contains %2. Remaining tracks will get null values.\n\n%3Save anyway?").arg(m_nExpectedTracks).arg(nCnt).arg(qstrVolMsg);
        }

        if (showMessage(this, QMessageBox::Question, 1, 1, tr("Count inconsistency"), s, tr("&Save"), tr("Cancel")) != 0) { return; }
    }

    m_bSaveImageOnly = false;
    accept();
}

void AlbumInfoDownloaderDlgImpl::on_m_pSaveImageB_clicked()
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::on_m_pSaveImageB_clicked");
    if (NOTHING != m_eWaiting)
    {
        showCritical(this, tr("Error"), tr("You cannot save the results now, because a request is still pending"));
        return;
    }

    if (0 == getAlbumCount() || -1 == m_nCrtImage)
    {
        showCritical(this, tr("Error"), tr("You cannot save any image now, because there is no image loaded"));
        return;
    }
    // ttt2 perhaps shouldn't save an "error" image

    m_bSaveImageOnly = true;
    accept();
    //ttt2 perhaps allow multiple images to be saved, by adding a button to "add to save list"; see https://sourceforge.net/projects/mp3diags/forums/forum/947207/topic/4006484
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
            // Show an already loaded image
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
            if (m_nLastLoadedEntry == m_nTotalEntryCnt - 1)
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
            (m_pTrackCntFltCkB->isChecked() && !album(nNextAlbum).checkTrackCountMatch(m_nExpectedTracks))) // ttt2 MusicBrainz could perform better here, because it knows how many tracks are on an album without actually loading it; ttt2 redo the whole next() / prev() thing in a more logical way; perhaps separate next() from nextAlbum();
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
            (m_pTrackCntFltCkB->isChecked() && !album(nPrevAlbum).checkTrackCountMatch(m_nExpectedTracks)))
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



void AlbumInfoDownloaderDlgImpl::onRequestFinished(QNetworkReply* pReply)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::onRequestFinished");
    //const string& strUrl = pReply->request().url().toString().toStdString();
    //const string& strErr = pReply->error() != QNetworkReply::NetworkError::NoError ? pReply->errorString().toStdString() : "no error";
    //qDebug("%d, AlbumInfoDownloaderDlgImpl::onRequestFinished(%p): %s / %s", __LINE__, pReply, strUrl.c_str(), strErr.c_str());
    // qDebug("%d, %s, AlbumInfoDownloaderDlgImpl::onRequestFinished(%p): %s / %s", __LINE__, getCurrentThreadInfo().c_str(), pReply, strUrl.c_str(), strErr.c_str());

    //CB_ASSERT (m_spNetworkReplies.count(pReply) == 1); //!!! Depending on how it got here, the assert might be wrong: resetNavigation() may call abort(), which calls here, but before that it erases m_spNetworkReplies in order to avoid recursive calls leading to crashes
    pReply->deleteLater();
    //addNote(QString("QQQ Request to %1 finished with status: %2").arg(strUrl.c_str()).arg(strErr.c_str()));
    m_spNetworkReplies.erase(pReply);
//cout << "received ID = " << nId << endl;
//if (1 == nId) { return; } // some automatically generated request, which should be ignored
    if (pReply->error() != QNetworkReply::NetworkError::NoError)
    {
        const string& err = convStr(pReply->errorString());
        addNote(tr("request error") + ": " + pReply->errorString());
        if (IMAGE == m_eWaiting)
        {
            handleImageError();
            return;
        }

        resetNavigation();
        return;
    }

    qint64 nAv (pReply->bytesAvailable());
    if (0 == nAv)
    {
        //addNote("QQQ empty rsp");
        //addNote("received empty response");
        //cout << "empty request returned\n";

        //m_eState = NORMAL; // !!! DON'T set m_eWaiting to NOTHING; empty responses come for no identifiable requests and they should be just ignored
        return;
    }

    CB_ASSERT (NOTHING != m_eWaiting);

    { QString qstrMsg (tr("received %1 bytes").arg(nAv)); addNote(qstrMsg); }

    QString qstrJson;

    QByteArray b (pReply->readAll());
    CB_ASSERT (b.size() == nAv);

    if (nAv < 10)
    { // too short
        addNote(tr("received very short response; aborting request ..."));
        resetNavigation();
        return;
    }

    if (IMAGE == m_eWaiting)
    {
        QString qstrInfo;
        QImage img;
        //b[0] = 4; // This makes decompression not work, to test handleImageError()
        QByteArray comprImg (b);
        ImageInfo::Compr eOrigCompr (m_eLoadingImageCompr);

        if (img.loadFromData(b)) //ttt2 not sure what happens for huge images;
        {
            qstrInfo = tr("Original: %1kB, %2x%3").arg(nAv/1024).arg(img.width()).arg(img.height());
            //cout << "image size " << img.width() << "x" << img.height() << endl;

            int nWidth (img.width()), nHeight (img.height());

            if (nAv > ImageInfo::MAX_IMAGE_SIZE || m_eLoadingImageCompr == ImageInfo::INVALID)
            {
                QImage scaledImg;
                ImageInfo::compress(img, scaledImg, comprImg);
                nWidth = scaledImg.width(); nHeight = scaledImg.height();
                m_eLoadingImageCompr = ImageInfo::JPG;

                //cout << "scaled image size " << img.width() << "x" << img.height() << endl;
                qstrInfo += tr("\nRecompressed to: %1kB, %2x%3").arg(comprImg.size()/1024).arg(img.width()).arg(img.height());
            }
            else
            {
                qstrInfo += tr("\nNot recompressed");
            }
            onImageLoaded(comprImg, nWidth, nHeight, qstrInfo);
        }
        else
        {
            handleImageError();
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
        if (Z_OK != nRes) { addNote(tr("init error")); goto e2; }

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

            addNote(tr("unexpected result")); goto e2;
        }

        {
            int nUncomprSize (reinterpret_cast<char*>(strm.next_out) - &v[0]);
            v.resize(nUncomprSize + 1);
            v[nUncomprSize] = 0;
            qstrJson = &v[0];
        }

    e2:
        inflateEnd(&strm);
    }
    else
    { // it's not gzip, so perhaps it is ASCII; //ttt2 check that it's ASCII
        qstrJson = b;
    }

    if (qstrJson.isEmpty())
    {
        addNote(tr("empty string received"));
    }
    else
    {
        //addNote("QQQ rsp: " + qstrJson);
        //addNote(QString("QQQ got response of size %1").arg(qstrJson.size()));
        if (m_bSaveResults)
        {
            QByteArray b1 (qstrJson.toUtf8());
            saveDownloadedData(b1.constData(), b1.size(), "json");
        }
    }

    switch (m_eWaiting)
    {
    case ALBUM:
        onAlbumLoaded(qstrJson);
        break;

    case SEARCH:
        onSearchLoaded(qstrJson);
        break;

    default:
        CB_ASSERT (false);
    }
}


/**
 * Generates an image that shows "Error", then continues the navigation, so the track info can be displayed even if the image is not OK
 */
void AlbumInfoDownloaderDlgImpl::handleImageError()
{
    showCritical(this, tr("Error"), tr("Failed to load the image"));
    const int SIZE (150);
    QImage errImg (SIZE, SIZE, QImage::Format_ARGB32);
    QPainter pntr (&errImg);
    pntr.fillRect(0, 0, SIZE, SIZE, QColor(255, 128, 128));
    pntr.drawRect(0, 0, SIZE - 1, SIZE - 1);
    pntr.drawText(QRectF(0, 0, SIZE, SIZE), Qt::AlignCenter, tr("Error"));
    QString qstrInfo;
    qstrInfo = tr("Error loading image\n");
    QByteArray comprImg;
    QBuffer bfr (&comprImg);
    errImg.save(&bfr, "png");
    m_eLoadingImageCompr = ImageInfo::PNG;
    onImageLoaded(comprImg, SIZE, SIZE, qstrInfo);
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


void AlbumInfoDownloaderDlgImpl::onSearchLoaded(const QString& qstrJson)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::onSearchLoaded");
    addNote(tr("search results received"));
    unique_ptr<JsonHandler> pHndl (getSearchJsonHandler());

    if (!pHndl->handle(qstrJson))
    {
        showCritical(this, tr("Error"), tr("Couldn't process the search result. (Usually this means that the server is busy, so trying later might work.)\n\nError: %1").arg(pHndl->getError()));
        if (0 == getAlbumCount())
        {
            m_nTotalEntryCnt = 0;
            m_nLastLoadedEntry = -1;
        }
        resetNavigation();
        return;
    }

    if (0 == getAlbumCount() && m_nLastLoadedEntry == m_nTotalEntryCnt - 1)
    {
        // With Discogs, it is possible to have 0 albums so far but more to load, if all entries until now were artists
        showCritical(this, tr("Error"), tr("No results found"));
    }

    retryNavigation();
}


void AlbumInfoDownloaderDlgImpl::onAlbumLoaded(const QString& qstrJson)
{
LAST_STEP("AlbumInfoDownloaderDlgImpl::onAlbumLoaded");
    addNote(tr("album info received"));
    unique_ptr<JsonHandler> pHndl (getAlbumJsonHandler(m_nLoadingAlbum));
    if (!pHndl->handle(qstrJson))
    {
        //CB_ASSERT (false);
        showCritical(this, tr("Error"), tr("Couldn't process the album information. (Usually this means that the server is busy, so trying later might work.)\n\nError: %1").arg(pHndl->getError()));
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
    addNote(tr("image received")); //ttt0: Try to distinguish between a real image and one generated in handleImageError()
    CB_ASSERT (0 == album(m_nLoadingAlbum).m_vpImages[m_nLoadingImage]);
    CB_ASSERT (ImageInfo::INVALID != m_eLoadingImageCompr);
    album(m_nLoadingAlbum).m_vpImages[m_nLoadingImage] = new ImageInfo(-1, ImageInfo::OK, m_eLoadingImageCompr, comprImg, nWidth, nHeight);
    album(m_nLoadingAlbum).m_vstrImageInfo[m_nLoadingImage] = convStr(qstrInfo);

    m_vpImages.push_back(album(m_nLoadingAlbum).m_vpImages[m_nLoadingImage]);

    retryNavigation();
}



void AlbumInfoDownloaderDlgImpl::addNote(const QString& qstrNote)
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
    q += qstrNote;
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

    m_pVolumeCbB->clear();
    if (albumInfo.m_vVolumes.size() <= 1)
    {
        m_pVolumeCbB->setEnabled(false);
        m_pVolumeL->setEnabled(false);
    }
    else
    {
        m_pVolumeCbB->setEnabled(true);
        m_pVolumeL->setEnabled(true);
        for (auto& vol : albumInfo.m_vVolumes)
        {
            m_pVolumeCbB->addItem(convStr(vol.m_strName));
        }
        m_pVolumeCbB->addItem(tr("<All>"));
    }


    QString q1 (m_nTotalEntryCnt == m_nLastLoadedEntry + 1 ? "" : "+");
    QString s (tr("Album %1/%2%3, image %4/%5").arg(m_nCrtAlbum + 1).arg(getAlbumCount()).arg(q1).arg(m_nCrtImage + 1).arg(albumInfo.m_vpImages.size()));
    m_pResultNoL->setText(s);

    m_pResArtistE->setText(convStr(albumInfo.m_strArtist));
    m_pResAlbumE->setText(convStr(albumInfo.m_strTitle));

    if (-1 == m_nCrtImage || 0 == albumInfo.m_vpImages[m_nCrtImage])
    {
        m_pImageL->setPixmap(QPixmap());
        m_pImageL->setText("");
        m_pImgSizeL->setText(tr("No image\n"));
    }
    else
    {
        m_pImageL->setPixmap(QPixmap::fromImage(albumInfo.m_vpImages[m_nCrtImage]->getImage(m_pImageL->width(), m_pImageL->height())));
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
    return p->getTotalTrackCount();
}

/*override*/ int WebDwnldModel::columnCount(const QModelIndex&) const
{
    const WebAlbumInfoBase* p (m_dwnld.getCrtAlbum());
    return m_dwnld.getColumnCount() + (p != nullptr && p->isMultiVolume() ? 1 : 0);
}


/*override*/ QVariant WebDwnldModel::data(const QModelIndex& index, int nRole) const
{
//LAST_STEP("WebDwnldModel::data()");
    if (!index.isValid()) { return QVariant(); }
    if (nRole != Qt::DisplayRole && nRole != Qt::ToolTipRole) { return QVariant(); }
    int i (index.row()), j (index.column());

    //if (nRole == Qt::CheckStateRole && j == 2) { return Qt::Checked; }

    const WebAlbumInfoBase* p (m_dwnld.getCrtAlbum());
    if (0 == p || i >= cSize(p->m_vpTracks))
    {
        if (nRole == Qt::ToolTipRole) { return ""; }
        return QVariant();
    }

    const TrackInfo& t (*p->m_vpTracks[i]);
    string s;
    int pos = j + (p->isMultiVolume() ? 0 : 1);
    switch (pos)
    {
    case 0: s = t.m_strVolume; break;
    case 1: s = t.m_strPos; break;
    case 2: s = t.m_strTitle; break;
    case 3: s = t.m_strArtist; break;
    case 4: s = t.m_strComposer; break;
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


/*override*/ QVariant WebDwnldModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /* = Qt::DisplayRole*/) const
{
    static QString headers[] = {tr("Volume"), tr("Pos"), tr("Title"), tr("Artist"), tr("Composer")};
    if (nRole != Qt::DisplayRole) { return QVariant(); }
    if (Qt::Horizontal == eOrientation)
    {
        CB_ASSERT(nSection >= 0);
        const WebAlbumInfoBase* p = m_dwnld.getCrtAlbum();
        if (p == nullptr || !p->isMultiVolume())
        {
            nSection++;
        }
        CB_ASSERT(nSection <= 4);
        return headers[nSection];
    }

    return nSection + 1;
}


