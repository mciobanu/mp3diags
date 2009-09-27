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


#include  <QLabel>
#include  <QVBoxLayout>
#include  <QTextEdit>
#include  <QTableWidget>
#include  <QHeaderView>
#include  <QApplication>
#include  <QToolButton>

#include  "TagReadPanel.h"

#include  "DataStream.h"
#include  "Helpers.h"


extern int CELL_HEIGHT;


using namespace std;


TagReadPanel::TagReadPanel(QWidget* pParent, TagReader* pTagReader) : QFrame(pParent)
{
    QVBoxLayout* pLayout (new QVBoxLayout(this));
    pLayout->setSpacing(6);
    pLayout->setContentsMargins(0, 10, 0, 0);

    setMaximumWidth(400);

    {
        QLabel* pLabel (new QLabel(pTagReader->getName(), this));
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

        pLayout->addWidget(pLabel);
    }

    QColor gray (palette().color(QPalette::Window));

    {
        QTableWidget* pTable (new QTableWidget(this));
        pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        pTable->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        pTable->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);
        pTable->horizontalHeader()->setStretchLastSection(true);
        pTable->setRowCount(7);
        pTable->setColumnCount(1);
        QStringList lLabels;
        lLabels << "Title" << "Artist" << "Track#" << "Time" << "Genre" << "Composer" << "Album";
        pTable->setVerticalHeaderLabels(lLabels);
        pTable->horizontalHeader()->hide();
        //pTable->setMinimumHeight(300);
        int nHeight (CELL_HEIGHT*7 + 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth));  //ttt not sure it's right; try several styles
        pTable->setMaximumHeight(nHeight);
        pTable->setMinimumHeight(nHeight);
        //pTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        pTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        //pTable->setFrameShape(QFrame::NoFrame);

        QTableWidgetItem* pItem;

        pItem = new QTableWidgetItem(); // !!! from doc: The table takes ownership of the item.
        switch (pTagReader->getSupport(TagReader::TITLE))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getTitle()));
        }
        pTable->setItem(0, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::ARTIST))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getArtist()));
        }
        pTable->setItem(1, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::TRACK_NUMBER))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getTrackNumber()));
        }
        pTable->setItem(2, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::TIME))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default:
            pItem->setText(pTagReader->getTime().asString());
        }
        pTable->setItem(3, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::GENRE))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getGenre()));
        }
        pTable->setItem(4, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::COMPOSER))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getComposer()));
        }
        pTable->setItem(5, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::ALBUM))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getAlbumName()));
        }
        pTable->setItem(6, 0, pItem);

        pLayout->addWidget(pTable);
    }


    {
        // ttt2 perhaps use QScrollArea
        vector<ImageInfo> vImg (pTagReader->getImages());
        QWidget* pImgWidget (0);
        QHBoxLayout* pImgWidgetLayout (0);

        for (int i = 0; i < cSize(vImg); ++i)
        {
            const ImageInfo& img (vImg[i]);

            if (0 == pImgWidget)
            {
                pImgWidget = new QWidget (this);
                pImgWidgetLayout = new QHBoxLayout(pImgWidget);
                pImgWidgetLayout->setContentsMargins(0, 0, 0, 0);
                //pImgWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                pImgWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                //pImgWidget->setMaximumHeight(40);
            }

            ImageInfoPanel* p (new ImageInfoPanel (this, img));

            pImgWidgetLayout->addWidget(p);
        }

        if (0 != pImgWidget)
        {
            pImgWidgetLayout->addStretch();
            pLayout->addWidget(pImgWidget);
        }
    }


    {
        QTextEdit* pOtherInfoM (new QTextEdit(this));
        pOtherInfoM->setPlainText(convStr(pTagReader->getOtherInfo()));
        //pOtherInfoM->setTabStopWidth(fontMetrics().width("aBcDeF"));
        pOtherInfoM->setTabStopWidth(fontMetrics().width("F"));

        //pOtherInfoM->setEnabled(false);
        QPalette grayPalette (pOtherInfoM->palette());
        grayPalette.setColor(QPalette::Base, gray);
        pOtherInfoM->setPalette(grayPalette);
        pOtherInfoM->setReadOnly(true);

        pOtherInfoM->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        //pOtherInfoM->setFrameShape(QFrame::NoFrame);

        pLayout->addWidget(pOtherInfoM);
    }

}


ImageInfoPanel::ImageInfoPanel(QWidget* pParent, const ImageInfo& imageInfo) : QFrame(pParent), m_imageInfo(imageInfo)
{
    QVBoxLayout* pLayout (new QVBoxLayout(this));
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(4);
    pLayout->addStretch();

    const int BTN_SIZE (64);
    const int ICON_SIZE (BTN_SIZE - 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth) - 2); //ttt2 not sure PM_DefaultFrameWidth is right

    QToolButton* pBtn (new QToolButton(this));
    pBtn->setIcon(imageInfo.getPixmap(ICON_SIZE));
    pBtn->setMaximumSize(BTN_SIZE, BTN_SIZE);
    pBtn->setMinimumSize(BTN_SIZE, BTN_SIZE);
    pBtn->setAutoRaise(true);
    pBtn->setToolTip("Click to see larger image");

    pBtn->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    pLayout->addWidget(pBtn, 0, Qt::AlignHCenter);

    QLabel* pInfoLabel (new QLabel("<error>", this));
    pInfoLabel->setText(m_imageInfo.getTextDescr());
    pInfoLabel->setAlignment(Qt::AlignHCenter);
    pLayout->addWidget(pInfoLabel, 0, Qt::AlignHCenter);

    //setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    connect(pBtn, SIGNAL(clicked(bool)), this, SLOT(onShowFull()));
}


void ImageInfoPanel::onShowFull()
{
    m_imageInfo.showFull(this);
}

