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

#include  <QPainter>
#include  <QTimer>
#include  <QHeaderView>

#include  "DoubleList.h"

#include  "Helpers.h"
#include  "CommonData.h"


using namespace std;

using namespace DoubleListImpl;




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================



AvailableModel::AvailableModel(ListPainter& listPainter) : m_listPainter(listPainter)
{
}


int AvailableModel::rowCount() const
{
    int n (cSize(m_listPainter.getAvailable()));
    return n;
}


int AvailableModel::columnCount() const
{
    return m_listPainter.getColCount();
}


/*override*/ QVariant AvailableModel::data(const QModelIndex& index, int nRole) const
{
//LAST_STEP("AvailableModel::data()");
    if (!index.isValid()) { return QVariant(); }

    if (nRole != Qt::DisplayRole) { return QVariant(); }

    int nRow (index.row());
    int nAllIndex (m_listPainter.getAvailable()[nRow]);
    return convStr(m_listPainter.getAll()[nAllIndex]->getText(index.column()));
}


/*override*/ QVariant AvailableModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /* = Qt::DisplayRole*/) const
{
//LAST_STEP("AvailableModel::headerData");
    if (nRole == Qt::SizeHintRole)
    {
        return getNumVertHdrSize(cSize(m_listPainter.getAvailable()), eOrientation);
    }

    if (nRole != Qt::DisplayRole) { return QVariant(); }
    if (Qt::Horizontal == eOrientation)
    {
        return convStr(m_listPainter.getColTitle(nSection));
    }

    return nSection + 1;
}



