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

#include  "NoteFilterDlgImpl.h"

#include  "Helpers.h"
#include  "StoredSettings.h"
#include  "CommonData.h"

using namespace std;
using namespace pearl;



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


/*override*/ std::string NoteListElem::getText(int nCol) const
{
    if (0 == nCol)
    {
        //return m_pCommonData->getNoteLabel(m_pNote).toUtf8().data();
        return convStr(getNoteLabel(m_pNote));
    }
    return m_pNote->getDescription();
}




//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


NoteFilterDlgImpl::NoteFilterDlgImpl(CommonData* pCommonData, QWidget* pParent /*=0*/) :
        QDialog(pParent, getDialogWndFlags()),
        NoteListPainterBase(pCommonData, "<all notes>")
{
    setupUi(this);

    m_pListHldr->setLayout(new QHBoxLayout());

    for (int i = 0, n = pCommonData->getUniqueNotes().getCount(); i < n; ++i)
    {
        m_vpOrigAll.push_back(new NoteListElem(pCommonData->getUniqueNotes().get(i), m_pCommonData));
    }

    for (int i = 0, n = cSize(pCommonData->m_filter.getNotes()); i < n; ++i)
    {
        int n (pCommonData->getUniqueNotes().getPos(pCommonData->m_filter.getNotes()[i]));
        if (n >= 0)
        {
            m_vOrigSel.push_back(n);
        }
    }

    m_vSel = m_vOrigSel;

    m_pDoubleList = new DoubleList(
            *this,
            DoubleList::ADD_ALL | DoubleList::DEL_ALL | DoubleList::RESTORE_OPEN,
            DoubleList::SINGLE_UNSORTABLE,
            "Available notes",
            "Include notes",
            this);

    m_pListHldr->layout()->addWidget(m_pDoubleList);
    m_pListHldr->layout()->setContentsMargins(0, 0, 0, 0);

    int nWidth, nHeight;
    m_pCommonData->m_settings.loadNoteFilterSettings(nWidth, nHeight);
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


NoteFilterDlgImpl::~NoteFilterDlgImpl()
{
    clearPtrContainer(m_vpOrigAll);
}



void NoteFilterDlgImpl::logState(const char* /*szPlace*/) const
{
    /*cout << szPlace << ": m_filter.m_vSelDirs=" << m_pCommonData->m_filter.m_vSelDirs.size() << " m_availableDirs.m_vDirs=" << m_availableDirs.m_vDirs.size() << " m_selectedDirs.m_vSelDirs=" << m_selectedDirs.m_vDirs.size() << endl;*/
}




//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

void NoteFilterDlgImpl::on_m_pOkB_clicked()
{
    //logState("on_m_pOkB_clicked 1");
    //m_pCommonData->m_filter.m_vpSelNotes = m_selectedNotes.m_vpNotes;
    vector<const Note*> v;
    //m_pCommonData->m_filter.m_vpSelNotes.clear();
    for (int i = 0, n = cSize(m_vSel); i < n; ++i)
    {
        const NoteListElem* p (dynamic_cast<const NoteListElem*>(m_vpOrigAll[m_vSel[i]]));
        CB_ASSERT(0 != p);
        v.push_back(p->getNote());
    }
    m_pCommonData->m_filter.setNotes(v); //ttt1 in other case changing a parent window before the modal dialog was closed led to incorrect resizing; however, here it seems OK
    //logState("on_m_pOkB_clicked 2");
    m_pCommonData->m_settings.saveNoteFilterSettings(width(), height());
    accept();
}

void NoteFilterDlgImpl::on_m_pCancelB_clicked()
{
    reject();
}


void NoteFilterDlgImpl::onAvlDoubleClicked(int nRow)
{
    //m_pCommonData->m_filter.m_vpSelNotes.clear();
    vector<const Note*> v;
    const NoteListElem* p (dynamic_cast<const NoteListElem*>(m_vpOrigAll[getAvailable()[nRow]]));
    CB_ASSERT(0 != p);
    v.push_back(p->getNote());
    m_pCommonData->m_filter.setNotes(v);

    accept();
}


//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------



/*override*/ string NoteFilterDlgImpl::getTooltip(TooltipKey eTooltipKey) const
{
    switch (eTooltipKey)
    {
    case SELECTED_G: return "";//"Notes to be included";
    case AVAILABLE_G: return "";//"Available notes";
    case ADD_B: return "Add selected note(s)";
    case DELETE_B: return "Remove selected note(s)";
    case ADD_ALL_B: return "Add all notes";
    case DELETE_ALL_B: return "Remove all notes";
    case RESTORE_DEFAULT_B: return "";
    case RESTORE_OPEN_B: return "Restore lists to the configuration they had when the window was open";
    default: CB_ASSERT(false);
    }
}


/*override*/ void NoteFilterDlgImpl::reset()
{
    CB_ASSERT(false);
}


void NoteFilterDlgImpl::onHelp()
{
    openHelp("170_note_filter.html");
}



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


/*override*/ std::string NoteListPainterBase::getColTitle(int nCol) const
{
    switch (nCol)
    {
    case 0: return "L";
    case 1: return "Note";
    default: return "???";
    }
}



/*override*/ void NoteListPainterBase::getColor(int nIndex, int nColumn, bool bSubList, QColor& bckgColor, QColor& penColor, double& dGradStart, double& dGradEnd) const
{
    const NoteListElem* p (dynamic_cast<const NoteListElem*>(getAll()[nIndex]));
    CB_ASSERT(0 != p);
    const Note* pNote (p->getNote());

    {
        const SubList& v (getAvailable());
        if (m_vpAvail.size() != v.size()) // !!! there's no need for a "dirty" flag; after the content of getAvailable() changes, a paint is executed, and when it gets here sizes will be different
        {
            m_vpAvail.clear();
            for (int i = 0; i < cSize(v); ++i)
            {
                const NoteListElem* p (dynamic_cast<const NoteListElem*>(getAll()[v[i]]));
                m_vpAvail.push_back(p->getNote());
            }
        }
    }

    {
        const SubList& v (getSel());
        if (m_vpSel.size() != v.size())
        {
            m_vpSel.clear();
            for (int i = 0; i < cSize(v); ++i)
            {
                const NoteListElem* p (dynamic_cast<const NoteListElem*>(getAll()[v[i]]));
                m_vpSel.push_back(p->getNote());
            }
        }
    }

    if (0 == nColumn)
    {
        if (Note::ERR == pNote->getSeverity())
        {
            penColor = ERROR_PEN_COLOR();
        }
        else if (Note::SUPPORT == pNote->getSeverity())
        {
            penColor = SUPPORT_PEN_COLOR();
        }
    }

    m_pCommonData->getNoteColor(*pNote, bSubList ? m_vpSel : m_vpAvail, bckgColor, dGradStart, dGradEnd);
}


// positive values are used for fixed widths, while negative ones are for "stretched"
/*override*/ int NoteListPainterBase::getColWidth(int nCol) const
{
    switch (nCol)
    {
    case 0: return CELL_WIDTH + 10;
    case 1: return -1;
    }
    CB_ASSERT(false);
}


/*override*/ int NoteListPainterBase::getHdrHeight() const
{
    return CELL_HEIGHT;
}


/*override*/ Qt::Alignment NoteListPainterBase::getAlignment(int nCol) const
{
    if (0 == nCol)
    {
        //return Qt::AlignTop | Qt::AlignHCenter;
        return Qt::AlignVCenter | Qt::AlignHCenter;
    }
    return Qt::AlignTop | Qt::AlignLeft;
}


