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


#ifndef DirFilterDlgImplH
#define DirFilterDlgImplH


#include  "ui_DirFilter.h"

#include  "DoubleList.h"

class CommonData;

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================



class DirFilterDlgImpl : public QDialog, private Ui::DirFilterDlg, public ListPainter
{
Q_OBJECT
    CommonData* m_pCommonData;

    void logState(const char* szPlace) const;


    void populateLists();

    /*override*/ int getColCount() const { return 1; }
    /*override*/ std::string getColTitle(int /*nCol*/) const { return "Folder"; }
    /*override*/ void getColor(int /*nIndex*/, int /*nColumn*/, bool /*bSubList*/, QColor& /*bckgColor*/, QColor& /*penColor*/, double& /*dGradStart*/, double& /*dGradEnd*/) const { }
    /*override*/ int getColWidth(int /*nCol*/) const { return -1; } // positive values are used for fixed widths, while negative ones are for "stretched"
    /*override*/ int getHdrHeight() const;
    /*override*/ std::string getTooltip(TooltipKey eTooltipKey) const;
    /*override*/ Qt::Alignment getAlignment(int nCol) const;
    /*override*/ void reset();

public:
    DirFilterDlgImpl(CommonData* pCommonData, QWidget *pParent = 0);
    ~DirFilterDlgImpl();

private slots:
    void on_m_pOkB_clicked();
    void on_m_pCancelB_clicked();

    void onAvlDoubleClicked(int nRow);

    void onHelp();
};

#endif // #ifndef DirFilterDlgImplH
