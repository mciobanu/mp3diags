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


#ifndef DoubleListH
#define DoubleListH

#include  <QWidget>

#include  "ui_DoubleListWdg.h"

#include  "MultiLineTvDelegate.h"



class ListPainter;


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


namespace DoubleListImpl {


// TableModels and Delegates don't have to be public, but they must be in a header, for the Qt preprocessing to work. // ttt3 perhaps move to separate header


class AvailableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    //std::vector<const Elem*> m_vpElems; // doesn't own the pointers
    const ListPainter& m_listPainter;
public:
    AvailableModel(ListPainter& listPainter);
    /*override*/ int rowCount(const QModelIndex&) const { return rowCount(); }
    /*override*/ int columnCount(const QModelIndex&) const { return columnCount(); }
    /*override*/ QVariant data(const QModelIndex&, int nRole) const;

    int columnCount() const;
    int rowCount() const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;

    void emitLayoutChanged();
};


class SelectedModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    //std::vector<const Elem*> m_vpElems; // doesn't own the pointers
    const ListPainter& m_listPainter;
public:
    SelectedModel(ListPainter& listPainter);
    /*override*/ int rowCount(const QModelIndex&) const { return rowCount(); }
    /*override*/ int columnCount(const QModelIndex&) const { return columnCount(); }
    /*override*/ QVariant data(const QModelIndex&, int nRole) const;

    int columnCount() const;
    int rowCount() const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;

    QTableView* m_pTableView; // needed to retrieve column widths, which are needed for tooltips, which are no longer used but this is an example of how to do it
    void emitLayoutChanged();
};


//class AvailableModelDelegate : public QItemDelegate
class AvailableModelDelegate : public MultiLineTvDelegate
{
    Q_OBJECT

    const ListPainter& m_listPainter;
public:
    //AvailableModelDelegate(AvailableModel* pAvailableModel, QObject* pParent) : QItemDelegate(pParent), m_pAvailableModel(pAvailableModel) {}
    //AvailableModelDelegate(AvailableModel* pAvailableModel, QTableView* pTableView) : MultiLineTvDelegate(pTableView), m_pAvailableModel(pAvailableModel) {}
    AvailableModelDelegate(ListPainter& listPainter, QTableView* pTableView) : MultiLineTvDelegate(pTableView), m_listPainter(listPainter) {}

    /*override*/ void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    //AvailableModel* m_pAvailableModel;
};


class SelectedModelDelegate : public MultiLineTvDelegate
{
    Q_OBJECT

    const ListPainter& m_listPainter;
public:
    SelectedModelDelegate(ListPainter& listPainter, QTableView* pTableView) : MultiLineTvDelegate(pTableView), m_listPainter(listPainter) {}

    /*override*/ void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

};

} // namespace DoubleListImpl


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================



class ListElem
{
public:
    virtual ~ListElem() {} //

    virtual std::string getText(int nCol) const = 0;
};


class DoubleList;

//ttt2 perhaps have a "mirror" option, where available elems are on the left and selected on the right