void AvailableModel::emitLayoutChanged()
{
    emit layoutChanged();
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------



/*override*/ void AvailableModelDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    pPainter->save();

    //pPainter->fillRect(option.rect, QBrush(m_listPainter.getColor(m_listPainter.getAvailable()[index.row()], index.column(), pPainter->background().color()))); //ttt2 make sure background() is the option to use
    QColor bckgCol (pPainter->background().color());
    QColor penCol (pPainter->pen().color());
    double dGradStart (-1), dGradEnd (-1);
    m_listPainter.getColor(m_listPainter.getAvailable()[index.row()], index.column(), ListPainter::ALL_LIST, bckgCol, penCol, dGradStart, dGradEnd);
    QLinearGradient grad (0, option.rect.y(), 0, option.rect.y() + option.rect.height());
    configureGradient(grad, bckgCol, dGradStart, dGradEnd);
    pPainter->fillRect(option.rect, grad);

    QStyleOptionViewItemV2 myOption (option);
    myOption.displayAlignment = m_listPainter.getAlignment(index.column());
    myOption.palette.setColor(QPalette::Text, penCol);
    QItemDelegate::paint(pPainter, myOption, index);

    pPainter->restore();
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================



SelectedModel::SelectedModel(ListPainter& listPainter) : m_listPainter(listPainter)
{
}


int SelectedModel::rowCount() const
{
    int n (cSize(m_listPainter.getSel()));
    if (0 == n && !m_listPainter.getNothingSelStr().empty()) { return 1; }
    return n;
}


int SelectedModel::columnCount() const
{
    if (m_listPainter.getSel().empty() && !m_listPainter.getNothingSelStr().empty())
    {
        return 1;
    }
    return m_listPainter.getColCount();
}



/*override*/ QVariant SelectedModel::data(const QModelIndex& index, int nRole) const
{
//LAST_STEP("SelectedModel::data()");

    if (!index.isValid()) { return QVariant(); }

    if (nRole != Qt::DisplayRole && nRole != Qt::ToolTipRole) { return QVariant(); }

    int nRow (index.row());

    QString qstrRes;
    if (m_listPainter.getSel().empty())
    {
        CB_ASSERT(0 == nRow);
        //qstrRes = 0 == index.column() ? "" : "<all notes>";
        qstrRes = convStr(m_listPainter.getNothingSelStr()); // !!! in this case there's only 1 column
    }
    else
    {
        int nAllIndex (m_listPainter.getSel()[nRow]);
        qstrRes = convStr(m_listPainter.getAll()[nAllIndex]->getText(index.column()));
    }

    if (nRole == Qt::ToolTipRole)
    {
#if 0
        // works but tooltip is no longer needed here
        //QFontMetrics fm (QApplication::fontMetrics());
        QFontMetrics fm (m_pTableView->fontMetrics()); // !!! no need to use "QApplication::fontMetrics()", because m_pTableView is needed anyway to get the column widths
        int nWidth (fm.width(qstrRes));

        if (nWidth + 10 < m_pTableView->horizontalHeader()->sectionSize(index.column())) // ttt2 "10" is hard-coded
        {
            return QVariant();
        }//*/
#else
        return QVariant();
#endif
    }

    return qstrRes;
}


/*override*/ QVariant SelectedModel::headerData(int nSection, Qt::Orientation eOrientation, int nRole /* = Qt::DisplayRole*/) const
{
//LAST_STEP("SelectedModel::headerData");
    if (nRole == Qt::SizeHintRole)
    {
        return getNumVertHdrSize(cSize(m_listPainter.getSel()), eOrientation);
    }

    if (nRole != Qt::DisplayRole) { return QVariant(); }
    if (Qt::Horizontal == eOrientation)
    {
        if (m_listPainter.getSel().empty() && !m_listPainter.getNothingSelStr().empty())
        {
            return "";
        }
        return convStr(m_listPainter.getColTitle(nSection));
    }

    if (m_listPainter.getSel().empty())
    {
        return "";
    }

    return nSection + 1;
}



void SelectedModel::emitLayoutChanged()
{
    emit layoutChanged();
}


//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------


/*override*/ void SelectedModelDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (m_listPainter.getSel().empty())
    {
        QItemDelegate::paint(pPainter, option, index);
        return;
    }

    pPainter->save();
    //int nRow (index.row());
    //int nCol (index.column());
    //const Note* pNote (m_pSelectedModel->m_vpNotes[nRow]);

    /*
    // first approach; works, but it's more complicated
    QStyleOptionViewItemV2 myOption (option);
    if (0 == index.column())
    {
        myOption.displayAlignment |= Qt::AlignHCenter;
    }

    pPainter->fillRect(myOption.rect, QBrush(noteColor(*pNote)));

    QString qstrText (index.model()->data(index, Qt::DisplayRole).toString());

    drawDisplay(pPainter, myOption, myOption.rect, qstrText);
    drawFocus(pPainter, myOption, myOption.rect);*/

    // second approach
    //pPainter->fillRect(option.rect, QBrush(m_listPainter.getColor(m_listPainter.getSel()[nRow], nCol, option.palette.color(QPalette::Active, QPalette::Base)))); //ttt3 " Active" not right if the window is inactive

    QColor bckgCol (option.palette.color(QPalette::Active, QPalette::Base)); //ttt3 compare to avl, where it's "QColor bckgCol (pPainter->background().color());" see why // 2009.07.15 - probably "option..." is better; the other one relies on the painter to be initialized to what's in option, but not sure that is required; so penCol is not quite right as well; however, the painter probably comes initialized correctly

    QColor penCol (pPainter->pen().color());
    double dGradStart (-1), dGradEnd (-1);
    m_listPainter.getColor(m_listPainter.getSel()[index.row()], index.column(), ListPainter::SUB_LIST, bckgCol, penCol, dGradStart, dGradEnd);
    QLinearGradient grad (0, option.rect.y(), 0, option.rect.y() + option.rect.height());
    configureGradient(grad, bckgCol, dGradStart, dGradEnd);
    pPainter->fillRect(option.rect, grad);

    QStyleOptionViewItemV2 myOption (option);
    myOption.displayAlignment = m_listPainter.getAlignment(index.column());
    myOption.palette.setColor(QPalette::Text, penCol);
    QItemDelegate::paint(pPainter, myOption, index);

    pPainter->restore();
}




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

//#include <iostream>



