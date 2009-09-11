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


#ifndef DebugDlgImplH
#define DebugDlgImplH

#include  <string>

#include  <QDialog>

#include  "ui_Debug.h"


class CommonData;
class QSettings;
class LogModel;

class DebugDlgImpl : public QDialog, private Ui::DebugDlg
{
    Q_OBJECT

    CommonData* m_pCommonData;

    void exportAsText(const std::string& strFileName);
    void exportLog(const std::string& strFileName);

    LogModel* m_pLogModel;

public:
    DebugDlgImpl(QWidget* pParent, CommonData* pCommonData);
    ~DebugDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    void run();

public slots:
    /*$PUBLIC_SLOTS$*/

    void on_m_pSaveFileInfoB_clicked();
    void on_m_pSaveLogB_clicked();
    void on_m_pTst01B_clicked();
    void on_m_pDecodeMpegFrameB_clicked();

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pCloseB_clicked();

    void onHelp();
};

#endif

