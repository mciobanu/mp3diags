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

#include  "DirFilterDlgImpl.h"

#include  "CommonData.h"
#include  "Helpers.h"
#include  "StoredSettings.h"


using namespace std;
using namespace pearl;



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


class DirListElem : public ListElem
{
    /*override*/ string getText(int /*nCol*/) const { return m_strDir; }
    string m_strDir;
    //CommonData* m_pCommonData;
public:
    DirListElem(const string& strDir) : m_strDir(strDir) {}
    const string& getDir() const { return m_strDir; }
};



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


void DirFilterDlgImpl::populateLists()
{
    const set<string>& sDirs (m_pCommonData->getAllDirs());
    if (sDirs.empty()) { return; }

    string strCommonDir;
    for (set<string>::const_iterator it = sDirs.begin(), begin = sDirs.begin(), end = sDirs.end(); it != end; ++it)
    {
        const string& strDir (*it);

        if (it == begin)
        {
            strCommonDir = strDir;
        }
        else
        {
            int j (0);
            for (int n = min(cSize(strCommonDir), cSize(strDir)); j < n; ++j)
            {
                if (strDir[j] != strCommonDir[j]) { break; }
            }
            strCommonDir.erase(j);
        }
    }

    set<string> sDirsAndParents (sDirs);
    CB_ASSERT (!strCommonDir.empty()); // ttt1 linux-specific
    string::size_type nCommonSize (strCommonDir.size());
    if (getPathSep() == strCommonDir[nCommonSize - 1]) // this condition is usually true, but it is false if all the files are in the same dir
    {
        strCommonDir.erase(nCommonSize - 1);
        --nCommonSize;
    }

    sDirsAndParents.insert(strCommonDir);

    for (set<string>::iterator it = sDirs.begin(), end = sDirs.end(); it != end; ++it)
    {
        const string& strDir(*it);
        CB_ASSERT(beginsWith(strDir, strCommonDir));
        string::size_type k (nCommonSize + 1);
        for (;;)
        {
            k = strDir.find(getPathSep(), k);
            if (string::npos == k)
            {
                break;
            }
            sDirsAndParents.insert(strDir.substr(0, k));
            ++k;
        }
    }

    for (set<string>::iterator it = sDirsAndParents.begin(), end = sDirsAndParents.end(); it != end; ++it)
    {
        m_vpOrigAll.push_back(new DirListElem(*it));
    }

    vector<string> vAll;
    vAll.insert(vAll.end(), sDirsAndParents.begin(), sDirsAndParents.end());
    for (int i = 0, n = cSize(m_pCommonData->m_filter.getDirs()); i < n; ++i)
    {
        const string& s (m_pCommonData->m_filter.getDirs()[i]);
        int k (lower_bound(vAll.begin(), vAll.end(), s) - vAll.begin());
        CB_ASSERT (k < cSize(sDirsAndParents) && vAll[k] == s);
        m_vOrigSel.push_back(k);
    }

    m_vSel = m_vOrigSel;
}




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================



DirFilterDlgImpl::DirFilterDlgImpl(CommonData* pCommonData, QWidget* pParent /*=0*/) :
        QDialog(pParent, getDialogWndFlags()),
        ListPainter("<all folders>"),
        m_pCommonData(pCommonData)
{
    setupUi(this);

    populateLists();

    m_pListHldr->setLayout(new QHBoxLayout());

    m_pDoubleList = new DoubleList(
            *this,
            DoubleList::RESTORE_OPEN,
            DoubleList::SINGLE_UNSORTABLE,
            "Available folders",
            "Include folders",
            this);

    m_pListHldr->layout()->addWidget(m_pDoubleList);
    m_pListHldr->layout()->setContentsMargins(0, 0, 0, 0);

    int nWidth, nHeight;
    m_pCommonData->m_settings.loadDirFilterSettings(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 400)
    {
        resize(nWidth, nHeight);
    }
    else
    {
        defaultResize(*this);
    }

    connect(m_pDoubleList, SIGNAL(avlDoubleClicked(int)), this, SLOT(onAvlDoubleClicked(int)));
}


DirFilterDlgImpl::~DirFilterDlgImpl()
{
    clearPtrContainer(m_vpOrigAll);
}



#if 0
void DirFilterDlgImpl::logState(const char* /*szPlace*/) const
{
    cout << szPlace << /*": m_filter.m_vSelDirs=" << m_pCommonData->m_filter.m_vSelDirs.size() << " m_availableDirs.m_vDirs=" << m_availableDirs.m_vDirs.size() << " m_selectedDirs.m_vSelDirs=" << m_selectedDirs.m_vDirs.size() <<*/ endl;
}
#endif

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------



void DirFilterDlgImpl::on_m_pOkB_clicked()
{
    vector<string> v;
    for (int i = 0, n = cSize(m_vSel); i < n; ++i)
    {
        v.push_back(dynamic_cast<const DirListElem*>(m_vpOrigAll[m_vSel[i]])->getDir());
    }
    m_pCommonData->m_filter.setDirs(v);
    m_pCommonData->m_settings.saveDirFilterSettings(width(), height());
    accept();
}

void DirFilterDlgImpl::on_m_pCancelB_clicked()
{
    reject();
}


void DirFilterDlgImpl::onAvlDoubleClicked(int nRow)
{
    vector<string> v;
    const DirListElem* p (dynamic_cast<const DirListElem*>(m_vpOrigAll[getAvailable()[nRow]]));
    CB_ASSERT(0 != p);
    v.push_back(p->getDir());
    m_pCommonData->m_filter.setDirs(v);

    accept();
}



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================





/*override*/ string DirFilterDlgImpl::getTooltip(TooltipKey eTooltipKey) const
{
    switch (eTooltipKey)
    {
    case SELECTED_G: return "";//"Notes to be included";
    case AVAILABLE_G: return "";//"Available notes";
    case ADD_B: return "Add selected folders";
    case DELETE_B: return "Remove selected folders";
    case ADD_ALL_B: return "";//"Add all folders";
    case DELETE_ALL_B: return "";//"Remove all folders";
    case RESTORE_DEFAULT_B: return "";
    case RESTORE_OPEN_B: return "Restore lists to the configuration they had when the window was open";
    default: CB_ASSERT(false);
    }
}



/*override*/ Qt::Alignment DirFilterDlgImpl::getAlignment(int /*nCol*/) const
{
    return Qt::AlignTop | Qt::AlignLeft;
}



/*override*/ void DirFilterDlgImpl::reset()
{
    CB_ASSERT(false);
}


/*override*/ int DirFilterDlgImpl::getHdrHeight() const { return CELL_HEIGHT; }


