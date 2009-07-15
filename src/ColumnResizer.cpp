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


#include  <algorithm>
#include  <stdexcept>
#include  <set>

#include  <QTableView>
#include  <QTreeWidget>
#include  <QHeaderView>
#include  <QApplication>
#include  <QScrollBar>

#include  "ColumnResizer.h"


//#define PRINT_SIZES

#ifdef PRINT_SIZES
//#include  "fstream_unicode.h"
#endif

using namespace std;


//======================================================================================================================
//======================================================================================================================
//======================================================================================================================




SimpleQTableViewWidthInterface::SimpleQTableViewWidthInterface(QTableView& tbl) : TableWidthInterface(tbl.model()->columnCount()), m_tbl(tbl), m_pModel(tbl.model())
{
    int nCols (m_pModel->columnCount());
    m_vbBold.resize(nCols);

    QFont font (m_tbl.font());
    font.setBold(true);
    QFontMetrics fontMetrics (font);

    QHeaderView* pHdr (tbl.horizontalHeader());

    for (int j = 0; j < nCols; ++j)
    {
        if (pHdr->isSectionHidden(j))
        {
            setFixedWidth(j, 0);
        }
        else
        {
            m_vnHdrWidth[j] = fontMetrics.width(m_pModel->headerData(j, Qt::Horizontal).toString()) + 8/*2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth)*/; // PM_DefaultFrameWidth is not THE thing to use, but it's one way to get some spacing for the header; (well it turned up to be too small, so it got replaced by 8; probably look at the source to see what should be used)
        }
    }
}

/*override*/ int SimpleQTableViewWidthInterface::getRequestedWidth(int nRow, int nCol) const
{
    if (hasFixedWidth(nCol))
    {
        return m_vnMinWidth[nCol];
    }

    int nMargin (QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1); // this "1" should probably be another "metric" constant

    QFont font (m_tbl.font());
    font.setBold(m_vbBold.at(nCol));
    QFontMetrics fontMetrics (font);

    QString qstrVal (m_pModel->data(m_pModel->index(nRow, nCol)).toString());
    int nWidth (fontMetrics.width(qstrVal));
    return nWidth + 2*nMargin + 1; // the "+1" is for the width of the line separating columns
}



/*override*/ int SimpleQTableViewWidthInterface::getTableWidth() const // space available for data columns
{
    int nRes (m_tbl.width()
        - m_tbl.verticalHeader()->sizeHint().width()
        - 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth)); // ttt2 not sure if PM_DefaultFrameWidth is the right param; perhaps looking at the sources might clarify this

    if (m_tbl.verticalScrollBar()->isVisible())
    {
        nRes -= QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        // nothing; // Plastique
        // nRes -= 6; ??? to test //Oxygen
        // nRes -= 2; // CDE
        // nRes -= 4; // Motif
        if (0 != QApplication::style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents))
        {
            nRes -= 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, &m_tbl); //ttt1 Qt 4.4 (and below) - specific; in 4.5 there's a QStyle::PM_ScrollView_ScrollBarSpacing
        }
    }

    return nRes;
}


/*override*/ bool SimpleQTableViewWidthInterface::isHidden(int nCol) const
{
    return m_tbl.horizontalHeader()->isSectionHidden(nCol);
}


/*override*/ void SimpleQTableViewWidthInterface::setWidth(int nCol, int nWidth)
{
    m_tbl.horizontalHeader()->resizeSection(nCol, nWidth);
}

/*override*/ int SimpleQTableViewWidthInterface::getRowCount() const { return m_pModel->rowCount(); }
/*override*/ int SimpleQTableViewWidthInterface::getColumnCount() const { return m_pModel->columnCount(); }

//======================================================================================================================
//======================================================================================================================
//======================================================================================================================




