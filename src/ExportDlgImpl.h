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



#ifndef EXPORTDLGIMPL_H
#define EXPORTDLGIMPL_H


#include <vector>

#include <QDialog>

#include "ui_Export.h"

class Mp3Handler;


class ExportDlgImpl : public QDialog, private Ui::ExportDlg
{
    Q_OBJECT

    bool exportAsText(const std::string& strFileName);
    bool exportAsM3u(const std::string& strFileName);
    bool exportAsXml(const std::string& strFileName);

    void getHandlers(std::vector<const Mp3Handler*>& v);
    void setFormatBtn();

    void setExt(const char* szExt);

public:
    ExportDlgImpl(QWidget* pParent);
    ~ExportDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    void run();

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pCloseB_clicked() { accept(); }
    void on_m_pExportB_clicked();
    void on_m_pChooseFileB_clicked();

    void on_m_pXmlRB_clicked() { setExt("xml"); }
    void on_m_pM3uRB_clicked() { setExt("m3u"); }
    void on_m_pTextRB_clicked() { setExt("txt"); }
};

#endif

