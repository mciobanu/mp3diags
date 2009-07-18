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


#include  <QColorDialog>
#include  <QPainter>

#include  "PaletteDlgImpl.h"

#include  "TagEditorDlgImpl.h"
#include  "Helpers.h"

PaletteDlgImpl::PaletteDlgImpl(CommonData* pCommonData, QWidget* pParent) : QDialog(pParent, getNoResizeWndFlags()), Ui::PaletteDlg(), m_pCommonData(pCommonData) // not a "thread window", but doesn't need resizing anyway
{
    setupUi(this);

    m_vpColButtons.push_back(m_pCol0B);
    m_vpColButtons.push_back(m_pCol1B);
    m_vpColButtons.push_back(m_pCol2B);
    m_vpColButtons.push_back(m_pCol3B);
    m_vpColButtons.push_back(m_pCol4B);
    m_vpColButtons.push_back(m_pCol5B);
    m_vpColButtons.push_back(m_pCol6B);

    for (int i = 0; i < cSize(m_vpColButtons); ++i)
    {
        setBtnColor(i);
    }

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
}

PaletteDlgImpl::~PaletteDlgImpl()
{
}

/*$SPECIALIZATION$*/

void PaletteDlgImpl::on_m_pOkB_clicked()
{
    accept();
}


void PaletteDlgImpl::setBtnColor(int n)
{
    /*QPalette pal (m_vpColButtons[n]->palette());
    //QPalette pal (m_pCol0B->palette());
    pal.setBrush(QPalette::Button, m_pCommonData->m_vTagEdtColors.at(n));
    pal.setBrush(QPalette::Window, m_pCommonData->m_vTagEdtColors[n]);
    //pal.setBrush(QPalette::Midlight, QColor(255, 0, 0));
    //pal.setBrush(QPalette::Dark, QColor(255, 0, 0));
    //pal.setBrush(QPalette::Mid, QColor(255, 0, 0));
    //pal.setBrush(QPalette::Shadow, QColor(255, 0, 0));
    m_vpColButtons[n]->setPalette(pal);*/

    int f (QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, m_vpColButtons.at(n)) + 2); //ttt2 hard-coded "2"
    int w (m_vpColButtons[n]->width() - f), h (m_vpColButtons[n]->height() - f);
    QPixmap pic (w, h);
    QPainter pntr (&pic);
    pntr.fillRect(0, 0, w, h, m_pCommonData->m_vTagEdtColors.at(n));

    m_vpColButtons[n]->setIcon(pic);
    m_vpColButtons[n]->setIconSize(QSize(w, h));

/*    QPixmap pic (21, 21);
    QPainter pntr (&pic);
    QRect r (0, 0, 21, 21);
    pntr.fillRect(r, QColor(255, 255, 0));
    pntr.drawText(r, Qt::AlignCenter, "i");

    m_vpColButtons[n]->setIcon(pic);
    m_vpColButtons[n]->setIconSize(QSize(21, 21));*/
}

void PaletteDlgImpl::onButtonClicked(int n)
{
    QColor c (QColorDialog::getColor(m_pCommonData->m_vTagEdtColors.at(n), this));
    if (!c.isValid()) { return; }
    m_pCommonData->m_vTagEdtColors[n] = c;
    setBtnColor(n);
}



void PaletteDlgImpl::onHelp()
{
    //openHelp("index.html");
}