SimpleQTreeWidgetWidthInterface::SimpleQTreeWidgetWidthInterface(QTreeWidget& tbl) : TableWidthInterface(tbl.columnCount()), m_tbl(tbl)
{
    int nCols (m_tbl.columnCount());
    m_vbBold.resize(nCols);

    QFont font (m_tbl.font());
    font.setBold(true);
    QFontMetrics fontMetrics (font);

    QHeaderView* pHdr (tbl.header());

    for (int j = 0; j < nCols; ++j)
    {
        if (pHdr->isSectionHidden(j))
        {
            setFixedWidth(j, 0);
        }
        else
        {
            m_vnHdrWidth[j] = fontMetrics.width(m_tbl.headerItem()->text(j)) + 8/*2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth)*/; // PM_DefaultFrameWidth is not THE thing to use, but it's one way to get some spacing for the header; (well it turned up to be too small, so it got replaced by 8; probably look at the source to see what should be used)
        }
    }
}



/*override*/ int SimpleQTreeWidgetWidthInterface::getRequestedWidth(int nRow, int nCol) const
{
    if (hasFixedWidth(nCol))
    {
        return m_vnMinWidth[nCol];
    }

    int nMargin (QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1); // this "1" should probably be another "metric" constant

    QFont font (m_tbl.font());
    font.setBold(m_vbBold.at(nCol));
    QFontMetrics fontMetrics (font);

    QString qstrVal (m_tbl.topLevelItem(nRow)->text(nCol));
    int nWidth (fontMetrics.width(qstrVal));
    return nWidth + 2*nMargin;
}



/*override*/ int SimpleQTreeWidgetWidthInterface::getTableWidth() const // space available for data columns
{
    int nRes (m_tbl.width()
        //- m_tbl.verticalHeader()->sizeHint().width()
        - 2*QApplication::style()->pixelMetric(QStyle::PM_DefaultFrameWidth)); // ttt2 not sure if PM_DefaultFrameWidth is the right param; perhaps looking at the sources might clarify this

    /*if (m_tbl.verticalScrollBar()->isVisible())
    {
        nRes -= QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    }*/

    nRes -= QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent); // just assume that the scrollbar is visible //ttt1 detect somehow when the scrollbar is visible

    return nRes;
}


/*override*/ bool SimpleQTreeWidgetWidthInterface::isHidden(int nCol) const
{
    return m_tbl.header()->isSectionHidden(nCol);
}


/*override*/ void SimpleQTreeWidgetWidthInterface::setWidth(int nCol, int nWidth)
{
    m_tbl.header()->resizeSection(nCol, nWidth);
}


//======================================================================================================================
//======================================================================================================================
//======================================================================================================================





//ttt2 maybe separate text width calculations from resizing, to avoid unneeded calls to QFontMetrics (useful only when resizing a window, so perhaps can be dropped)
ColumnResizer::ColumnResizer(TableWidthInterface& intf, int nMaxRows /*= 80*/, bool bFill /*= false*/, bool bConsistentResults /*= false*/) : m_intf(intf), m_nMaxRows(nMaxRows), m_bConsistentResults(bConsistentResults) // if bFill is true, colums are resized wider than needed, so they fill the whole area of the table
{
    autoSize(bFill);
}


// how many rows from a column can be shown completely for a given size
int ColumnResizer::getFitCount(int nCol, int nWidth) const
{
    const vector<int>& v (m_vvAllWidths[nCol]);
    vector<int>::const_iterator it (lower_bound(v.begin(), v.end(), nWidth));
    return it - v.begin();
}


double ColumnResizer::SequenceGen::rnd() const
{
    if (RAND_MAX == 32767)
    {
        return (0.0 + rand() + rand()*32768)/32768/32768;
    }
    return rand()*1.0/RAND_MAX;
}


