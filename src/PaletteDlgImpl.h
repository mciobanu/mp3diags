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


#ifndef PaletteDlgImplH
#define PaletteDlgImplH

#include  <vector>

#include  <QDialog>

#include  "ui_Palette.h"

#include  "CommonData.h"


class PaletteDlgImpl : public QDialog, private Ui::PaletteDlg
{
    Q_OBJECT

    std::vector<QToolButton*> m_vpColButtons;

    void setBtnColor(int n);
    void onButtonClicked(int n);

    CommonData* m_pCommonData;

public:
    PaletteDlgImpl(CommonData* pCommonData, QWidget* pParent);
    ~PaletteDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pOkB_clicked();

    void on_m_pCol0B_clicked() { onButtonClicked(0); }
    void on_m_pCol1B_clicked() { onButtonClicked(1); }
    void on_m_pCol2B_clicked() { onButtonClicked(2); }
    void on_m_pCol3B_clicked() { onButtonClicked(3); }
    void on_m_pCol4B_clicked() { onButtonClicked(4); }
    void on_m_pCol5B_clicked() { onButtonClicked(5); }
    void on_m_pCol6B_clicked() { onButtonClicked(6); }

    void onHelp();
};

#endif // #ifndef PaletteDlgImplH

