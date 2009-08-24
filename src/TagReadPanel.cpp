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


#include  <QLineEdit>
#include  <QLabel>
#include  <QGridLayout>
#include  <QTextEdit>

#include  "TagReadPanel.h"

#include  "DataStream.h"
#include  "Helpers.h"
//ttt0 set tab size to something small

using namespace std;

//ttt1 switch from lineedits to grid (or make more space somehow) 25px / row with lineedit vs 20 in grid
TagReadPanel::TagReadPanel(QWidget* pParent, TagReader* pTagReader) : QFrame(pParent), m_pTagReader(pTagReader)
{
    //QVBoxLayout* pMainLayout (new QVBoxLayout());
    QGridLayout* pMainLayout (new QGridLayout());
    QGridLayout* pGridLayout (new QGridLayout());

    QFrame* pFrame (new QFrame(this));
    pFrame->setLayout(pGridLayout);
    setLayout(pMainLayout);
    pGridLayout->setSpacing(2);
    pMainLayout->setSpacing(2); // doesn't really matter: there's only one layout inside
    //setFrameShadow(QFrame::Sunken);
    setFrameShape(QFrame::StyledPanel);
    //setContentsMargins(0, 0, 0, 0);
    //setContentsMargins(1, 1, 1, 1);
    //setContentsMargins(2, 2, 2, 2);
    //setContentsMargins(20, 20, 20, 20);
    //pMainLayout->setContentsMargins(4, 4, 4, 4);
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    //setContentsMargins(4, 4, 4, 4);
    //pGridLayout->setContentsMargins(4, 4, 4, 4);
    //pGridLayout->setContentsMargins(1, 1, 1, 1);
    //pGridLayout->setContentsMargins(3, 3, 3, 3);
    pGridLayout->setContentsMargins(6, 6, 6, 6);
    pMainLayout->addWidget(pFrame);
//    pMainLayout->addStretch(0);

    QLabel* pLabel (new QLabel(m_pTagReader->getName()));
    QFont font (pLabel->font());
    int nSize (font.pixelSize());
    if (-1 == nSize)
    {
        nSize = font.pointSize();
        if (-1 != nSize)
        {
            nSize = nSize*4/3;
            font.setPointSize(nSize);
        }
    }
    else
    {
        nSize = nSize*4/3;
        font.setPixelSize(nSize);
    }
    font.setBold(true);
    pLabel->setFont(font);

    pGridLayout->addWidget(pLabel, 0, 0, 1, 2);


    m_pTrackNameE = new QLineEdit(pFrame);
    m_pTrackArtistE = new QLineEdit(pFrame);
    m_pTrackNumberE = new QLineEdit(pFrame);
    m_pTrackTimeE = new QLineEdit(pFrame);
    m_pTrackGenreE = new QLineEdit(pFrame);
    m_pAlbumNameE = new QLineEdit(pFrame);

    m_pPictureL = new QLabel("<No picture available>", pFrame); //ttt1 ID3V1 and others don't "support" it, so of course it's not available
    m_pPictureSizeL = new QLabel(pFrame);

    pGridLayout->addWidget(m_pTrackNameE, 1, 1, 1, 2);
    pGridLayout->addWidget(m_pTrackArtistE, 2, 1, 1, 2);
    pGridLayout->addWidget(m_pTrackNumberE, 3, 1, 1, 2);
    pGridLayout->addWidget(m_pTrackTimeE, 4, 1, 1, 2);
    pGridLayout->addWidget(m_pTrackGenreE, 5, 1, 1, 2);
    pGridLayout->addWidget(m_pAlbumNameE, 6, 1, 1, 2);
    pGridLayout->addWidget(m_pPictureL, 7, 1, 1, 1);
    pGridLayout->addWidget(m_pPictureSizeL, 7, 2, 1, 1);

    //m_pPictureL->setMinimumSize(64, 64);
    //m_pPictureL->setMaximumSize(64, 64);
    m_pPictureL->setMinimumHeight(64);
    m_pPictureL->setMaximumHeight(64);

    pGridLayout->addWidget(new QLabel("Track Name"), 1, 0, 1, 1);
    pGridLayout->addWidget(new QLabel("Track Artist"), 2, 0, 1, 1);
    pGridLayout->addWidget(new QLabel("Track Number"), 3, 0, 1, 1);
    pGridLayout->addWidget(new QLabel("Track Time"), 4, 0, 1, 1);
    pGridLayout->addWidget(new QLabel("Track Genre"), 5, 0, 1, 1);
    pGridLayout->addWidget(new QLabel("Album Name"), 6, 0, 1, 1);
    pGridLayout->addWidget(new QLabel("Picture"), 7, 0, 1, 1);

    //pMainLayout->addLayout(pGridLayout);
    //pFrame->setFrameShape(QFrame::StyledPanel);

    //pFrame->setMaximumWidth(400);
    setMaximumWidth(400);

    QPalette grayPalette (m_pTrackNameE->palette());
    //grayPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(255, 0, 0));
    grayPalette.setColor(QPalette::Base, grayPalette.color(QPalette::Disabled, QPalette::Window));

    switch (pTagReader->getSupport(TagReader::TITLE))
    {
    case TagReader::NOT_SUPPORTED: m_pTrackNameE->setEnabled(false); m_pTrackNameE->setPalette(grayPalette); break; /*m_pTrackNameE->setBackgroundRole(QPalette::Window);*/
    case TagReader::READ_ONLY: m_pTrackNameE->setReadOnly(true); //break;
    /*case TagReader::READ_WRITE:*/ m_pTrackNameE->setText(convStr(pTagReader->getTitle()));
    }

    switch (pTagReader->getSupport(TagReader::ARTIST))
    {
    case TagReader::NOT_SUPPORTED: m_pTrackArtistE->setEnabled(false); m_pTrackArtistE->setPalette(grayPalette); break;
    case TagReader::READ_ONLY: m_pTrackArtistE->setReadOnly(true); //break;
    /*case TagReader::READ_WRITE:*/ m_pTrackArtistE->setText(convStr(pTagReader->getArtist()));
    }

    switch (pTagReader->getSupport(TagReader::TRACK_NUMBER))
    {
    case TagReader::NOT_SUPPORTED: m_pTrackNumberE->setEnabled(false); m_pTrackNumberE->setPalette(grayPalette); break;
    case TagReader::READ_ONLY: m_pTrackNumberE->setReadOnly(true); //break;
    /*case TagReader::READ_WRITE:*/ m_pTrackNumberE->setText(convStr(pTagReader->getTrackNumber()));
    }

    switch (pTagReader->getSupport(TagReader::TIME))
    {
    case TagReader::NOT_SUPPORTED: m_pTrackTimeE->setEnabled(false); m_pTrackTimeE->setPalette(grayPalette); break;
    case TagReader::READ_ONLY: m_pTrackTimeE->setReadOnly(true); //break;
    /*case TagReader::READ_WRITE:*/
        {
            string s (pTagReader->getTime().asString());
            m_pTrackTimeE->setText(convStr(s));
        }
    }

    switch (pTagReader->getSupport(TagReader::GENRE))
    {
    case TagReader::NOT_SUPPORTED: m_pTrackGenreE->setEnabled(false); m_pTrackGenreE->setPalette(grayPalette); break;
    case TagReader::READ_ONLY: m_pTrackGenreE->setReadOnly(true); //break;
    /*case TagReader::READ_WRITE:*/ m_pTrackGenreE->setText(convStr(pTagReader->getGenre()));
    }

    switch (pTagReader->getSupport(TagReader::IMAGE))
    {
    case TagReader::NOT_SUPPORTED:
        //m_pPictureL->setEnabled(false); m_pPictureL->setPalette(grayPalette);
        break;
    case TagReader::READ_ONLY: //m_pPictureL->setReadOnly(true); //break;
    /*case TagReader::READ_WRITE:*/
        //ImageStatus eImageStatus;
        ImageInfo img (pTagReader->getImage());
        switch (img.getStatus())
        {
        case ImageInfo::OK:
        case ImageInfo::LOADED_NOT_COVER:
            CB_ASSERT (!img.isNull());
            {
                m_pPictureL->setPixmap(img.getPixmap(64));

                QString s;
                s.sprintf("%d x %d", img.getWidth(), img.getHeight());
                m_pPictureSizeL->setText(s);

                m_pPictureSizeL->setText(s + "\n" + img.getImageType());
            }
            break;

        case ImageInfo::USES_LINK:
            m_pPictureL->setText("Links not supported (yet)");
            break;

        case ImageInfo::ERROR_LOADING:
            m_pPictureL->setText("Error loading picture.");
            break;

        case ImageInfo::NO_PICTURE_FOUND:
            break;

        default:
            CB_ASSERT (false);
        }
    }

    switch (pTagReader->getSupport(TagReader::ALBUM))
    {
    case TagReader::NOT_SUPPORTED: m_pAlbumNameE->setEnabled(false); m_pAlbumNameE->setPalette(grayPalette); break;
    case TagReader::READ_ONLY: m_pAlbumNameE->setReadOnly(true); //break;
    /*case TagReader::READ_WRITE:*/ m_pAlbumNameE->setText(convStr(pTagReader->getAlbumName()));
    }

    //case TagReader::RATING:
    //case TagReader::COMPOSER:; //ttt1


    QTextEdit* pOtherInfoM (new QTextEdit());
    pOtherInfoM->setPlainText(convStr(pTagReader->getOtherInfo()));
    pGridLayout->addWidget(pOtherInfoM, 8, 0, 1, 3);
    pGridLayout->setRowStretch(8, 1); // seems to work ok without this; ttt2 see why
//pGridLayout->setRowStretch(3, 1);

    //pOtherInfoM->setEnabled(false);
    pOtherInfoM->setPalette(grayPalette);
    pOtherInfoM->setReadOnly(true);
}