/*
ListPainter has 2 related purposes:
    1) it holds vectors and some other data used to describe the contents of the lists;
    2) provides data needed by QAbstractTableModel and QItemDelegate to display the tables; to do this it uses the vectors described above, as well as abstract functions, which have to be implemented by derived classes;

The basic idea is that there are 3 vectors: one with "elements", containing everything that is included in the lists, and two with integers, which are indexes in the vector with "elements". One of these two is used for "selected" items, while the other is for "available" ones. Depending on the settings, elements that are included in "selected" may be removed or not from "available" (this is DoubleList's business, though; ListPainter only provides the storage).

Actually it's a bit more complex: 2 more vectors are used to support "restore open" and "restore default" functionality.

"Restore default" is optional. To work, it needs reset() to be implemented and a param must be passed to DoubleList, telling it to show the corresponding button. If reset() is implemented, it should populate m_vpResetAll with pointers to elements and m_vSel with indices into m_vpResetAll. In most cases, reset() would only change m_vpResetAll if it is empty, because "there's only one default"; however, m_vSel should most likely be changed.

Assuming DoubleList is used in a dialog window that gets closed, if reset() is implemented, the caller should check if m_vpResetAll contains elements regardless of the dialog being closed with OK or Cancel. Then it should probably transfer the information and delete the pointers in both vpOrigAll and vpResetAll (or perhaps only in one of them, based on the dialog result and m_bResultInReset). It's the caller's responsibility to provide reset() and to deal with m_vpResetAll in a compatible way.


ListPainter and DoubleList don't delete any pointers from m_vpOrigAll or m_vpResetAll. It is the responsability of the class derived from ListPainter or of its users to release pointers. The reason for this is that no reasonable assumptions can be made about the lifetime of the pointers. They may have to be deleted after DoubleList is destroyed or they may have to live until the program is closed. Also, more things may have to be done besides deleting the pointers.


Sometimes it makes sense to consider the fact that nothing is "selected" as meaning that everything is "selected". To make this convention obvious from a visual point of view, m_strNothingSel should be used. If its value is non-empty, when m_vSel is empty the "selected" list switches to using a single column (with no title) and a single row, containing a single data cell, whose text is m_strNothingSel (which should be something like "<<all elements>>". Of course, this is just visual. The user of the class must also make sure that the meaning of an empty m_vSel is the same as that of a m_vSel that contains all the available values.

*/
class ListPainter
{
public:
    typedef std::vector<const ListElem*> AllList;
    typedef std::vector<int> SubList;

    ListPainter(const std::string& strNothingSel) : m_pDoubleList(0), m_bResultInReset(false), m_strNothingSel(strNothingSel)  {}
    virtual ~ListPainter() {}

    virtual int getColCount() const = 0;
    virtual std::string getColTitle(int nCol) const = 0;

    enum { ALL_LIST, SUB_LIST };
    virtual void getColor(int nIndex, int nColumn, bool bSubList, QColor& bckgColor, QColor& penColor, double& dGradStart, double& dGradEnd) const = 0; // nIndex is an index in the "all" table; color is both input and output param; dGradStart and dGradEnd must be either -1 (in which case no gradient is used) or between 0 and 1, with dGradStart < dGradEnd; they are -1 when the call is made, so can be left alone
    virtual int getColWidth(int nCol) const = 0; // positive values are used for fixed widths, while negative ones are for "stretched"
    virtual int getHdrHeight() const = 0;

    enum TooltipKey { SELECTED_G, AVAILABLE_G, ADD_B, DELETE_B, ADD_ALL_B, DELETE_ALL_B, RESTORE_DEFAULT_B, RESTORE_OPEN_B };
    virtual std::string getTooltip(TooltipKey eTooltipKey) const = 0; // !!! this must return an empty string for buttons that are removed, otherwise deallocated memory gets accessed

    virtual Qt::Alignment getAlignment(int nCol) const = 0;
    virtual void reset() = 0; // "restore default" functionality

    const std::string& getNothingSelStr() const { return m_strNothingSel; }

    const SubList& getSel() const { return m_vSel; }
    const SubList& getAvailable() const { return m_vAvailable; }

    const AllList& getAll() const { return m_bResultInReset ? m_vpResetAll : m_vpOrigAll; }

protected:
    DoubleList* m_pDoubleList; // widget initialized with "this" as parent; deleted automatically by Qt; put here more as a convenience place to store the pointer, but it doesn't HAVE to be used (unlike the other member variables);

    AllList m_vpOrigAll; // the initial list with "all" elements; the derived class must initialize this;
    AllList m_vpResetAll; // the list with "all" elements after reset() got called; the derived class must initialize this when reset() is called;

    SubList m_vOrigSel; // used by on_m_pRestoreOpenB_clicked(); the derived class must initialize this;
    SubList m_vSel; // the "selected" elements; the derived class must initialize this; usually it's identical to m_vOrigSel at creation;

