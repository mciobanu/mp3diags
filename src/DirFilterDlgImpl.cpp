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
#include  <map>

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
    string m_strDir; // uses native separators
    //CommonData* m_pCommonData;
public:
    DirListElem(const string& strDir) : m_strDir(strDir) {}
    const string& getDir() const { return m_strDir; }
};



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

static string addSepToRoot(const string& s)
{
#ifndef WIN32
    return s;
#else //ttt2
    if (s.size() != 2) { return s; }
    return s + "\\";
#endif
}

static string removeSepFromRoot(const string& s)
{
#ifndef WIN32
    return s;
#else //ttt2
    if (s.size() != 3) { return s; }
    return s.substr(0, 2);
#endif
}



#ifndef WIN32
    static char getDrive(const string&) { return '.'; }
#else //ttt2
    static char getDrive(const string& s) { CB_ASSERT(s.size() > 2); return s[0]; }
#endif



void DirFilterDlgImpl::populateLists()
{
    TRACER("DirFilterDlgImpl::populateLists");
    const set<string>& sDirs (m_pCommonData->getAllDirs());
    if (sDirs.empty()) { return; }

    map<char, pair<string, int> > mCommonDirs;
    for (set<string>::const_iterator it = sDirs.begin(), begin = sDirs.begin(), end = sDirs.end(); it != end; ++it)
    {
        const string strDir (*it + getPathSepAsStr());

        char d (getDrive(strDir));
        if (0 == mCommonDirs.count(d))
        {
            mCommonDirs[d] = make_pair(strDir, 0);
        }
        else
        {
            string s (mCommonDirs[d].first);
            int j (0);
            for (int n = min(cSize(s), cSize(strDir)); j < n; ++j)
            {
                if (strDir[j] != s[j]) { break; }
            }

            for (; j >= 0 && getPathSep() != strDir[j]; --j) {}
            s.erase(j + 1); // it's OK for j==-1
            mCommonDirs[d] = make_pair(s, 0); // "0" doesn't matter
            //if (cSize(strCommonDir) <= 1) { break; }
        }
    }
//ttt2 strCommonDir should be drive-specific on Wnd
    set<string> sDirsAndParents (sDirs);

    for (map<char, pair<string, int> >::iterator it = mCommonDirs.begin(), end = mCommonDirs.end(); it != end; ++it)
    {
        string s (it->second.first);
        string::size_type nCommonSize (s.size());
        if (nCommonSize > 0)
        {
            CB_ASSERT (getPathSep() == s[nCommonSize - 1]);
            s.erase(nCommonSize - 1);
            --nCommonSize;
            mCommonDirs[getDrive(s)] = make_pair(s, nCommonSize);

            if (nCommonSize > 0) // this is always true on Windows
            {
                sDirsAndParents.insert(s);
            }
        }
    }

    for (set<string>::const_iterator it = sDirs.begin(), end = sDirs.end(); it != end; ++it)
    {
        const string& strDir(*it);
        CB_ASSERT(beginsWith(strDir, mCommonDirs[getDrive(strDir)].first));
        int nCommonSize (mCommonDirs[getDrive(strDir)].second);
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
        m_vpOrigAll.push_back(new DirListElem(addSepToRoot(toNativeSeparators(*it))));
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
    TRACER("DirFilterDlgImpl constr");
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

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
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
        v.push_back(fromNativeSeparators(removeSepFromRoot(dynamic_cast<const DirListElem*>(m_vpOrigAll[m_vSel[i]])->getDir())));
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
    v.push_back(fromNativeSeparators(removeSepFromRoot(p->getDir())));
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


void DirFilterDlgImpl::onHelp()
{
    openHelp("180_folder_filter.html");
}

