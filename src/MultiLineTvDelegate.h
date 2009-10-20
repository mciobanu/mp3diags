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


#ifndef MultiLineTvDelegateH
#define MultiLineTvDelegateH

#include  <QItemDelegate>


class QTableView;

// This helps fix an issue with the default delegate, namely the fact that there is a shight discrepancy between the estimated size returned by sizeHint() and the actual size used when drawing; when a column gets resized, it is quite likely that close to the limit when the number of lines changes the last letters in a word will be changed to "...", because sizeHint() detects that 1 line is enough while paint() wants 2 lines.
class MultiLineTvDelegate : public QItemDelegate
{
    Q_OBJECT
protected:
    QTableView* m_pTableView;
    mutable int m_nLineHeight, m_nAddPerLine; // needed because of Qt bugs in QFontMetrics::boundingRect() that cause incorrect heights to be returned when the text wraps across several lines (e.g. with some fonts an additional pixel must be added for each line to get the correct value)
    void calibrate(const QFontMetrics&, const QFont&) const; // sets up m_nLine and m_nTotalAdd
public:
    MultiLineTvDelegate(QTableView* pTableView/*, QObject* pParent = 0*/);

    /*~MultiLineTvDelegate()
    {
        printf("destr MultiLineTvDelegate: %p\n", this);
    }*/

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    //void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    /*void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        //cout << "draw: " << option.rect.width() << "x" << option.rect.height() << endl;
        int nCol (index.column());
        if (0 == nCol)
        {
            return QItemDelegate::paint(pPainter, option, index);
        }
    }*/


};



#endif // #ifndef MultiLineTvDelegateH



