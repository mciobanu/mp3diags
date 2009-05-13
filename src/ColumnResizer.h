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


#ifndef ColumnResizerH
#define ColumnResizerH

#include  <vector>


class QTreeWidget;
class QTableView;
class QAbstractItemModel;

/*

ColumnResizer resizes the columns in a QTableView or a QTreeWidget, trying to maximize the number of cells that can be displayed without being truncated, while using no horizontal scrollbar.

It can handle huge tables, because it doesn't read all the data in such a case. There is a threshold for the values in a column that are going to be considered (they are selected randomly). Of course, in some cases this may lead to less than optimal results, but that may happen anyway, because deciding which column is best to shrink and by how much is something quite subjective.

A horizontal scrollbar may still be visible, if even after shrinking the columns need more space than the table has. To avoid such a thing, it is possible to manually assign fixed or minimum widths to various columns. This is done in a TableWidthInterface-derived object, which is what a ColumnResizer works with (it doesn't interact directly with QTableView or QTreeWidget).

ColumnResizer doesn't really have an interface that the user calls. It does everything on its constructor.

Note that for this to work correctly, all layouts must be applied before calling ColumnResizer, which may require a timer. See SimpleQTableViewWidthInterface for more details.

The optional parameter bConsistentResults that is passed to ColumnResizer can be important if other parts of the current program use rand(). If there are more rows than nMaxRows, this triggers the pseudorandom selection of elements. If we want the same data to produce the same result, srand() must be called so the pseudorandom process always gives the same numbers. Calling srand() is something that other functions might not appreciate, which is why bConsistentResults is false by default. (A future version might use its own pseudorandom generator, which won't interfere with srand().)

Usage sample:
    SimpleQTableViewWidthInterface intf1 (*tblView);
    intf1.setMinWidth(0, 50);
    intf1.setFixedWidth(2, 60);
    ColumnResizer rsz1 (intf1);



*/

//======================================================================================================================
//======================================================================================================================
//======================================================================================================================


class TableWidthInterface
{
protected:
    //std::vector<double> m_vdFractionLimit; // fraction of the total table width (? min/max)
    std::vector<bool> m_vbFixedWidth; // if this is set, the width should be exactly getMinWidth()
    std::vector<int> m_vnMinWidth;
    std::vector<int> m_vnHdrWidth;

public:
    TableWidthInterface(int nCols)
    {
        m_vbFixedWidth.resize(nCols);
        m_vnMinWidth.resize(nCols);
        m_vnHdrWidth.resize(nCols);
    }

    virtual ~TableWidthInterface() {}

    virtual int getTableWidth() const = 0; // space available for data columns
    virtual int getRowCount() const = 0;
    virtual int getColumnCount() const = 0;
    virtual int getRequestedWidth(int nRow, int nCol) const = 0;
    virtual bool isHidden(int nCol) const = 0;

    void setMinWidth(int nCol, int nLim) { m_vnMinWidth.at(nCol) = nLim; }
    void setFixedWidth(int nCol, int nLim) { m_vbFixedWidth.at(nCol) = true; m_vnMinWidth.at(nCol) = nLim; }
    bool hasFixedWidth(int nCol) const { return m_vbFixedWidth.at(nCol); }

    int getMinWidthDataHdr(int nCol) const // the minimum width required by the header or set explicitely
    {
        return std::max(m_vnMinWidth.at(nCol), m_vnHdrWidth.at(nCol));
    }

    int getMinWidth(int nCol) const
    {
        return m_vnMinWidth.at(nCol);
    }

    virtual void setWidth(int nCol, int nWidth) = 0; // sets the column width for the underlying widget
};



/*
!!! Note that this class should be used after the table got displayed. There seems to be an issue in Qt 4.3 (and perhaps others), causing "resizeEvent()" to not get called after all the layouts are applied. As a result, incorrect sizes and scrollbars get used. The workaround is to use QTimer to do the calculations after laying out was done: "QTimer::singleShot(1, this, SLOT(onResizeTimer()));", where onResizeTimer does a table resizing.
*/
class SimpleQTableViewWidthInterface : public TableWidthInterface
{
    std::vector<bool> m_vbBold; // bold is defined on a "per-column" basis; another class is needed to handle the "per-cell" case