DoubleList::DoubleList(
        ListPainter& listPainter,
        int nButtons,
        //bool bAllowMultipleSel,
        //bool bUserSortSel,
        SelectionMode eSelectionMode,
        const std::string& strAvailableLabel,
        const std::string& strSelLabel,
        QWidget* pParent, Qt::WFlags fl /* =0*/) :

        QWidget(pParent, fl), Ui::DoubleListWdg(),

        m_listPainter(listPainter),
        //m_bAllowMultipleSel(bAllowMultipleSel),
        //m_bUserSortSel(bUserSortSel),
        m_eSelectionMode(eSelectionMode),
        m_availableModel(listPainter),
        m_selectedModel(listPainter),
        m_bSectionMovedLock(false)
{
    setupUi(this);
//printContainer(listPainter.m_vOrigSel, cout);
    if (SINGLE_UNSORTABLE == eSelectionMode)
    {
        for (int i = 0, n = cSize(listPainter.m_vOrigSel); i < n - 1; ++i)
        {
            CB_ASSERT(listPainter.m_vOrigSel[i] < listPainter.m_vOrigSel[i + 1]);
        }
    }

    m_pAvailableL->setText(convStr(strAvailableLabel));
    m_pSelL->setText(convStr(strSelLabel));

    if (0 == (nButtons & ADD_ALL)) { delete m_pAddAllB; }
    if (0 == (nButtons & DEL_ALL)) { delete m_pDeleteAllB; }
    if (0 == (nButtons & RESTORE_OPEN)) { delete m_pRestoreOpenB; }
    if (0 == (nButtons & RESTORE_DEFAULT)) { delete m_pRestoreDefaultB; }

    //m_listPainter.m_vSel = m_listPainter.m_vOrigSel;

    initAvailable();

    m_pAvailableG->setModel(&m_availableModel);
    m_pSelectedG->setModel(&m_selectedModel);

    AvailableModelDelegate* pAvDel = new AvailableModelDelegate(m_listPainter, m_pAvailableG);
    m_pAvailableG->setItemDelegate(pAvDel);

    SelectedModelDelegate* pSelDel = new SelectedModelDelegate(m_listPainter, m_pSelectedG);
    m_pSelectedG->setItemDelegate(pSelDel);

    setUpGrid(m_pAvailableG);
    setUpGrid(m_pSelectedG);

    {
        string s;
        s = m_listPainter.getTooltip(ListPainter::SELECTED_G); if (!s.empty()) { m_pSelectedG->setToolTip(convStr(s)); }
        s = m_listPainter.getTooltip(ListPainter::AVAILABLE_G); if (!s.empty()) { m_pAvailableG->setToolTip(convStr(s)); }
        s = m_listPainter.getTooltip(ListPainter::ADD_B); if (!s.empty()) { m_pAddB->setToolTip(convStr(s)); }
        s = m_listPainter.getTooltip(ListPainter::DELETE_B); if (!s.empty()) { m_pDeleteB->setToolTip(convStr(s)); }
        s = m_listPainter.getTooltip(ListPainter::ADD_ALL_B); if (!s.empty()) { m_pAddAllB->setToolTip(convStr(s)); }
        s = m_listPainter.getTooltip(ListPainter::DELETE_ALL_B); if (!s.empty()) { m_pDeleteAllB->setToolTip(convStr(s)); }
        s = m_listPainter.getTooltip(ListPainter::RESTORE_DEFAULT_B); if (!s.empty()) { m_pRestoreDefaultB->setToolTip(convStr(s)); }
        s = m_listPainter.getTooltip(ListPainter::RESTORE_OPEN_B); if (!s.empty()) { m_pRestoreOpenB->setToolTip(convStr(s)); }
    }

    if (SINGLE_UNSORTABLE != m_eSelectionMode)
    {
        m_pSelectedG->verticalHeader()->setMovable(true);
    }

    connect(m_pSelectedG->verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(onSelSectionMoved(int, int, int)));

    connect(m_pAvailableG, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onAvlDoubleClicked(const QModelIndex&)));

    //ttt2 perhaps use QAction::shortcutContext(Qt::WidgetShortcut) to set up shortcuts, if it works well enough
}


