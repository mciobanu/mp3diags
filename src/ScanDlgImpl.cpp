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


#include  <QHeaderView>

#include  "ScanDlgImpl.h"

#include  "CommonData.h"
#include  "CheckedDir.h"


using namespace std;
using namespace pearl;



ScanDlgImpl::ScanDlgImpl(QWidget* pParent, CommonData* pCommonData) : QDialog(pParent, getDialogWndFlags()), Ui::ScanDlg(), m_pCommonData(pCommonData)
{
    setupUi(this);

    m_pDirModel = new CheckedDirModel(this, CheckedDirModel::USER_CHECKABLE);

    m_pDirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::Drives);
    m_pDirModel->setSorting(QDir::IgnoreCase);
    m_pDirsT->setModel(m_pDirModel);
    m_pDirsT->expand(m_pDirModel->index("/"));
    m_pDirsT->header()->hide();
    m_pDirsT->header()->setStretchLastSection(false); m_pDirsT->header()->setResizeMode(0, QHeaderView::ResizeToContents);

    //m_pDirModel->setDirs(pCommonData->getIncludeDirs(), pCommonData->getExcludeDirs(), m_pDirsT);
    //m_pDirModel->expandNodes(m_pDirsT);
    QTimer::singleShot(1, this, SLOT(onShow()));

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
}


ScanDlgImpl::~ScanDlgImpl()
{
}


void ScanDlgImpl::onShow()
{
    m_pDirModel->setDirs(m_pCommonData->getIncludeDirs(), m_pCommonData->getExcludeDirs(), m_pDirsT); // !!! needed here because on the constructor the tree view doesn't have the right size; //ttt1 perhaps do the same in SessionEditorDlgImpl / see which approach is better
}

// if returning true, it also calls CommonData::setDirs()
bool ScanDlgImpl::run(bool& bForce)
{
    if (QDialog::Accepted != exec()) { return false; }

    bForce = m_pForceScanCkB->isChecked();
    m_pCommonData->setDirectories(m_pDirModel->getCheckedDirs(), m_pDirModel->getUncheckedDirs());
    return true;
}


void ScanDlgImpl::onHelp()
{
    // openHelp("index.html");
}

