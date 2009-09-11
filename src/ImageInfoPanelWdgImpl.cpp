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

#include  "ImageInfoPanelWdgImpl.h"

#include  "Helpers.h"
//#include  "Profiler.h"

using namespace std;
//using namespace pearl;

static const int IMG_SIZE (110);

ImageInfoPanelWdgImpl::ImageInfoPanelWdgImpl(QWidget* pParent, const TagWrtImageInfo& tagWrtImageInfo, int nPos) :
        QFrame(pParent, 0),
        Ui::ImageInfoPanelWdg(),
        m_tagWrtImageInfo(tagWrtImageInfo),
        m_nPos(nPos)
{
//PROF("ImageInfoPanelWdgImpl::ImageInfoPanelWdgImpl");
    CB_ASSERT (nPos >= 0 /*&& nPos < cSize(vImageInfo)*/);
    setupUi(this);
    m_pPosL->setText(QString("# %1").arg(nPos + 1));
    m_pSizeL->setText(QString("%1kB").arg(tagWrtImageInfo.m_imageInfo.getSize()/1024));

    m_pDimL->setText(QString("%1x%2").arg(tagWrtImageInfo.m_imageInfo.getWidth()).arg(tagWrtImageInfo.m_imageInfo.getHeight()));
//PROFD(4);
    m_pThumbL->setPixmap(tagWrtImageInfo.m_imageInfo.getPixmap(IMG_SIZE)); //ttt1p performance issue; doesn't look like much can be done, though
//PROFD(5);
    if (tagWrtImageInfo.m_sstrFiles.empty())
    {
        m_pEraseB->hide();
    }
    else
    {
        QString s;
        if (tagWrtImageInfo.m_sstrFiles.size() > 1)
        {
            s = "Erase these files:";
            for (set<string>::const_iterator it = tagWrtImageInfo.m_sstrFiles.begin(); it != tagWrtImageInfo.m_sstrFiles.end(); ++it)
            {
                s += "\n" + convStr(*it);
            }
        }
        else
        {
            s = "Erase " + convStr(*tagWrtImageInfo.m_sstrFiles.begin());
        }

        m_pEraseB->setToolTip(s);
    }
}

ImageInfoPanelWdgImpl::~ImageInfoPanelWdgImpl()
{
}

void ImageInfoPanelWdgImpl::on_m_pFullB_clicked()
{
    QDialog dlg (this, getNoResizeWndFlags());

    QGridLayout* pGridLayout (new QGridLayout(&dlg));
    dlg.setLayout(pGridLayout);
    QLabel* p (new QLabel(&dlg));
    pGridLayout->addWidget(p);

    p->setPixmap(m_tagWrtImageInfo.m_imageInfo.getPixmap()); //ttt1 see if it should limit size (IIRC QLabel scaled down once a big image)

    dlg.exec();
}

void ImageInfoPanelWdgImpl::setNormalBackground()
{
    QPalette pal;
    setPalette(pal);
}


void ImageInfoPanelWdgImpl::setHighlightBackground()
{
    QPalette pal;
    //QPalette pal (palette());
    //QPalette pal (QColor(255, 220, 190));
    pal.setColor(QPalette::Window, pal.color(QPalette::Disabled, QPalette::Dark));

    setPalette(pal);
}