DoubleList::~DoubleList()
{
}


void DoubleList::onSelSectionMoved(int /*nLogicalIndex*/, int nOldVisualIndex, int nNewVisualIndex)
{
    CB_ASSERT(SINGLE_SORTABLE == m_eSelectionMode || MULTIPLE == m_eSelectionMode);
    NonblockingGuard sectionMovedGuard (m_bSectionMovedLock);

    if (!sectionMovedGuard)
    {
        return;
    }

    //m_pSelectedG->verticalHeader()->headerDataChanged(Qt::Vertical, 0, 4);
    m_pSelectedG->verticalHeader()->moveSection(nNewVisualIndex, nOldVisualIndex);

    SubList& vSel (m_listPainter.m_vSel);
    int x (vSel[nOldVisualIndex]);
    vSel.erase(vSel.begin() + nOldVisualIndex);
    vSel.insert(vSel.begin() + nNewVisualIndex, x);

    adjustOnDataChanged();
}


void DoubleList::initAvailable()
{
    m_listPainter.m_vAvailable.clear();

    switch (m_eSelectionMode)
    {
    case MULTIPLE:
        for (int i = 0, n = cSize(m_listPainter.getAll()); i < n; ++i)
        {
            m_listPainter.m_vAvailable.push_back(i); // in a sense, m_listPainter.m_vAvailable shouldn't be used at all in this case, and it isn't in the DoubleList code; however, it makes AvailableModel easier, because while DoubleList always has to check the value of m_eSelectionMode anyway, AvailableModel doesn't have to do this, but an empty m_listPainter.m_vAvailable in the MULTIPLE case would force it to do so;
        }
        break;

    case SINGLE_UNSORTABLE:
        {
            int n (cSize(m_listPainter.getAll()));
            m_listPainter.m_vSel.push_back(n);
            for (int i = 0, j = 0; i < n; ++i)
            {
                if (i < m_listPainter.m_vSel[j])
                {
                    m_listPainter.m_vAvailable.push_back(i);
                }
                else
                {
                    ++j;
                }
            }
            m_listPainter.m_vSel.pop_back();
        }
        break;

    case SINGLE_SORTABLE:
        {
            SubList v (m_listPainter.m_vSel.begin(), m_listPainter.m_vSel.end());
            sort(v.begin(), v.end());
            int n (cSize(m_listPainter.getAll()));
            v.push_back(n);
            for (int i = 0, j = 0; i < n; ++i)
            {
                if (i < v[j])
                {
                    m_listPainter.m_vAvailable.push_back(i);
                }
                else
                {
                    ++j;
                }
            }
        }
        break;

    }
}


void DoubleList::setUpGrid(QTableView* pGrid)
{
    pGrid->verticalHeader()->setMinimumSectionSize(m_listPainter.getHdrHeight());

    pGrid->verticalHeader()->setDefaultSectionSize(m_listPainter.getHdrHeight());
    pGrid->verticalHeader()->setResizeMode(QHeaderView::Interactive);
    pGrid->verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);

    resizeColumns(pGrid);
}

void DoubleList::resizeColumns(QTableView* pGrid)
{
    if (1 == pGrid->horizontalHeader()->count())
    {
        pGrid->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    }
    else
    {
        for (int i = 0, n = m_listPainter.getColCount(); i < n; ++i)
        {
            int w (m_listPainter.getColWidth(i));
            if (w >= 0)
            {
                pGrid->horizontalHeader()->setResizeMode(i, QHeaderView::Interactive);
                pGrid->horizontalHeader()->resizeSection(i, w);
            }
            else
            {
                pGrid->horizontalHeader()->setResizeMode(i, QHeaderView::Stretch);
            }
        }
    }
}



void DoubleList::clearSel()
{
    m_pSelectedG->selectionModel()->clear();
    m_pAvailableG->selectionModel()->clear();
}



void DoubleList::resizeRows()
{
    m_pAvailableG->resizeRowsToContents();
    m_pSelectedG->resizeRowsToContents();
}



