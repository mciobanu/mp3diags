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

#include  "NotesModel.h"

#include  "FilesModel.h"
#include  "StreamsModel.h"
#include  "DataStream.h"
#include  "CommonData.h"


using namespace std;

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


NotesModel::NotesModel(CommonData* pCommonData) : QAbstractTableModel(pCommonData->m_pNotesG), m_pCommonData(pCommonData) {}

/*override*/ int NotesModel::rowCount(const QModelIndex&) const
{
    return cSize(m_pCommonData->getCrtNotes());
}

/*override*/ int NotesModel::columnCount(const QModelIndex&) const
{
    return 3;
}



/*override*/ QVariant NotesModel::data(const QModelIndex& index, int nRole) const
{
LAST_STEP("NotesModel::data()");
    int j (index.column());
    //if (nRole == Qt::SizeHintRole && j > 0) { return QSize(CELL_WIDTH - 1, CELL_HEIGHT - 1); }  // !!! "-1" so one pixel can be used to draw the grid
    //if (nRole == Qt::SizeHintRole) { return QSize(CELL_WIDTH - 10, CELL_HEIGHT - 10); }  // !!! "-1" so one pixel can be used to draw the grid

    if (!index.isValid() || nRole != Qt::DisplayRole) { return QVariant(); }

    int nRow (index.row());
    int nSize (cSize(m_pCommonData->getCrtNotes()));
    CB_ASSERT (nRow >= 0 || nRow < nSize);

    const Note* pNote (m_pCommonData->getCrtNotes()[nRow]);
//printf("Note at %p\n", pNote);
    switch (j)
    {
    case 0: return getNoteLabel(pNote);
    case 1:
        {
            const string& s (pNote->getDetail());
            if (s.empty()) { return pNote->getDescription(); }
            return convStr(s);
        }
    case 2: return convStr(pNote->getPosHex());
        //return "mmmiiiWWWlll";
        //return "ABCDEFGHIJKLM";
    default: CB_ASSERT(false);
    }
}

/*override*/ QVariant NotesModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /*= Qt::DisplayRole*/) const
{
LAST_STEP("NotesModel::headerData");
    if (nRole == Qt::SizeHintRole)
    {
        /*QVariant v (QAbstractTableModel::headerData(nSection, eOrientation, nRole)); // !!! doesn't work because QAbstractTableModel::headerData always returns an invalid value
        [...] */
        return getNumVertHdrSize(cSize(m_pCommonData->getCrtNotes()), eOrientation);
    }

    if (nRole != Qt::DisplayRole) { return QVariant(); }
    if (Qt::Horizontal == eOrientation)
    {
        switch (nSection)
        {
        case 0: return "L";
        case 1: return "Note";
        case 2: return "Address";
        default:
            CB_ASSERT (false);
        }
    }

    return nSection + 1;
}