    QTableView& m_tbl;
    QAbstractItemModel* m_pModel;

    /*override*/ int getTableWidth() const; // space available for data columns
    /*override*/ int getRowCount() const;
    /*override*/ int getColumnCount() const;
    /*override*/ int getRequestedWidth(int nRow, int nCol) const;
    /*override*/ bool isHidden(int nCol) const;
    /*override*/ void setWidth(int nCol, int nWidth);

public:
    SimpleQTableViewWidthInterface(QTableView& tbl);

    void setBold(int nCol, bool bVal = true) { m_vbBold.at(nCol) = bVal; }
};



/*
!!! Note that this class should be used after the table got displayed. There seems to be an issue in Qt 4.3 (and perhaps others), causing "resizeEvent()" to not get called after all the layouts are applied. As a result, incorrect sizes and scrollbars get used. The workaround is to use QTimer to do the calculations after laying out was done: "QTimer::singleShot(1, this, SLOT(onResizeTimer()));", where onResizeTimer does a table resizing.
*/
class SimpleQTreeWidgetWidthInterface : public TableWidthInterface
{
    std::vector<bool> m_vbBold; // bold is defined on a "per-column" basis; another class is needed to handle the "per-cell" case //ttt2 see if "bold" can actually be done

    QTreeWidget& m_tbl;

    /*override*/ int getTableWidth() const; // space available for data columns
    /*override*/ int getRowCount() const;
    /*override*/ int getColumnCount() const;
    /*override*/ int getRequestedWidth(int nRow, int nCol) const;
    /*override*/ bool isHidden(int nCol) const;
    /*override*/ void setWidth(int nCol, int nWidth);

public:
    SimpleQTreeWidgetWidthInterface(QTreeWidget& tbl);

    void setBold(int nCol, bool bVal = true) { m_vbBold.at(nCol) = bVal; }
};





/*
nMaxRows sets a limit to how many values are considered for a column; useful because going through all of them may be too costly with big tables;
if bFill is set, the columns get widened more than needed, so the whole area of the table gets covered (fixed-width columns don't get touched, though)
*/
class ColumnResizer
{
    TableWidthInterface& m_intf;

    std::vector<std::vector<int> > m_vvAllWidths;

    int getFitCount(int nCol, int nWidth) const; // how many rows from a column can be shown completely for a given size

    struct ColInfo
    {
        int m_nLargeAvg;
        double m_dLargeDev;
        int m_nMinWidth;
        int m_nWidth;

        int m_nAskSmall;
        double m_dPrioSmall; // usually between 0 and 1, but may be higher for narrow columns
        int m_nAskLarge;
        double m_dPrioLarge; // between 0 and 1
    };

    void autoSize(bool bFill);
    int m_nMaxRows;
    bool m_bConsistentResults;

    int m_nTotalRows;
    int m_nUsedRows;
    int m_nCols;
    int m_nFixedWidthCount;

    std::vector<ColInfo> m_vColInfo;
    int m_nTotalWidth;
    int m_nTotalMax;
    int m_nTableWidth;

    void readAllWidths();
    void computeColInfo();

    void setUpMaxWidths();
    void setUpAvgWidths();
    void setUpMinWidths();

    void distributeAskSmall();
    void distributeAskLarge();
    void distributeRemaining(); // several pixels might remain undistributed after distributeAskSmall() and distributeAskLarge() because of rounding errors; they are dealt with here;

    void setWidths();

    void printWidths(const char* szLabel/*, std::ostream& out*/);
    void printCellInfo(const char* szLabel/*, std::ostream& out*/);


    class SequenceGen
    {
        std::vector<int> m_vn;
        int m_nCrt;
        double rnd() const;
    public:
        SequenceGen(int nTotalRows, int nTargetRows);
        int getNext() { return m_vn.at(m_nCrt++); }
    };

public:
    enum { DONT_FILL, FILL };
    enum { INCONSISTENT_RESULTS, CONSISTENT_RESULTS };
    ColumnResizer(TableWidthInterface& intf, int nMaxRows = 80, bool bFill = false, bool bConsistentResults = false); // resizing is achieved by just instantiating ColumnResizer
};




#endif // #ifndef ColumnResizerH