void DoubleList::emitLayoutChanged()
{
    m_availableModel.emitLayoutChanged();
    m_selectedModel.emitLayoutChanged();
}




void DoubleList::adjustOnDataChanged()
{
    emitLayoutChanged();
    resizeColumns(m_pSelectedG); // ttt2 too much: this should only be done when switching between 1 and multiple columns; and even then, going from 1 to many, it should use the "previous" column widths, rather than the "default" ones
    resizeRows();
    clearSel();

    emit dataChanged();
}




void DoubleList::on_m_pAddB_clicked()
{
    QItemSelectionModel* pSelMdl (m_pAvailableG->selectionModel());
    QModelIndexList lSel (pSelMdl->selection().indexes());

    set<int> sSelPos; // the rows must be sorted

    for (QModelIndexList::iterator it = lSel.begin(), end = lSel.end(); it != end; ++it)
    {
        int nRow (it->row());
        CB_ASSERT (nRow >= 0);
        sSelPos.insert(nRow);
    }

    add(sSelPos);
}

void DoubleList::add(const std::set<int>& sSelPos) // adds elements from the specified indexes
{
    switch (m_eSelectionMode)
    {
    case SINGLE_UNSORTABLE:
        for (set<int>::const_reverse_iterator it = sSelPos.rbegin(), end = sSelPos.rend(); it != end; ++it) // the last must be processed first, so removal of elements doesn't change the row number for the remaining ones
        {
            int nRow (*it);
            int nIndex (m_listPainter.m_vAvailable[nRow]);
            vector<int>::iterator it1 (lower_bound(m_listPainter.m_vSel.begin(), m_listPainter.m_vSel.end(), nIndex));
            m_listPainter.m_vSel.insert(it1, nIndex);
            m_listPainter.m_vAvailable.erase(m_listPainter.m_vAvailable.begin() + nRow);
        }
        break;

    case SINGLE_SORTABLE:
        for (set<int>::const_iterator it = sSelPos.begin(), end = sSelPos.end(); it != end; ++it)
        {
            int nRow (*it);
            int nIndex (m_listPainter.m_vAvailable[nRow]);
            m_listPainter.m_vSel.push_back(nIndex);
        }
        for (set<int>::const_reverse_iterator it = sSelPos.rbegin(), end = sSelPos.rend(); it != end; ++it)
        {
            int nRow (*it);
            m_listPainter.m_vAvailable.erase(m_listPainter.m_vAvailable.begin() + nRow);
        }
        break;

    case MULTIPLE:
        for (set<int>::const_iterator it = sSelPos.begin(), end = sSelPos.end(); it != end; ++it)
        {
            int nRow (*it);
            //int nIndex (m_listPainter.m_vAvailable[nRow]);
            //m_listPainter.getSel_().push_back(nIndex);
            m_listPainter.m_vSel.push_back(nRow);
        }
        break;

    default:
        CB_ASSERT(false);
    }

    adjustOnDataChanged();
}




void DoubleList::on_m_pDeleteB_clicked()
{
    if (m_listPainter.m_vSel.empty()) { return; }
    QItemSelectionModel* pSelMdl (m_pSelectedG->selectionModel());
    QModelIndexList lSel (pSelMdl->selection().indexes());

    set<int> sSelPos; // the rows must be sorted and the last must be processed first, so removal of elements doesn't change the row number for the remaining ones

    for (QModelIndexList::iterator it = lSel.begin(), end = lSel.end(); it != end; ++it)
    {
        int nRow (it->row());
        CB_ASSERT (nRow >= 0);
        sSelPos.insert(nRow);
    }

    remove(sSelPos);
}


void DoubleList::remove(const std::set<int>& sSelPos) // removes elements from the specified indexes
{
    for (set<int>::const_reverse_iterator it = sSelPos.rbegin(), end = sSelPos.rend(); it != end; ++it)
    {
        int nRow (*it);
        int nIndex (m_listPainter.m_vSel[nRow]);
        if (MULTIPLE != m_eSelectionMode)
        {
            vector<int>::iterator it1 (lower_bound(m_listPainter.m_vAvailable.begin(), m_listPainter.m_vAvailable.end(), nIndex));
            m_listPainter.m_vAvailable.insert(it1, nIndex);
        }
        m_listPainter.m_vSel.erase(m_listPainter.m_vSel.begin() + nRow);
    }

    adjustOnDataChanged();
}