void NotesModel::matchSelToMain()
{
    QModelIndexList lSelFiles (m_pCommonData->m_pFilesG->selectionModel()->selection().indexes());

    set<int> sSel;
    for (QModelIndexList::iterator it = lSelFiles.begin(), end = lSelFiles.end(); it != end; ++it)
    {
        int nCol (it->column());
        if (nCol > 0) // skip file name
        {
            sSel.insert(nCol - 1);
        }
    }

    QItemSelectionModel* pNotesSelModel (m_pCommonData->m_pNotesG->selectionModel());
    pNotesSelModel->clearSelection();
    bool bFirstFound (false);
    for (int i = 0, nNoteCnt = cSize(m_pCommonData->getCrtNotes()); i < nNoteCnt; ++i)
    {
        const Note* pNote (m_pCommonData->getCrtNotes()[i]);
        int nPos (m_pCommonData->findPos(pNote));
        if (sSel.count(nPos) > 0)
        {
            if (!bFirstFound)
            {
                bFirstFound = true;
                m_pCommonData->m_pNotesG->setCurrentIndex(index(i, 0));
            }
            pNotesSelModel->select(index(i, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
}


void NotesModel::onNotesGSelChanged()
{
    NonblockingGuard g (m_pCommonData->m_bChangeGuard);
    if (!g) { return; }

    m_pCommonData->m_pFilesModel->matchSelToNotes();
    m_pCommonData->m_pStreamsModel->matchSelToNotes();
}

static bool isInList(const Note* pNote, vector<const DataStream*>& vStreams)
{
    if (-1 == pNote->getPos()) { return false; }
    for (int i = 0, n = cSize(vStreams); i < n; ++i) // ttt2 use a binary search; the streams are sorted;
    {
        if (pNote->getPos() >= vStreams[i]->getPos() && pNote->getPos() < vStreams[i]->getPos() + vStreams[i]->getSize())
        {
//cout << "accepted " << pNote->getPosHex() << " " << pNote->getDescription() << endl;
            return true;
        }
    }
    return false;
}



void NotesModel::matchSelToStreams()
{
    QModelIndexList lSelStreams (m_pCommonData->m_pStreamsG->selectionModel()->selection().indexes());

    const vector<DataStream*>& vCrtStreams (m_pCommonData->getViewHandlers()[m_pCommonData->getFilesGCrtRow()]->getStreams());
    vector<const DataStream*> vSelStreams;
    for (QModelIndexList::iterator it = lSelStreams.begin(), end = lSelStreams.end(); it != end; ++it)
    {
        int nRow (it->row());
        vSelStreams.push_back(vCrtStreams[nRow]);
    }

    QItemSelectionModel* pNotesSelModel (m_pCommonData->m_pNotesG->selectionModel());
    pNotesSelModel->clearSelection();
    bool bFirstFound (false);

    for (int i = 0, nNoteCnt = cSize(m_pCommonData->getCrtNotes()); i < nNoteCnt; ++i)
    {
        const Note* pNote (m_pCommonData->getCrtNotes()[i]);
        if (isInList(pNote, vSelStreams))
        {
            if (!bFirstFound)
            {
                bFirstFound = true;
                m_pCommonData->m_pNotesG->setCurrentIndex(index(i, 0));
            }
            pNotesSelModel->select(index(i, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


NotesGDelegate::NotesGDelegate(CommonData* pCommonData) : MultiLineTvDelegate(pCommonData->m_pNotesG), m_pCommonData(pCommonData)
{
}


/*override*/ void NotesGDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int nRow (index.row());
//f1();
    pPainter->save();

    const Note* pNote (m_pCommonData->getCrtNotes()[nRow]);
    QStyleOptionViewItemV2 myOption (option);

    //myOption.palette.setColor(QPalette::Base, pal.color(QPalette::Disabled, QPalette::Window)); // !!! the palette doesn't matter; fillRect() should be called
    QColor col;
    double d1, d2;
    m_pCommonData->getNoteColor(*pNote, vector<const Note*>(), col, d1, d2);
    pPainter->fillRect(myOption.rect, QBrush(col));

    if (0 == index.column())
    {
        myOption.displayAlignment |= Qt::AlignHCenter;
        //myOption.font = m_pCommonData->getGeneralFont();
        //myOption.font.setPixelSize(9);
        if (Note::ERR == pNote->getSeverity())
        {
            myOption.palette.setColor(QPalette::Text, ERROR_PEN_COLOR());
        }
        else if (Note::SUPPORT == pNote->getSeverity())
        {
            myOption.palette.setColor(QPalette::Text, SUPPORT_PEN_COLOR());
        }
    }

    if (2 == index.column())
    {
        myOption.displayAlignment |= Qt::AlignRight;
        myOption.font = m_pCommonData->getFixedFont();
    }

    //myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
#if 0

    //myOption.palette.setColor(QPalette::Base, pal.color(QPalette::Disabled, QPalette::Window));

    QString qstrText (index.model()->data(index, Qt::DisplayRole).toString());
    //qstrText = "QQQ";

    drawDisplay(pPainter, myOption, myOption.rect, qstrText); // ttt3 see why selecting cells in column 0 or 1 doesn't make column 2 selected unless there's an address for the note;
    drawFocus(pPainter, myOption, myOption.rect);
#else

    MultiLineTvDelegate::paint(pPainter, myOption, index);
#endif

    pPainter->restore();
}





