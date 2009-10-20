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
#include  <QApplication>
#include  <QScrollBar>
#include  <QMessageBox>

#include  "MultiLineTvDelegate.h"

#include  "Helpers.h"

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

MultiLineTvDelegate::MultiLineTvDelegate(QTableView* pTableView/*, QObject* pParent = 0*/) : QItemDelegate(pTableView), m_pTableView(pTableView), m_nLineHeight(0), m_nAddPerLine(0)
{
    CB_CHECK1 (0 != pTableView, std::runtime_error("NULL QTableView not allowed"));
    connect(pTableView->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), pTableView, SLOT(resizeRowsToContents()));
}



QSize MultiLineTvDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.isValid()) { return QSize(); }
    if (0 == m_nLineHeight) { calibrate(option.fontMetrics, option.font); }
    //cout << option.rect.width() << "x" << option.rect.height() << " ";
    int nMargin (QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1);
    //cout << "margin=" << nMargin << endl;

    int j (index.column());
    int nColWidth (m_pTableView->horizontalHeader()->sectionSize(j));
    /*if (4 == j)
    {
        qDebug("%s %d %d", m_pTableView->objectName().toUtf8().data(), m_pTableView->verticalScrollBar()->maximum(), nColWidth);
    }
    QRect r (0, 0, nColWidth - 2*nMargin - 1, 10000); // !!! this "-1" is what's different from Qt's implementation (4.3); it is for the vertical line that delimitates the cells //ttt2 do a screen capture to be sure //ttt2 see if this is fixed in 4.4 2008.30.06 - apparently it's not fixed and the workaround no longer works
*/
    // !!! 2009.04.17 - while working in most cases, the "1" above has this issue: Qt may toggle between showing a scrollbar and hiding it, doing this as many times per second as the CPU can handle; while the app is not frozen, what happens is quite annoying; so we'll just assume there's a scrollbar, until a proper solution is found; (it looks like Qt bug, though, because it can't make up its mind about showing a scrollbar; what Qt should do is try first to remove the scrollbar, see if it can fit everything and if not put back the scrollbar and don't try anything more); the downside is that in some cases more lines are requested than actually needed, but that happened before too (but to a lesser extent); //ttt2 perhaps at least don't do the same for all columns, normally only one is stretcheable
    int nSpace (1);
    //if (m_pTableView->verticalScrollBar()->isVisible())
    if (1 == m_pTableView->verticalScrollBar()->maximum()) // the scrollbar gets 1 up for each line; the issues are around switching between no scrollbar and a scrollbar for 1 line, so hopefully this should take care of the issue;
    {
        nSpace += QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        if (0 != QApplication::style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents))
        {
            nSpace += 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, m_pTableView); //ttt2 Qt 4.4 (and below) - specific; in 4.5 there's a QStyle::PM_ScrollView_ScrollBarSpacing // see also ColumnResizer
        }
    }
    QRect r (0, 0, nColWidth - 2*nMargin - nSpace, 10000);//*/

//QWidget* p (m_pTableView->viewport());
//qDebug("%d %d / %d %d", p->width(), p->height(), m_pTableView->width(), m_pTableView->height());
//QSize s (m_pTableView->maximumViewportSize());
//qDebug("%s %d %d / %d %d", m_pTableView->objectName().toUtf8().data(), m_pTableView->verticalScrollBar()->maximum(), m_pTableView->verticalScrollBar()->minimum(), m_pTableView->width(), m_pTableView->height());
//QString s (index.data(Qt::DisplayRole).toString());
//const char* sz (index.data(Qt::DisplayRole).toString().toUtf8().data());
//if (s.startsWith("No normal"))
//qDebug("#%s", "");
    QSize res (option.fontMetrics.boundingRect(r, Qt::AlignTop | Qt::TextWordWrap, index.data(Qt::DisplayRole).toString()).size());

    //cout << "at (" << index.row() << "," << index.column() << "): " << res.width() << "x" << res.height();
//if (index.column() == 4)
//qDebug("sz %d %d / spc %d", res.width(), res.height(), option.fontMetrics.lineSpacing());
//if (s.startsWith("No normal"))
//qDebug("sz %s %d %d", "", res.width(), res.height());
    res.setWidth(nColWidth);
//res.setHeight(res.height() + 6);
    /*if (1 == m_nAddPerLine)
    {
        res.setHeight(res.height() + res.height()/m_nLineHeight - 1);
        //res.setHeight(res.height() + res.height()/m_nLineHeight); // ??m_nAddPerLine
    }
    else if (2 == m_nAddPerLine)
    {
        res.setHeight(res.height() + 2*res.height()/m_nLineHeight - 2);
    }
    else
    */if (m_nAddPerLine > 0)
    {
        res.setHeight(res.height() + m_nAddPerLine*(res.height()/m_nLineHeight - 1));
    }
//if (s.startsWith("No normal"))
//qDebug("adj sz %s %d %d", "", res.width(), res.height());

    //cout << " => " << res.width() << "x" << res.height() << endl;

    //QSize res (fontMetrics().size(0, text()));
    return res;
}//*/



//s.toUtf8().data()
/*void MultiLineTvDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    //cout << "draw: " << option.rect.width() << "x" << option.rect.height() << endl;
//QString s (index.data(Qt::DisplayRole).toString());
//if (s.startsWith("No normal"))
//qDebug("draw: %d x %d - %s ", option.rect.width(), option.rect.height(), "");
    //int nCol (index.column());
    //if (0 == nCol)
    {
        return QItemDelegate::paint(pPainter, option, index);
    }
}*/


void MultiLineTvDelegate::calibrate(const QFontMetrics& fm, const QFont& /*f*/) const// sets up m_nLine and m_nTotalAdd
{
    //set snHeights;
    //QString s;
    QRect r (0, 0, 300, 10000);//*/
    QSize res (fm.boundingRect(r, Qt::AlignTop | Qt::TextWordWrap, "a").size());
    m_nAddPerLine = res.height() - fm.lineSpacing();
//qDebug("%d ww", m_nAddPerLine);

    //CB_ASSERT (0 <= m_nAddPerLine && m_nAddPerLine <= 1); //ttt2 triggered by "Microsoft Sans Serif 7pt"; see if it can be fixed
    /*if (m_nAddPerLine < 0 || m_nAddPerLine > 1)
    {
        QString s (QString("%1, %2pt").arg(f.family()).arg(f.pointSize()));
        static QString s_qstrLastErrFont;
        if (s != s_qstrLastErrFont)
        {
            s_qstrLastErrFont = s;
            QMessageBox::warning(m_pTableView, "Warning", "The font \"" + s + "\" cannot be displayed correctly. You should go to the Configuration Dialog and choose another general font");
        }
    }*/

    m_nLineHeight = fm.lineSpacing();
//qDebug("%d", m_nLine);
}



