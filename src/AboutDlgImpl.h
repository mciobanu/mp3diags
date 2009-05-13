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


#ifndef AboutDlgImplH
#define AboutDlgImplH

#include  <QDialog>

#include  "ui_About.h"

class AboutDlgImpl : public QDialog, private Ui::AboutDlg
{
    Q_OBJECT

    void initText(QTextBrowser* p, const char* szFileName);
public:
    AboutDlgImpl(QWidget* pParent = 0);
    ~AboutDlgImpl();

protected slots:
    void on_m_pOkB_clicked() { accept(); }
};

#endif

