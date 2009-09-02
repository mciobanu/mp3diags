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


#include  <QTreeView>
#include  <QScrollBar>

#include  "CheckedDir.h"

#include  "Helpers.h"


using namespace std;


/*override*/ QVariant CheckedDirModel::data(const QModelIndex& index, int nRole /*= Qt::DisplayRole*/) const
{
    LAST_STEP("CheckedDirModel::data()");
    if (Qt::CheckStateRole == nRole)
    {
        //QString s (getDir(index));
        QString s (filePath(index));
        bool bIsChecked (false), bIsUnchecked (false), bHasUncheckedDescendant (false);
        QString qstrClosestCheckedAncestor, qstrClosestUncheckedAncestor;

        for (int i = 0, n = cSize(m_vCheckedDirs); i < n; ++i)
        {
            QString s1 (m_vCheckedDirs[i]);

            if (s == s1)
            {
                bIsChecked = true;
            }

            if (isDescendant(s, s1))
            {
                if (isDescendant(s1, qstrClosestCheckedAncestor))
                {
                    qstrClosestCheckedAncestor = s1;
                }
            }
        }

        for (int i = 0, n = cSize(m_vUncheckedDirs); i < n; ++i)
        {
            QString s1 (m_vUncheckedDirs[i]);

            if (s == s1)
            {
                bIsUnchecked = true;
            }

            if (isDescendant(s, s1))
            {
                if (isDescendant(s1, qstrClosestUncheckedAncestor))
                {
                    qstrClosestUncheckedAncestor = s1;
                }
            }

            if (isDescendant(s1, s))
            {
                bHasUncheckedDescendant = true;
            }
        }

        if (
            !bIsChecked &&
            (bIsUnchecked || qstrClosestCheckedAncestor.isEmpty() || isDescendant(qstrClosestUncheckedAncestor, qstrClosestCheckedAncestor))
            )
        {
            return Qt::Unchecked;
        }

        return bHasUncheckedDescendant ? Qt::PartiallyChecked : Qt::Checked;
    }

    return QDirModel::data(index, nRole);
}


bool CheckedDirModel::setData(const QModelIndex& index, const QVariant& value, int nRole /*= Qt::EditRole*/)
{
    if (Qt::CheckStateRole == nRole)
    {
        bool hasClosestAncestorChecked (false);
        QString sClosestAncestor;

        QString s (filePath(index));
        //if (3 == s.size()) { return false; }
        vector<QString> v;
        // remove s and its descendants from m_vCheckedDirs, if any are there; compute sClosestAncestor;
        for (int i = 0, n = cSize(m_vCheckedDirs); i < n; ++i)
        {
            QString s1 (m_vCheckedDirs[i]);
    //qDebug("s1=%s, s=%s", s1.toUtf8().data(), s.toUtf8().data());
            if (!isDescendant(s1, s) && s != s1)
            {
                v.push_back(s1);

                if (isDescendant(s1, sClosestAncestor) && isDescendant(s, s1))
                {
                    sClosestAncestor = s1;
                    hasClosestAncestorChecked = true;
                }
            }
        }
        v.swap(m_vCheckedDirs);

        v.clear();
        // remove s and its descendants from m_vUncheckedDirs, if any are there; compute sClosestAncestor;
        for (int i = 0, n = cSize(m_vUncheckedDirs); i < n; ++i)
        {
            QString s1 (m_vUncheckedDirs[i]);
            if (!isDescendant(s1, s) && s != s1)
            {
                v.push_back(s1);

                if (isDescendant(s1, sClosestAncestor) && isDescendant(s, s1))
                {
                    sClosestAncestor = s1;
                    hasClosestAncestorChecked = false;
                }
            }
        }
        v.swap(m_vUncheckedDirs);

        Qt::CheckState eCheck ((Qt::CheckState)value.toInt());
        switch (eCheck)
        {
        case Qt::Checked:
            if (!hasClosestAncestorChecked)
            {
                m_vCheckedDirs.push_back(s);
            }
            break;

        case Qt::Unchecked:
            if (hasClosestAncestorChecked)
            {
                m_vUncheckedDirs.push_back(s);
            }
            break;

        default:
            CB_ASSERT (false);
        }

        /*for (;;) //ttt1
        {
            if all children of a checked node are unchecked, remove the node from the checked list;
            repeat until there's nothing to change;
            must check all in the tree;

            2009.04.13 - or maybe not: more directories may be added later; more importantly, the node might have files itself
        }*/

        emit layoutChanged();
        return true;
    }

    return QDirModel::setData(index, value, nRole);
}

