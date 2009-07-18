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


#ifndef NoteFilterDlgImplH
#define NoteFilterDlgImplH

#include  <QDialog>

#include  "ui_NoteFilter.h"

#include  "DoubleList.h"

class Note;
class CommonData;

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


class NoteListElem : public ListElem
{
    /*override*/ std::string getText(int nCol) const;
    const Note* m_pNote; // doesn't own the pointer
    CommonData* m_pCommonData;
public:
    NoteListElem(const Note* pNote, CommonData* pCommonData) : m_pNote(pNote), m_pCommonData(pCommonData) {}
    const Note* getNote() const { return m_pNote; }
    const CommonData* getCommonData() const { return m_pCommonData; }
};


class NoteListPainterBase : public ListPainter
{
    /*override*/ int getColCount() const { return 2; }
    /*override*/ std::string getColTitle(int nCol) const;
    /*override*/ int getColWidth(int nCol) const; // positive values are used for fixed widths, while negative ones are for "stretched"
    /*override*/ int getHdrHeight() const;
    /*override*/ Qt::Alignment getAlignment(int nCol) const;
    /*override*/ void getColor(int nIndex, int nColumn, bool bSubList, QColor& bckgColor, QColor& penColor, double& dGradStart, double& dGradEnd) const;
    mutable std::vector<const Note*> m_vpAvail, m_vpSel;
protected:
    CommonData* m_pCommonData;
public:
    NoteListPainterBase(CommonData* pCommonData, const std::string& strNothingSel) : ListPainter(strNothingSel), m_pCommonData(pCommonData) {}
};



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================




class NoteFilterDlgImpl : public QDialog, private Ui::NoteFilterDlg, public NoteListPainterBase
{
    Q_OBJECT

    void logState(const char* szPlace) const;

    /*override*/ std::string getTooltip(TooltipKey eTooltipKey) const;
    /*override*/ void reset();

public:
    /*$PUBLIC_FUNCTIONS$*/
    NoteFilterDlgImpl(CommonData* pCommonData, QWidget *pParent);
    ~NoteFilterDlgImpl();

public slots:
    /*$PUBLIC_SLOTS$*/
    void on_m_pOkB_clicked();
    void on_m_pCancelB_clicked();

    void onAvlDoubleClicked(int nRow);

    void onHelp();
};

#endif // #ifndef NoteFilterDlgImplH

