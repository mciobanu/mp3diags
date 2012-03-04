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
#include  <QTimer>

#include  "TagReadPanel.h"

#include  "DataStream.h"
#include  "Helpers.h"
#include  "Widgets.h"


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
        pTable->setVerticalHeader(new NoCropHeaderView(pTable));

        pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        pTable->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        pTable->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);
        pTable->horizontalHeader()->setStretchLastSection(true);
        const int ROW_CNT (9);
        pTable->setRowCount(ROW_CNT);
        pTable->setColumnCount(1);
        QStringList lLabels;
        //lLabels << "Title" << "Artist" << "Track#" << "Time" << "Genre" << "Composer" << "Album" << "VA" << "Rating";
        lLabels << tr("Track#") << tr("Artist") << tr("Title") << tr("Album") << tr("VA") << tr("Time") << tr("Genre") << tr("Rating") << tr("Composer");
        pTable->setVerticalHeaderLabels(lLabels);

        pTable->horizontalHeader()->hide();
        //pTable->setMinimumHeight(300);
        int nHeight (CELL_HEIGHT*ROW_CNT + 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, pTable));

        pTable->setMaximumHeight(nHeight);
        pTable->setMinimumHeight(nHeight);
        //pTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        pTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        //pTable->setFrameShape(QFrame::NoFrame);

        QTableWidgetItem* pItem;

        pItem = new QTableWidgetItem(); // !!! from doc: The table takes ownership of the item.
        switch (pTagReader->getSupport(TagReader::TRACK_NUMBER))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getTrackNumber()));
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
        switch (pTagReader->getSupport(TagReader::TITLE))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getTitle()));
        }
        pTable->setItem(2, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::ALBUM))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getAlbumName()));
        }
        pTable->setItem(3, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::VARIOUS_ARTISTS))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getValue(TagReader::VARIOUS_ARTISTS)));
        }
        pTable->setItem(4, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::TIME))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default:
            pItem->setText(pTagReader->getTime().asString());
        }
        pTable->setItem(5, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::GENRE))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getGenre()));
        }
        pTable->setItem(6, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::RATING))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default:
            {
                double d (pTagReader->getRating());
                if (d >= 0)
                {
                    pItem->setText(QString().sprintf("%1.1f", d));
                }
            }
        }
        pTable->setItem(7, 0, pItem);

        pItem = new QTableWidgetItem();
        switch (pTagReader->getSupport(TagReader::COMPOSER))
        {
        case TagReader::NOT_SUPPORTED: pItem->setBackground(gray); break;
        default: pItem->setText(convStr(pTagReader->getComposer()));
        }
        pTable->setItem(8, 0, pItem);

        pLayout->addWidget(pTable);

        //ttt2 Oxygen shows regular cells under mouse with diferent background color
    }

    {
        // ttt2 perhaps use QScrollArea
        vector<ImageInfo> vImg (pTagReader->getImages());
        m_pImgWidget = 0;
        QHBoxLayout* pImgWidgetLayout (0);

        for (int i = 0; i < cSize(vImg); ++i)
        {
            const ImageInfo& img (vImg[i]);

            if (0 == m_pImgWidget)
            {
                m_pImgWidget = new QWidget (this);
                pImgWidgetLayout = new QHBoxLayout(m_pImgWidget);
                pImgWidgetLayout->setContentsMargins(0, 0, 0, 0);
                //pImgWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                m_pImgWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                //pImgWidget->setMaximumHeight(40);
            }

            ImageInfoPanel* p (new ImageInfoPanel (this, img));
            m_vpImgPanels.push_back(p);

            pImgWidgetLayout->addWidget(p);
        }


        if (0 != m_pImgWidget)
        {
            pImgWidgetLayout->addStretch();
            pLayout->addWidget(m_pImgWidget);
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

    if (0 != m_pImgWidget)
    {
        QTimer::singleShot(1, this, SLOT(onCheckSize()));
    }
}


void TagReadPanel::onCheckSize()
{
    QSize hint (m_pImgWidget->sizeHint());
    QSize act (m_pImgWidget->size());
    //qDebug("pImgWidget %dx%d  %dx%d", hint.width(), hint.height(), act.width(), act.height());
    if (hint.width() > act.width())
    {
        int n (cSize(m_vpImgPanels));
        int nNewSize ((act.width() + m_pImgWidget->layout()->spacing()) / n - m_pImgWidget->layout()->spacing() - 8); //ttt2 hard-coded "8"; it's for the frame drawn around auto-raise buttons; minimum value that works is 6, but not sure where to get it from; using 8 just in case)
        if (nNewSize > 64) { nNewSize = 64; }
        //qDebug("new sz %d", nNewSize);
        for (int i = 0; i < n; ++i)
        {
            m_vpImgPanels[i]->resize(nNewSize);
        }
    }
}



ImageInfoPanel::ImageInfoPanel(QWidget* pParent, const ImageInfo& imageInfo) : QFrame(pParent), m_imageInfo(imageInfo) //ttt1 name conflict with ImageInfoPanelWdgImpl
{
    QVBoxLayout* pLayout (new QVBoxLayout(this));
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(4);
    pLayout->addStretch();

    createButton(64);

    m_pInfoLabel = new QLabel(tr("<error>"), this);
    m_pInfoLabel->setText(m_imageInfo.getTextDescr());
    m_pInfoLabel->setAlignment(Qt::AlignHCenter);
    pLayout->addWidget(m_pInfoLabel, 0, Qt::AlignHCenter);

    //setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}


void ImageInfoPanel::onShowFull()
{
    m_imageInfo.showFull(this);
}


void ImageInfoPanel::createButton(int nSize)
{
    QVBoxLayout* pLayout (dynamic_cast<QVBoxLayout*>(layout()));

    const int BTN_SIZE (nSize);
    const int ICON_SIZE (BTN_SIZE - 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth) - 2); //ttt2 not sure PM_DefaultFrameWidth is right

    m_pBtn = new QToolButton(this);
    m_pBtn->setIcon(QPixmap::fromImage(m_imageInfo.getImage(ICON_SIZE)));
    m_pBtn->setMaximumSize(BTN_SIZE, BTN_SIZE);
    m_pBtn->setMinimumSize(BTN_SIZE, BTN_SIZE);
    m_pBtn->setAutoRaise(true);
    m_pBtn->setToolTip(tr("Click to see larger image"));

    m_pBtn->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    pLayout->addWidget(m_pBtn, 0, Qt::AlignHCenter);

    connect(m_pBtn, SIGNAL(clicked(bool)), this, SLOT(onShowFull()));
}


void ImageInfoPanel::resize(int nSize)
{
    delete m_pBtn;
    delete m_pInfoLabel;

    createButton(nSize);

    QVBoxLayout* pLayout (dynamic_cast<QVBoxLayout*>(layout()));

    // ttt2 see why is this needed (otherwise the button isn't shown)
    m_pInfoLabel = new QLabel("", this);
    m_pInfoLabel->setMaximumSize(100, 1);
    pLayout->addWidget(m_pInfoLabel, 0, Qt::AlignHCenter);
    pLayout->setSpacing(0);
}