/*QString CheckedDirModel::getDir(const QModelIndex& index) const
{
    if (!index.isValid()) { return "root"; } // !!! using "root" rather than an empty dir to distinguish between an unassigned string and one that holds the root dir
    QString s (getDir(index.parent()) + "/" + data(index).toString());
    //qDebug("%s", s.toUtf8().data());
    return s;
}
*/

// non-reflexive (isDescendant(s, s) is false)
// order: "" is above "/"; "/" is above the rest;
bool CheckedDirModel::isDescendant(const QString& s1, const QString& s2) const
{
    if (s1.isEmpty()) { return false; }

    return
        s2.isEmpty() ||
        (s2 == "/" && s1.size() > 1) || // ttt3 linux-specific
        (s1.startsWith(s2) && s1.size() > s2.size() && s1[s2.size()] == '/')
#ifndef WIN32
#else
        || (s1.startsWith(s2) && s1.size() > s2.size() && 3 == s2.size() && ':' == s1[1] && ':' == s2[1])
#endif
        ;
}


// besides assigning the vectors, also expands the tree to show them
void CheckedDirModel::setDirs(const vector<string>& vstrCheckedDirs, const vector<string>& vstrUncheckedDirs, QTreeView* pTreeView)
{
    m_vCheckedDirs = convStr(vstrCheckedDirs);
    m_vUncheckedDirs = convStr(vstrUncheckedDirs);
    expandNodes(pTreeView);
}
//ttt0 wnd: see if possible to show labels, not just drive letters
void CheckedDirModel::expandNodes(QTreeView* pTreeView)
{
    for (int i = cSize(m_vUncheckedDirs) - 1; i >= 0; --i)
    {
        expandNode(m_vUncheckedDirs[i], pTreeView);
    }

    for (int i = cSize(m_vCheckedDirs) - 1; i >= 0; --i)
    {
        expandNode(m_vCheckedDirs[i], pTreeView);
    }

    emit layoutChanged();
}


void CheckedDirModel::expandNode(const QString& s, QTreeView* pTreeView)
{
    pTreeView->expand(index("/"));
    int n (1);
    for (;;)
    {
        n = s.indexOf('/', n);
        if (-1 == n) { break; }
        pTreeView->expand(index(s.left(n)));
        ++n;
    }

//qDebug("%d %d", pTreeView->width(), pTreeView->height());
    pTreeView->scrollTo(index(s), QAbstractItemView::PositionAtCenter);
    pTreeView->horizontalScrollBar()->setValue(0); // !!! needed because PositionAtCenter scrolls horizontally as well, which is strange for dirs with large names
    //pTreeView->scrollTo(index(s), QAbstractItemView::EnsureVisible);
    //pTreeView->scrollTo(index(s), QAbstractItemView::PositionAtTop);
    //pTreeView->scrollTo(index(s), QAbstractItemView::PositionAtBottom);
}


std::vector<std::string> CheckedDirModel::getCheckedDirs() const { return convStr(m_vCheckedDirs); }
std::vector<std::string> CheckedDirModel::getUncheckedDirs() const { return convStr(m_vUncheckedDirs); }


#ifdef jiLPojiojsdjoiHJIOHIO
//#include  "CheckedDir.h"

    CheckedDirModel* pModel (new CheckedDirModel(this));
    pModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::Drives);
    //pModel->setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::Drives);
    //QStringList lst;
    //lst << "/r/temp/1/tmp2/z/pic";
    //pModel->setNameFilters(lst);
    pModel->setSorting(QDir::IgnoreCase);
    treeView->setModel(pModel);
    //treeView->setRootIndex(pModel->index("/r/temp/1/tmp2/z/pic"));
    treeView->expand(pModel->index("/"));
    treeView->expand(pModel->index("/r"));
    treeView->expand(pModel->index("/r/temp"));
    treeView->expand(pModel->index("/r/temp/1"));
    treeView->expand(pModel->index("/r/temp/1/tmp2"));
    treeView->expand(pModel->index("/r/temp/1/tmp2/z"));
    treeView->expand(pModel->index("/r/temp/1/tmp2/z/pic"));
    treeView->scrollTo(pModel->index("/r/temp/1/tmp2/z/pic", QAbstractItemView::PositionAtCenter));


#endif