ColumnResizer::SequenceGen::SequenceGen(int nTotalRows, int nTargetRows) : m_nCrt(0)
{
    if (nTargetRows > nTotalRows || nTotalRows <= 0) { throw invalid_argument("Invalid params for SequenceGen"); }

    if (nTotalRows == nTargetRows)
    {
        m_vn.resize(nTargetRows);
        for (int i = 0; i < nTotalRows; ++i)
        {
            m_vn[i] = i;
        }
        return;
    }

    m_vn.reserve(nTargetRows);

    if (nTotalRows > nTargetRows*2)
    {
        set<int> s;
        while ((int)s.size() < nTargetRows)
        {
            int n (int(rnd()*nTotalRows));
            if (0 == s.count(n))
            {
                s.insert(n);
            }
        }
        m_vn.insert(m_vn.end(), s.begin(), s.end());
        return;
    }

    // total not much bigger than target; proceed by elimination;
    for (int i = 0; i < nTotalRows; ++i)
    {
        m_vn.push_back(i);
    }
    set<int> s (m_vn.begin(), m_vn.end());

    while ((int)s.size() > nTargetRows)
    {
        int n (int(rnd()*nTotalRows));
        if (0 != s.count(n))
        {
            s.erase(int(rnd()*nTotalRows));
        }
    }
    m_vn.clear();
    m_vn.insert(m_vn.end(), s.begin(), s.end());
}


/*struct QQQ
{
    QQQ()
    {
        {
            int tot (20); int tgt (20); ColumnResizer::SequenceGen seq (tot, tgt);
            for (int i = 0; i < tgt; ++i)
                cout << seq.getNext() << " ";
            cout << endl;
        }
        {
            int tot (30); int tgt (20); ColumnResizer::SequenceGen seq (tot, tgt);
            for (int i = 0; i < tgt; ++i)
                cout << seq.getNext() << " ";
            cout << endl;
        }
        {
            int tot (100); int tgt (20); ColumnResizer::SequenceGen seq (tot, tgt);
            for (int i = 0; i < tgt; ++i)
                cout << seq.getNext() << " ";
            cout << endl;
        }
    }
};
QQQ qqq;//*/


#ifdef PRINT_SIZES

void ColumnResizer::printWidths(const char* szLabel)
{
    ofstream_utf8 out ("/home/ciobi/yast.txt", ios_base::app); // !!! has to log to a file for when it's running in a library
    out << szLabel << ": " << m_nTableWidth << " / ";
    int qq (0);
    for (int j = 0; j < m_nCols; ++j)
    {
        if (m_intf.isHidden(j))
        {
            out << "[" << m_vColInfo[j].m_nWidth << "] ";
        }
        else
        {
            out << m_vColInfo[j].m_nWidth << " ";
        }
        qq += m_vColInfo[j].m_nWidth;
    }
    out << " / " << m_nTotalWidth << "=" << qq << endl;
}


void ColumnResizer::printCellInfo(const char* szLabel)
{
    ofstream_utf8 out ("/home/ciobi/yast.txt", ios_base::app); // !!! has to log to a file for when it's running in a library
    out << szLabel << ": " << m_nTableWidth << endl;
    for (int j = 0; j < m_nCols; ++j)
    {
        ColInfo& inf (m_vColInfo[j]);
        if (m_intf.isHidden(j))
        {
            out << "[" << m_vColInfo[j].m_nWidth << "] ";
        }
        else
        {
            out << m_vColInfo[j].m_nWidth << " ";
        }
        out << ", " << inf.m_nMinWidth << ", " << inf.m_nLargeAvg << ",, " << inf.m_nAskSmall << ", " << inf.m_nAskLarge << ",, " << inf.m_dLargeDev << ",, **" << inf.m_dPrioSmall << "**, " << inf.m_dPrioLarge << endl;
    }
}

#else

//inline void ColumnResizer::printWidths(const char*) {}
//inline void ColumnResizer::printCellInfo(const char*) {}

#define printWidths(X)
#define printCellInfo(X)

#endif




void ColumnResizer::readAllWidths()
{
    for (int j = 0; j < m_nCols; ++j)
    {
        vector<int>& v (m_vvAllWidths[j]);
        v.resize(m_nUsedRows);
        SequenceGen seq (m_nTotalRows, m_nUsedRows);
        for (int i = 0; i < m_nUsedRows; ++i)
        {
            v[i] = m_intf.isHidden(j) ? 0 : m_intf.getRequestedWidth(seq.getNext(), j); // !!! getRequestedWidth() returns the fixed width for "fixed width" columns, so no special test is needed
        }

        sort(v.begin(), v.end());

        if (!m_intf.hasFixedWidth(j) && !m_intf.isHidden(j) && m_intf.getMinWidthDataHdr(j) > m_vvAllWidths[j][m_nUsedRows - 1])
        {
            m_vvAllWidths[j][m_nUsedRows - 1] = m_intf.getMinWidthDataHdr(j); // this way we avoid doing a lot of "max(m_intf.getMinWidthDataHdr(j), m_vvAllWidths[j][nRows - 1])"; it doesn't matter that the value is destroyed;
        }
    }

    m_nTotalMax = 0;
    for (int j = 0; j < m_nCols; ++j)
    {
        m_nTotalMax += m_vvAllWidths[j][m_nUsedRows - 1];
    }
}


