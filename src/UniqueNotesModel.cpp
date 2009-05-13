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


#include  <QPainter>
#include  <QTableView>

#include  "UniqueNotesModel.h"

#include  "CommonData.h"


using namespace std;

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

UniqueNotesModel::UniqueNotesModel(CommonData* pCommonData) : QAbstractTableModel(pCommonData->m_pUniqueNotesG), m_pCommonData(pCommonData)
{
}


/*override*/ int UniqueNotesModel::rowCount(const QModelIndex&) const
{
    return m_pCommonData->getUniqueNotes().getFltCount();
}

/*override*/ int UniqueNotesModel::columnCount(const QModelIndex&) const
{
    return 2;
}



/*override*/ QVariant UniqueNotesModel::data(const QModelIndex& index, int nRole) const
{
    if (!index.isValid() || nRole != Qt::DisplayRole) { return QVariant(); }

    if (0 == index.column())
    {
        const vector<const Note*>& v (m_pCommonData->getUniqueNotes().getFltVec());
        return getNoteLabel(v[index.row()]);
    }

    return m_pCommonData->getUniqueNotes().getFlt(index.row())->getDescription();
}

/*override*/ QVariant UniqueNotesModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /*= Qt::DisplayRole*/) const
{
    if (nRole == Qt::SizeHintRole)
    {
        return getNumVertHdrSize(m_pCommonData->getUniqueNotes().getFltCount(), eOrientation);
    }

    if (nRole != Qt::DisplayRole) { return QVariant(); }
    if (Qt::Horizontal == eOrientation)
    {
        switch (nSection)
        {
        case 0: return "L";
        case 1: return "Note";
        default:
            CB_ASSERT (false);
        }
    }

    return nSection + 1;
}


void UniqueNotesModel::selectTopLeft()
{
    QItemSelectionModel* pSelModel (m_pCommonData->m_pUniqueNotesG->selectionModel());
    pSelModel->clear();

    emit layoutChanged();
    if (m_pCommonData->getUniqueNotes().getFltCount() > 0)
    {
        //pSelModel->select(index(0, 0), QItemSelectionModel::Current);
        //pSelModel->select(index(0, 0), QItemSelectionModel::Select);

        //m_pCommonData->printFilesCrt();
        m_pCommonData->m_pUniqueNotesG->setCurrentIndex(index(0, 0));
        //m_pCommonData->printFilesCrt();
    }
}




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


UniqueNotesGDelegate::UniqueNotesGDelegate(CommonData* pCommonData) : MultiLineTvDelegate(pCommonData->m_pUniqueNotesG), m_pCommonData(pCommonData)
{
}

/*override*/ void UniqueNotesGDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    pPainter->save();

    QStyleOptionViewItemV2 myOption (option);

    if (0 == index.column())
    {
        myOption.displayAlignment |= Qt::AlignHCenter;
    }

    const Note* pNote (m_pCommonData->getUniqueNotes().getFlt(index.row()));
    pPainter->fillRect(myOption.rect, QBrush(getNoteColor(*pNote)));

    MultiLineTvDelegate::paint(pPainter, myOption, index);

    pPainter->restore();
}

















