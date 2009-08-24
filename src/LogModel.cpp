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


#include  <QTableView>
#include  <QHeaderView>

#include  "LogModel.h"

#include  "CommonData.h"


using namespace std;


LogModel::LogModel(CommonData* pCommonData, QTableView* pLogG) : QAbstractTableModel(pLogG), m_pCommonData(pCommonData), m_pLogG(pLogG)
{
}


/*override*/ int LogModel::rowCount(const QModelIndex&) const
{
    //qDebug("size %d", cSize(m_pCommonData->getLog()));
    return cSize(m_pCommonData->getLog());
}

/*override*/ int LogModel::columnCount(const QModelIndex&) const
{
    return 1;
}



/*override*/ QVariant LogModel::data(const QModelIndex& index, int nRole) const
{
LAST_STEP("LogModel::data()");
    if (!index.isValid()) { return QVariant(); }

    if (nRole == Qt::ToolTipRole)
    {
        QString s (convStr(m_pCommonData->getLog()[index.row()]));

        //QFontMetrics fm (QApplication::fontMetrics());
        QFontMetrics fm (m_pLogG->fontMetrics()); // !!! no need to use "QApplication::fontMetrics()"
        int nWidth (fm.width(s));

        if (nWidth + 10 < m_pLogG->horizontalHeader()->sectionSize(0)) // ttt2 "10" is hard-coded
        {
            //return QVariant();
            return ""; // !!! with "return QVariant()" the previous tooltip remains until the cursor moves over another cell that has a tooltip
        }//*/

        return s;
    }

    if (nRole != Qt::DisplayRole) { return QVariant(); }

    return convStr(m_pCommonData->getLog()[index.row()]);
}

/*override*/ QVariant LogModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /*= Qt::DisplayRole*/) const
{
LAST_STEP("LogModel::headerData");
#if 0
    if (nRole == Qt::SizeHintRole)
    {
        /*QVariant v (QAbstractTableModel::headerData(nSection, eOrientation, nRole)); // !!! doesn't work because QAbstractTableModel::headerData always returns an invalid value
        [...] */

        if (eOrientation == Qt::Vertical)
        {
            /*QFontMetrics fm (m_pLogG->fontMetrics()); // ttt2 duplicate of FilesModel; move
            double d (1.01 + log10(double(m_pCommonData->getLog().size())));
            int n (d);
            QString s (n, QChar('9'));
            //QSize size (fm.boundingRect(r, Qt::AlignTop | Qt::TextWordWrap, s).size());
            int nWidth (fm.width(s));

            return QSize(nWidth + 10, CELL_HEIGHT); //ttt1 replace CELL_HEIGHT; ttt1 "10" hard-coded*/
            getNumVertHdrSize ...
        }


        return QVariant();
    }
#endif
    if (nRole != Qt::DisplayRole) { return QVariant(); }
    if (Qt::Horizontal == eOrientation)
    {
        switch (nSection)
        {
        case 0: return "Message";
        default:
            CB_ASSERT (false);
        }
    }

    //return "";
    return nSection + 1;
}


/*
void LogModel::selectTopLeft()
{
    QItemSelectionModel* pSelModel (m_pLogG->selectionModel());
    pSelModel->clear();

    emit layoutChanged();
    if (!m_pCommonData->getLog().empty())
    {
        m_pLogG->setCurrentIndex(index(0, 0));
    }
}
*/