void ColumnResizer::computeColInfo()
{
    int nBeg (m_nUsedRows / 2);
    int nLargeElemsCnt (m_nUsedRows - nBeg);
    if (nLargeElemsCnt > 34)
    {
        nLargeElemsCnt = nLargeElemsCnt*90/100; // cut 10%
    }
    else if (nLargeElemsCnt > 20)
    {
        nLargeElemsCnt -= 3;
    }
    else if (nLargeElemsCnt > 10)
    {
        nLargeElemsCnt -= 2;
    }
    else if (nLargeElemsCnt > 3)
    {
        nLargeElemsCnt -= 1;
    }

    m_nTotalWidth = 0;

    for (int j = 0; j < m_nCols; ++j)
    { // assign LargeAvg for width
        vector<int>& v (m_vvAllWidths[j]); // this contains only zeroes for hidden columns
        int nSum (0);
        int nDev (0);
        for (int i = nBeg; i < nBeg + nLargeElemsCnt; ++i)
        {
            nSum += v[i];
            int x (v[i] - v[nBeg]);
            //nDev += x*x;
            nDev += x;
        }

        ColInfo& inf (m_vColInfo[j]);
        inf.m_nLargeAvg = int(nSum*1.0/nLargeElemsCnt);
        inf.m_dLargeDev = nDev*1.0/nSum;
        inf.m_nMinWidth = m_intf.getMinWidthDataHdr(j);
        if (inf.m_nLargeAvg < inf.m_nMinWidth)
        {
            inf.m_nLargeAvg = inf.m_nMinWidth;
        }

        if (m_intf.hasFixedWidth(j))
        {
            inf.m_dPrioSmall = inf.m_dPrioLarge = 0;
        }
        else
        {
            inf.m_nAskSmall = int(inf.m_nLargeAvg*(1 + inf.m_dLargeDev/2));
            inf.m_dPrioSmall = 1 - inf.m_dLargeDev*0.8;
            //inf.m_dPrioSmall *= log(5) - log(inf.m_nLargeAvg*1.0 / m_nTableWidth + 0.02) + 0.5; // narrow colums get increased priority
            inf.m_nAskLarge = int(inf.m_nLargeAvg*(1 + inf.m_dLargeDev*2));
            inf.m_dPrioLarge = 1 - inf.m_dLargeDev*0.8;
        }
    }

    printCellInfo("computeColInfo");
}


void ColumnResizer::setUpMaxWidths()
{
    m_nTotalWidth = 0;
    for (int j = 0; j < m_nCols; ++j)
    {
        ColInfo& inf (m_vColInfo[j]);
        inf.m_nWidth = m_vvAllWidths[j][m_nUsedRows - 1];
        m_nTotalWidth += inf.m_nWidth;
    }

    printWidths("after setUpMaxWidths");
}


void ColumnResizer::setUpAvgWidths()
{
    m_nTotalWidth = 0;
    for (int j = 0; j < m_nCols; ++j)
    {
        ColInfo& inf (m_vColInfo[j]);
        inf.m_nWidth = inf.m_nLargeAvg;
        m_nTotalWidth += inf.m_nWidth;
    }

    printWidths("after setUpAvgWidths");
}


