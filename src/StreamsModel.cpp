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


#include  <sstream>

#include  <QPainter>
#include  <QTableView>

#include  "StreamsModel.h"

#include  "NotesModel.h"
#include  "FilesModel.h"
#include  "CommonData.h"
#include  "DataStream.h"

using namespace std;

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

StreamsModel::StreamsModel(CommonData* pCommonData) : QAbstractTableModel(pCommonData->m_pStreamsG), m_pCommonData(pCommonData)
{
}


/*override*/ int StreamsModel::rowCount(const QModelIndex&) const
{
    return cSize(m_pCommonData->getCrtStreams());
}

/*override*/ int StreamsModel::columnCount(const QModelIndex&) const
{
    return 5;
}



/*override*/ QVariant StreamsModel::data(const QModelIndex& index, int nRole) const
{
LAST_STEP("StreamsModel::data()");

/*
//ttt2

Excessive calls to this from MultiLineTvDelegate::sizeHint() might make applying a filter pretty slow. To reproduce:

1) The first file that will get selected by the filter should be badly broken, having 1000 streams.
2) Apply a filter by a note of that file

The issue is that StreamsModel::data() gets called way too many times from MultiLineTvDelegate::sizeHint(). So even if a single call is pretty quick (not leaving room to much improvement), the total time is noticeable.

Since this is a pathologic case and even then it's only a matter of several seconds, the fix can wait. (One idea would be to report that all lines above 200 have a height of one line, but perhaps there's something better.)

static int CRT (0);
++CRT;
if (0 == CRT % 2000)
{
    qDebug("StreamsModel::data() call %d", CRT);
}
*/

    int j (index.column());
    //if (nRole == Qt::SizeHintRole && j > 0) { return QSize(CELL_WIDTH - 1, CELL_HEIGHT - 1); }  // !!! "-1" so one pixel can be used to draw the grid
    //if (nRole == Qt::SizeHintRole) { return QSize(CELL_WIDTH - 10, CELL_HEIGHT - 10); }  // !!! "-1" so one pixel can be used to draw the grid

    if (!index.isValid() || nRole != Qt::DisplayRole) { return QVariant(); }

    const DataStream* pDataStream (m_pCommonData->getCrtStreams()[index.row()]);
    switch (j)
    {
    case 0:
        {
            ostringstream out;
            //out << "0x" << hex << setw(8) << setfill('0') << pDataStream->getPos();
            out << "0x" << hex << pDataStream->getPos();
            return convStr(out.str());
        }
    case 1:
        {
            ostringstream out;
            out << pDataStream->getSize();
            return convStr(out.str());
        }
    case 2:
        {
            ostringstream out;
            out << "0x" << hex << pDataStream->getSize();
            return convStr(out.str());
        }
    case 3:
        {
            return QString(pDataStream->getTranslatedDisplayName());
        }
    case 4: return convStr(pDataStream->getInfo());
    default: CB_ASSERT(false);
    }//out << "Ape: " <<" offset=0x" << hex << m_pos << ", size=0x" << m_nSize << dec << " (" << m_nSize << ")";
}

/*override*/ QVariant StreamsModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /* = Qt::DisplayRole*/) const
{
LAST_STEP("StreamsModel::headerData");
    if (nRole == Qt::SizeHintRole)
    {
        return getNumVertHdrSize(cSize(m_pCommonData->getCrtStreams()), eOrientation);
    }

    if (nRole != Qt::DisplayRole) { return QVariant(); }
    if (Qt::Horizontal == eOrientation)
    {
        //return section;
        switch (nSection)
        {
        case 0: return tr("Address");
        case 1: return tr("Size (dec)");
        case 2: return tr("Size (hex)");
        case 3: return tr("Type");
        case 4: return tr("Stream details");
        default:
            CB_ASSERT (false);
        }
    }

    return nSection + 1;
}


static bool containsAnyOf(const DataStream* pStream, const set<streampos>& sSel)
{
    streampos begPos (pStream->getPos());
    streampos endPos (begPos);
    endPos += pStream->getSize();
    for (set<streampos>::const_iterator it = sSel.begin(), end = sSel.end(); it != end; ++it) //ttt2 binary search
    {
        streampos pos (*it);
        if (begPos <= pos && pos < endPos)
        {
            return true;
        }
    }
    return false;
}

void StreamsModel::matchSelToNotes()
{
    int nCrt (m_pCommonData->getFilesGCrtRow());
    if (-1 == nCrt) { return; }

    set<streampos> sSel;
    QModelIndexList lSelNotes (m_pCommonData->m_pNotesG->selectionModel()->selection().indexes());

    QItemSelectionModel* pStreamsSelModel (m_pCommonData->m_pStreamsG->selectionModel());
    pStreamsSelModel->clearSelection();
    for (QModelIndexList::iterator it = lSelNotes.begin(), end = lSelNotes.end(); it != end; ++it)
    {
        int nCol (it->column());
        if (0 == nCol) // it's "whole row selection", so use only the first column
        {
            sSel.insert(m_pCommonData->getCrtNotes()[it->row()]->getPos());
        }
    }

    const vector<DataStream*>& vCrtStreams (m_pCommonData->getViewHandlers()[nCrt]->getStreams());
    bool bFirstFound (false);

    for (int i = 0, n = cSize(vCrtStreams); i < n; ++i)
    {
        const DataStream* pStream (vCrtStreams[i]);
        if (containsAnyOf(pStream, sSel))
        {
            if (!bFirstFound)
            {
                bFirstFound = true;
                m_pCommonData->m_pStreamsG->setCurrentIndex(index(i, 0));
            }
            pStreamsSelModel->select(index(i, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
}


void StreamsModel::onStreamsGSelChanged()
{
    NonblockingGuard g (m_pCommonData->m_bChangeGuard);
    if (!g) { return; }

    m_pCommonData->m_pNotesModel->matchSelToStreams();
    m_pCommonData->m_pFilesModel->matchSelToNotes();
}




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


StreamsGDelegate::StreamsGDelegate(CommonData* pCommonData) : MultiLineTvDelegate(pCommonData->m_pStreamsG), m_pCommonData(pCommonData)
{
}


/*override*/ void StreamsGDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    pPainter->save();

    QStyleOptionViewItemV2 myOption (option);

    if (0 == index.column() || 1 == index.column() || 2 == index.column())
    {
        myOption.displayAlignment |= Qt::AlignRight;
        myOption.font = m_pCommonData->getFixedFont();
    }

    MultiLineTvDelegate::paint(pPainter, myOption, index);

    pPainter->restore();
}