void DoubleList::on_m_pAddAllB_clicked()
{
    switch (m_eSelectionMode)
    {
    case SINGLE_UNSORTABLE:
        m_listPainter.m_vAvailable.clear();
        m_listPainter.m_vSel.clear();
        for (int i = 0, n = cSize(m_listPainter.getAll()); i < n; ++i)
        {
            m_listPainter.m_vSel.push_back(i);
        }
        break;

    case SINGLE_SORTABLE:
        m_listPainter.m_vSel.insert(m_listPainter.m_vSel.end(), m_listPainter.m_vAvailable.begin(), m_listPainter.m_vAvailable.end());
        m_listPainter.m_vAvailable.clear();
        break;

    case MULTIPLE:
        m_listPainter.m_vSel.insert(m_listPainter.m_vSel.end(), m_listPainter.m_vAvailable.begin(), m_listPainter.m_vAvailable.end());
        break;

    default:
        CB_ASSERT(false);
    }

    adjustOnDataChanged();
}


void DoubleList::on_m_pDeleteAllB_clicked()
{
    m_listPainter.m_vSel.clear();
    if (MULTIPLE != m_eSelectionMode)
    {
        m_listPainter.m_vAvailable.clear();
        for (int i = 0, n = cSize(m_listPainter.getAll()); i < n; ++i)
        {
            m_listPainter.m_vAvailable.push_back(i);
        }
    }

    adjustOnDataChanged();
}


void DoubleList::on_m_pRestoreDefaultB_clicked()
{
    m_listPainter.reset();
    m_listPainter.m_bResultInReset = true;
    initAvailable();

    adjustOnDataChanged();
}


void DoubleList::on_m_pRestoreOpenB_clicked()
{
    m_listPainter.m_vSel = m_listPainter.m_vOrigSel;
    m_listPainter.m_bResultInReset = false;
    initAvailable();

    adjustOnDataChanged();
}



/*override*/ void DoubleList::resizeEvent(QResizeEvent*)
{
//return;
//QDialog::resizeEvent(pEvent);
/*cout << this << ": size: " << width() << "x" << height() << " m_pAvailableG: " << m_pAvailableG->width() << "x" << m_pAvailableG->height() << " m_pSelectedG: " << m_pSelectedG->width() << "x" << m_pSelectedG->height() << " sel hdr: " << m_pSelectedG->horizontalHeader()->sectionSize(0) << ", " << m_pSelectedG->horizontalHeader()->sectionSize(1) << endl;

    m_pAvailableG->resizeRowsToContents();
    m_pSelectedG->resizeRowsToContents();*/

    QTimer::singleShot(1, this, SLOT(onResizeTimer())); // !!! 2008.10.27 - this is a workaround for what appears to be a bug in Qt: resizeEvent() doesn't get called after applying a layout; as a result, resizeRowsToContents() uses incorrect sizes to figure line heights; perhaps there's no bug and there's some other way to achieve a QTableView that adjusts the row heights such that all rows fit on the screen (perhaps after Qt 4.3.1), but for now this approach seems good enough; (calling layout()->activate() might do something similar, but it's cumbersome to use)
}




void DoubleList::onResizeTimer()
{
//cout << this << ": size: " << width() << "x" << height() << " m_pAvailableG: " << m_pAvailableG->width() << "x" << m_pAvailableG->height() << " m_pSelectedG: " << m_pSelectedG->width() << "x" << m_pSelectedG->height() << " sel hdr: " << m_pSelectedG->horizontalHeader()->sectionSize(0) << ", " << m_pSelectedG->horizontalHeader()->sectionSize(1) << endl;

    resizeRows();
}


void DoubleList::onAvlDoubleClicked(const QModelIndex& index)
{
    emit avlDoubleClicked(index.row());
}