// if MinWidth wasn't specified, LargeAvg is used instead
void ColumnResizer::setUpMinWidths()
{
    m_nTotalWidth = 0;
    for (int j = 0; j < m_nCols; ++j)
    {
        ColInfo& inf (m_vColInfo[j]);
        if (!m_intf.hasFixedWidth(j) && m_intf.getMinWidth(j) > 0) // !!! "m_intf.getMinWidth(inf.m_nCol) > 0" means that a minWidth was explicitely specified (inf.m_nMinWidth also includes the header)
        {
            inf.m_nWidth = inf.m_nMinWidth;
        }
        else
        {
            inf.m_nWidth = inf.m_nLargeAvg;
        }
        m_nTotalWidth += inf.m_nWidth;
    }

    printWidths("after setUpMinWidths");
}


void ColumnResizer::distributeAskSmall()
{
    if (m_nTotalWidth >= m_nTableWidth) { return; }

    // distribute AskSmall according to prioriries
    int nAskSmallDiffTotal (0);
    double dPrioSum (0);
    for (int j = 0; j < m_nCols; ++j)
    {
        if (!m_intf.hasFixedWidth(j))
        {
            ColInfo& inf (m_vColInfo[j]);
            int nDiff (inf.m_nAskSmall - inf.m_nWidth);
            nAskSmallDiffTotal += nDiff; // !!! inf.m_nAskSmall >= inf.m_nWidth always
            dPrioSum += inf.m_dPrioSmall*nDiff;
        }
    }

    if (nAskSmallDiffTotal + m_nTotalWidth <= m_nTableWidth)
    { // assign all
        for (int j = 0; j < m_nCols; ++j)
        {
            if (!m_intf.hasFixedWidth(j))
            {
                ColInfo& inf (m_vColInfo[j]);
                inf.m_nWidth = inf.m_nAskSmall;
            }
        }
        m_nTotalWidth += nAskSmallDiffTotal;
    }
    else
    { // use priorities to assign all remaining space to AskSmall
        for (int j = 0; j < m_nCols; ++j)
        {
            if (!m_intf.hasFixedWidth(j))
            {
                ColInfo& inf (m_vColInfo[j]);
                int nDiff (inf.m_nAskSmall - inf.m_nWidth);
                //int nAdd (int(nDiff*inf.m_dPrioSmall/dPrioSum));
                int nAdd (int(nDiff*inf.m_dPrioSmall*(m_nTableWidth - m_nTotalWidth)/dPrioSum));
                inf.m_nWidth += nAdd;
                m_nTotalWidth += nAdd;
            }
        }
    }

    printWidths("after distributeAskSmall");
}


void ColumnResizer::distributeAskLarge()
{
    if (m_nTotalWidth >= m_nTableWidth) { return; }

    // distribute AskLarge according to prioriries
    int nAskLargeDiffTotal (0);
    double dPrioSum (0);
    for (int j = 0; j < m_nCols; ++j)
    {
        if (!m_intf.hasFixedWidth(j))
        {
            ColInfo& inf (m_vColInfo[j]);
            int nDiff (inf.m_nAskLarge - inf.m_nWidth);
            if (nDiff > 0) // !!! while initially we had "inf.m_nAskLarge >= inf.m_nWidth", inf.m_nWidth might have been increased in the meantime
            {
                nAskLargeDiffTotal += nDiff;
                dPrioSum += inf.m_dPrioLarge*nDiff;
            }
        }
    }

    if (nAskLargeDiffTotal + m_nTotalWidth <= m_nTableWidth)
    { // assign all
        for (int j = 0; j < m_nCols; ++j)
        {
            if (!m_intf.hasFixedWidth(j))
            {
                ColInfo& inf (m_vColInfo[j]);
                inf.m_nWidth = inf.m_nAskLarge;
            }
        }
        m_nTotalWidth += nAskLargeDiffTotal;
    }
    else
    { // use priorities to assign all remaining space to AskLarge
        for (int j = 0; j < m_nCols; ++j)
        {
            if (!m_intf.hasFixedWidth(j))
            {
                ColInfo& inf (m_vColInfo[j]);
                int nDiff (inf.m_nAskLarge - inf.m_nWidth);
                if (nDiff > 0)
                {
                    //int nAdd (int(nDiff*inf.m_dPrioLarge/dPrioSum));
                    //int nAdd (int(nDistr*inf.m_dPrioLarge/dPrioSum*nDiff/nAskLargeDiffTotal));
                    int nAdd (int(nDiff*inf.m_dPrioLarge*(m_nTableWidth - m_nTotalWidth)/dPrioSum));
                    inf.m_nWidth += nAdd;
                    m_nTotalWidth += nAdd;
                }
            }
        }
    }

    printWidths("after distributeAskLarge");
}


