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


#include  <QDialog>
#include  <QLabel>
#include  <QVBoxLayout>
#include  <QAction>
#include  <QClipboard>
#include  <QApplication>
#include  <QMimeData>

#include  "FullSizeImgDlg.h"

#include  "Helpers.h"
#include  "DataStream.h" // for translations



FullSizeImgDlg::FullSizeImgDlg(QWidget* pParent, const ImageInfo& imageInfo) : QDialog(pParent, getNoResizeWndFlags()), m_imageInfo(imageInfo)
{
    QVBoxLayout* pLayout (new QVBoxLayout(this));
    //dlg.setLayout(pGridLayout);
    QLabel* p (new QLabel(this));
    p->setPixmap(QPixmap::fromImage(imageInfo.getImage())); //ttt2 see if it should limit size (IIRC QLabel scaled down once a big image)
    pLayout->addWidget(p, 0, Qt::AlignHCenter);


    QString s;
    s.sprintf("%dx%d / %dkB\n", imageInfo.getWidth(), imageInfo.getHeight(), imageInfo.getSize()/1024);
    s += TagReader::tr(imageInfo.getImageType());

    p = new QLabel(s, this);
    p->setAlignment(Qt::AlignHCenter);
    pLayout->addWidget(p, 0, Qt::AlignHCenter);

    { QAction* pAct (new QAction(this)); pAct->setShortcut(QKeySequence("Ctrl+C")); connect(pAct, SIGNAL(triggered()), this, SLOT(onCopy())); addAction(pAct); }
}


void FullSizeImgDlg::onCopy()
{
#ifndef WIN32
    QByteArray ba (m_imageInfo.getComprData(), m_imageInfo.getSize());
    QMimeData* pMimeData (new QMimeData());
    const char* szFmt (0);
    switch (m_imageInfo.getCompr())
    {
    case ImageInfo::JPG: szFmt = "image/jpeg"; break;
    case ImageInfo::PNG: szFmt = "image/png"; break; //ttt2 doesn't work for BMP, GIF, ...
    default: break;
    }

    if (0 != szFmt)
    {
        pMimeData->setData(szFmt, ba);
        QApplication::clipboard()->setMimeData(pMimeData);
    }
    else
    {
        delete pMimeData;
    }
#else
    QApplication::clipboard()->setPixmap(QPixmap::fromImage(m_imageInfo.getImage()));
#endif
}


