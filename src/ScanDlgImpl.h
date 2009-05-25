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



#ifndef ScanDlgImplH
#define ScanDlgImplH

#include <QDialog>

#include "ui_Scan.h"

class CommonData;
class CheckedDirModel;

class ScanDlgImpl : public QDialog, private Ui::ScanDlg
{
    Q_OBJECT

    CommonData* m_pCommonData;
    CheckedDirModel* m_pDirModel;
public:
    ScanDlgImpl(QWidget* pParent, CommonData* pCommonData);
    bool run(bool& bForce); // if returning true, it also calls CommonData::setDirs()
    ~ScanDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pScanB_clicked() { accept(); }
    void on_m_pCancelB_clicked() { reject(); }
    void onShow();
};

#endif