void ColumnResizer::distributeRemaining() // several pixels might remain undistributed after distributeAskSmall() and distributeAskLarge() because of rounding errors; they are dealt with here;
{
    printWidths("before distributeRemaining");

    if (m_nTotalWidth >= m_nTableWidth) { return; }

    // distribute what's left
    int nLeft (m_nTableWidth - m_nTotalWidth);
    int nVarCols (m_nCols - m_nFixedWidthCount);
    int nStep (nLeft/nVarCols);

    for (int j = 0; j < m_nCols; ++j)
    {
        if (!m_intf.hasFixedWidth(j))
        {
            m_vColInfo[j].m_nWidth += nStep;
        }
    }

    nLeft -= nVarCols*nStep;

    for (int j = 0; nLeft > 0; ++j)
    {
        if (!m_intf.hasFixedWidth(j))
        {
            ++m_vColInfo[j].m_nWidth;
            --nLeft;
        }
    }

    m_nTotalWidth = m_nTableWidth;

    printWidths("after distributeRemaining");
}



void ColumnResizer::setWidths()
{
    for (int j = 0; j < m_nCols; ++j)
    {
        if (!m_intf.isHidden(j))
        {
            m_intf.setWidth(j, m_vColInfo[j].m_nWidth);
        }
    }

    printWidths("  setWidths");
}



void ColumnResizer::autoSize(bool bFill)
{
    m_nTotalRows  = m_intf.getRowCount();
    m_nUsedRows = min(m_nTotalRows, m_nMaxRows);
    m_nCols = m_intf.getColumnCount();

    if (0 == m_nTotalRows || 0 == m_nCols) { return; } // not sure if m_nCols can be 0; ... whatever

    m_vvAllWidths.resize(m_nCols);

    m_nFixedWidthCount = 0;
    for (int j = 0; j < m_nCols; ++j)
    {
        if (m_intf.hasFixedWidth(j))
        {
            ++m_nFixedWidthCount;
        }
    }

    m_vColInfo.resize(m_nCols);

    if (m_nFixedWidthCount == m_nCols)
    { // all columns have fixed size
        for (int j = 0; j < m_nCols; ++j)
        {
            m_vColInfo[j].m_nWidth = m_intf.getMinWidth(j);
        }
    }
    else
    {
        if (!m_bConsistentResults && m_nTotalRows != m_nUsedRows)
        {
            srand(11); //ttt2 not such a good idea, as it interferes with other uses of rand(); OTOH, it's better to use it, because then we always get the same results for the same data, even if the selection process is random; seems better to use a separate generator;
        }

        readAllWidths();
        m_nTableWidth = m_intf.getTableWidth();

        computeColInfo();

        if (m_nTotalMax <= m_nTableWidth)
        {
            setUpMaxWidths();
            if (!bFill)
            {
                m_nTotalWidth = m_nTableWidth; // so columns won't get expanded
            }
        }
        else
        {
            setUpAvgWidths();

            if (m_nTotalWidth > m_nTableWidth)
            { // if still too wide, cut to minWidth, if one was set (header width doesn't count)
                setUpMinWidths();
            }
        }

        if (m_nTotalWidth < m_nTableWidth)
        {
            distributeAskSmall();
            if (m_nTotalWidth < m_nTableWidth)
            {
                distributeAskLarge();
                if (m_nTotalWidth < m_nTableWidth)
                {
                    distributeRemaining();
                }
            }
        }
    }

    setWidths();
}


//======================================================================================================================
//======================================================================================================================
//======================================================================================================================


/*override*/ int SimpleQTreeWidgetWidthInterface::getRowCount() const { return m_tbl.topLevelItemCount(); }
/*override*/ int SimpleQTreeWidgetWidthInterface::getColumnCount() const { return m_tbl.columnCount(); }
