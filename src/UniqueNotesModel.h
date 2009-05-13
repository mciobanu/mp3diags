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


#ifndef UniqueNotesModelH
#define UniqueNotesModelH

#include  <QAbstractTableModel>

#include  "MultiLineTvDelegate.h"

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


class CommonData;

// all notes
struct UniqueNotesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    UniqueNotesModel(CommonData* pCommonData);

    /*override*/ int rowCount(const QModelIndex&) const;
    /*override*/ int columnCount(const QModelIndex&) const;
    /*override*/ QVariant data(const QModelIndex&, int) const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;

    CommonData* m_pCommonData;

    void selectTopLeft(); // makes current and selects the element in the top-left corner and emits a change signal regardless of the element that was selected before; makes current the default invalid index (-1,-1) if the table is empty
};


class UniqueNotesGDelegate : public MultiLineTvDelegate
{
    Q_OBJECT

public:
    UniqueNotesGDelegate(CommonData* pCommonData);

    /*override*/ void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    CommonData* m_pCommonData;
};



#endif // #ifndef UniqueNotesModelH

