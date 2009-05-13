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


#ifndef CheckedDirH
#define CheckedDirH

#include  <vector>
#include  <string>

#include  <QDirModel>

class QTreeView;

class CheckedDirModel : public QDirModel
{
    /*override*/ int columnCount(const QModelIndex&) const { return 1; }

    /*override*/ Qt::ItemFlags flags(const QModelIndex& ndx) const { return QDirModel::flags(ndx) | (m_bUserCheckable ? Qt::ItemIsUserCheckable | Qt::ItemIsTristate : Qt::ItemIsTristate); }

    /*override*/ QVariant data(const QModelIndex& index, int nRole = Qt::DisplayRole) const;

    bool setData(const QModelIndex& index, const QVariant& value, int nRole /*= Qt::EditRole*/);

    // non-reflexive (isDescendant(s, s) is false)
    // order: "" is above "/"; "/" is above the rest;
    bool isDescendant(const QString& s1, const QString& s2) const;

    bool m_bUserCheckable;

    std::vector<QString> m_vCheckedDirs; // strings are not terminated with the path separator, excepting for the root
    std::vector<QString> m_vUncheckedDirs;

    void expandNode(const QString& s, QTreeView* pTreeView);

public:
    enum { NOT_USER_CHECKABLE, USER_CHECKABLE };
    CheckedDirModel(QObject* pParent, bool bUserCheckable) : QDirModel(pParent), m_bUserCheckable(bUserCheckable) {}

    void setDirs(const std::vector<std::string>& vstrCheckedDirs, const std::vector<std::string>& vstrUncheckedDirs, QTreeView* pTreeView); // besides assigning the vectors, also expands the tree to show them
    std::vector<std::string> getCheckedDirs() const;
    std::vector<std::string> getUncheckedDirs() const;
    void expandNodes(QTreeView* pTreeView);
};




#endif