    // !!! note that both m_vOrigSel and m_vSel must be initialized (in a previous version m_vSel used to be copied from m_vOrigSel, but the current way is more flexible with cases when a DoubleList is created and deleted dynamically, multiple times, while its parent is alive and visible;)

    bool isResultInReset() const { return m_bResultInReset; }
private:
    SubList m_vAvailable; // shouldn't be initialized in the derived class (well, it can't anyway, since it's private); DoubleList takes care of this, making it a sorted vector containing either all the numbers between 0 and "getAll().size() - 1", or only those numbers that aren't in m_vSel

    bool m_bResultInReset; // if m_vSel indexes m_vpResetAll or m_vpOrigAll

    std::string m_strNothingSel;

    friend class DoubleList;
};


template<typename T, typename V> void getCastElem(T*& p, V& v, int i)
{
    p = dynamic_cast<T*>(v[i]);
    CB_ASSERT(0 != p);
}



/*
Widget handling a sublist of elements that are "selected" from a full list.

It is quite configurable, but more options can be envisioned.

Most of the data are stored in a ListPainter, which must be passed in the constructor (actually some class derived from ListPainter, which is abstract). This is mainly for convenience and clarity (some vectors could be moved to DoubleList, but some must stay in ListPainter and having them all in one place seems better).

*/
class DoubleList : public QWidget, private Ui::DoubleListWdg
{
    Q_OBJECT

    typedef ListPainter::AllList AllList;
    typedef ListPainter::SubList SubList;

public:
    enum SelectionMode { SINGLE_UNSORTABLE, SINGLE_SORTABLE, MULTIPLE }; // "SORTABLE" means "sortable by the user"; if the user can't sort the "sel" list, the elements are sorted to match the order they appear in the "all" list"; MULTIPLE is always "sortable" //ttt2 the word "Selection" is confusing in this context

private:

    ListPainter& m_listPainter;

    SelectionMode m_eSelectionMode; // ttt2 perhaps add "Up" and "Down" buttons

    void setUpGrid(QTableView* pGrid);

    void resizeColumns(QTableView* pGrid); // this should be called after LayoutChanged was sent
    void clearSel();
    void resizeRows();
    void emitLayoutChanged();

    void adjustOnDataChanged();


    DoubleListImpl::AvailableModel m_availableModel;
    DoubleListImpl::SelectedModel m_selectedModel;
    friend class DoubleListImpl::AvailableModel;
    friend class DoubleListImpl::SelectedModel;
    friend class DoubleListImpl::AvailableModelDelegate;
    friend class DoubleListImpl::SelectedModelDelegate;

    /*override*/ void resizeEvent(QResizeEvent* pEvent);

    void initAvailable();

    bool m_bSectionMovedLock;
public:
    enum UsedButtons { NONE = 0x00, ADD_ALL = 0x01, DEL_ALL = 0x02, RESTORE_OPEN = 0x04, RESTORE_DEFAULT = 0x08 };
    DoubleList(
        ListPainter& listPainter,
        int nButtons, // or/xor of UsedButtons
        SelectionMode eSelectionMode,
        const std::string& strAvailableLabel,
        const std::string& strSelLabel,
        QWidget* pParent, Qt::WFlags fl = 0);
    ~DoubleList();

signals:
    void dataChanged();
    void avlDoubleClicked(int nRow);

protected slots:
    void on_m_pAddB_clicked();
    void on_m_pDeleteB_clicked();
    void on_m_pAddAllB_clicked();
    void on_m_pDeleteAllB_clicked();
    void on_m_pRestoreOpenB_clicked();
    void on_m_pRestoreDefaultB_clicked();

    void onSelSectionMoved(int nLogicalIndex, int nOldVisualIndex, int nNewVisualIndex);

    void onResizeTimer();

    void onAvlDoubleClicked(const QModelIndex& index);
};

#endif

