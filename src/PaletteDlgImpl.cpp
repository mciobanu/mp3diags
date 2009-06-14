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


#include  "PaletteDlgImpl.h"

#include  "TagEditorDlgImpl.h"
#include  "Helpers.h"

PaletteDlgImpl::PaletteDlgImpl(QWidget* pParent) : QDialog(pParent, getNoResizeWndFlags()), Ui::PaletteDlg() // not a "thread window", but doesn't need resizing anyway
{
    setupUi(this);

    QPalette pal (m_pAlbNormalF->palette());

    pal.setColor(QPalette::Window, TagEditorDlgImpl::ALBFILE_NORM_COLOR); m_pAlbNormalF->setPalette(pal); m_pFileNormalF->setPalette(pal);

    pal.setColor(QPalette::Window, TagEditorDlgImpl::ALB_NONID3V2_COLOR); m_pAlbNonId3V2F->setPalette(pal);
    pal.setColor(QPalette::Window, TagEditorDlgImpl::ALB_ASSIGNED_COLOR); m_pAlbAssignedF->setPalette(pal);

    pal.setColor(QPalette::Window, TagEditorDlgImpl::FILE_TAG_MISSING_COLOR); m_pFileNoTagF->setPalette(pal);
    pal.setColor(QPalette::Window, TagEditorDlgImpl::FILE_NA_COLOR); m_pFileNoFieldF->setPalette(pal);
    pal.setColor(QPalette::Window, TagEditorDlgImpl::FILE_NO_DATA_COLOR); m_pFileNoDataF->setPalette(pal);
}

PaletteDlgImpl::~PaletteDlgImpl()
{
}

/*$SPECIALIZATION$*/

void PaletteDlgImpl::on_m_pOkB_clicked()
{
    accept();
}

//ttt1 perhaps implement color chooser, by moving the colors to CommonData


