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


#include  <sstream>
#include  <algorithm>

#include  "Notes.h"

#include  "Helpers.h"

using namespace std;
using namespace pearl;


/*static*/ Notes::NoteSet Notes::s_spAllNotes;

/*static*/ vector<const Note*> Notes::s_vpErrNotes;
/*static*/ vector<const Note*> Notes::s_vpWarnNotes;
/*static*/ vector<const Note*> Notes::s_vpSuppNotes;

/*static*/ vector<const Note*> Notes::s_vpAllNotes;


static const char* s_szPlaceholderDescr ("<Placeholder for a note that can no longer be found, most likely as a result of a software upgrade. You should rescan the file.>");

// to be used by serialization: if a description is no longer found, the note gets replace with a default, "missing", one
/*static*/ const Note* Notes::getMissingNote()
{
    static Note::SharedData sd (Note::SUPPORT, s_szPlaceholderDescr); // !!! m_nLabelIndex is initialized to -1, which will result in an empty label;
    static Note note (sd, -1);
    return &note;
}


/*static*/ const Note* Notes::getMaster(const Note* p)
{
    const Note* q (getNote(p->getDescription()));
    if (0 != q) { return q; }

    q = getMissingNote();
    CB_ASSERT (p->m_pSharedData == q->m_pSharedData);
    return q;
}



// destoys the pointers added by addToDestroyList(); to be called after loading is complete;
/*static*/void Notes::clearDestroyList()
{
    clearPtrContainer(s_spDestroyList);
}

/*static*/ set<Note::SharedData*> Notes::s_spDestroyList;

/*static*/ void Notes::addNote(Note* p)
{
    CB_ASSERT (0 == s_spAllNotes.count(p));
    switch (p->getSeverity())
    {
    case Note::ERR: p->m_pSharedData->m_nLabelIndex = cSize(s_vpErrNotes); s_vpErrNotes.push_back(p); break;
    case Note::WARNING: p->m_pSharedData->m_nLabelIndex = cSize(s_vpWarnNotes); s_vpWarnNotes.push_back(p); break;
    case Note::SUPPORT: p->m_pSharedData->m_nLabelIndex = cSize(s_vpSuppNotes); s_vpSuppNotes.push_back(p); break;
    default: CB_ASSERT (false);
    }

    p->m_pSharedData->m_nNoteId = cSize(s_vpAllNotes);
    s_vpAllNotes.push_back(p);

    s_spAllNotes.insert(p);
}


//ttt1 if one of the addNote() is missing, the program just crashes instead of showing an assertion; the reason seems to be that the UI will ask for the color of an invalid note;
/*static*/ void Notes::initVec()
{
    static bool s_bInit (false);
    if (s_bInit) { return; }
    s_bInit = true;

    // audio
    addNote(&Notes::twoAudio()); // e
    addNote(&Notes::lowQualAudio()); // w
    addNote(&Notes::noAudio()); // e
    addNote(&Notes::vbrUsedForNonMpg1L3()); // w
    addNote(&Notes::incompleteFrameInAudio()); // e
    addNote(&Notes::validFrameDiffVer()); // e
    addNote(&Notes::validFrameDiffLayer()); // e
    addNote(&Notes::validFrameDiffMode()); // e
    addNote(&Notes::validFrameDiffFreq()); // e
    addNote(&Notes::validFrameDiffCrc()); // e
    addNote(&Notes::audioTooShort()); // e
    addNote(&Notes::diffBitrateInFirstFrame()); // e
    addNote(&Notes::noMp3Gain()); // w
    addNote(&Notes::untestedEncoding()); // s

    // xing
    addNote(&Notes::twoLame()); // e
    addNote(&Notes::xingAddedByMp3Fixer()); // e
    addNote(&Notes::xingFrameCountMismatch()); // e
    addNote(&Notes::twoXing()); // e
    addNote(&Notes::xingNotBeforeAudio()); // e
    addNote(&Notes::incompatXing()); // e
    addNote(&Notes::missingXing()); // w

    // vbri
    addNote(&Notes::twoVbri());
    addNote(&Notes::vbriFound());
    addNote(&Notes::foundVbriAndXing()); // w

    // id3 v2
    addNote(&Notes::id3v2FrameTooShort()); // e
    addNote(&Notes::id3v2InvalidName()); // e
    addNote(&Notes::id3v2UnsuppFlags1()); // s
    addNote(&Notes::id3v2UnsuppFlags2()); // s
    addNote(&Notes::id3v2IncorrectFlg1()); // w
    addNote(&Notes::id3v2IncorrectFlg2()); // w
    addNote(&Notes::id3v2TextError()); // e
    addNote(&Notes::id3v2UnsuppVer()); // s
    addNote(&Notes::id3v2UnsuppFlag()); // s
    addNote(&Notes::id3v2HasLatin1NonAscii()); // w
    addNote(&Notes::id3v2EmptyTcon());
    addNote(&Notes::id3v2MultipleFramesWithSameName());
    addNote(&Notes::id3v2DuplicatePopm());

    // apic
    addNote(&Notes::id3v2NoApic()); // w
    addNote(&Notes::id3v2CouldntLoadPic()); // w
    //addNote(&Notes::id3v2LinkNotSupported()); // s
    addNote(&Notes::id3v2NotCoverPicture()); // w
    addNote(&Notes::id3v2UnsupApicTextEnc());
    addNote(&Notes::id3v2LinkInApic());
    addNote(&Notes::id3v2ErrorLoadingApic());
    addNote(&Notes::id3v2ErrorLoadingApicTooShort());
    addNote(&Notes::id3v2PictDescrIgnored());
    addNote(&Notes::id3v2DuplicatePic());
    addNote(&Notes::id3v2MultipleApic());

    // id3 v2.3.0
    addNote(&Notes::noId3V230()); // w
    addNote(&Notes::twoId3V230()); // e
    addNote(&Notes::bothId3V230_V240()); // w
    addNote(&Notes::id3v230AfterAudio()); // e
    addNote(&Notes::id3v230UnsuppText()); // s

    // id3 v2.4.0
    addNote(&Notes::twoId3V240()); // e
    addNote(&Notes::id3v240FrameTooShort());
    addNote(&Notes::id3v240IncorrectSynch());
    addNote(&Notes::id3v240UnsuppText());
    addNote(&Notes::id3v240DeprTyerAndTdrc());
    addNote(&Notes::id3v240DeprTyer());
    addNote(&Notes::id3v240DeprTdatAndTdrc());
    addNote(&Notes::id3v240DeprTdat());

    // id3 v1
    addNote(&Notes::onlyId3V1());
    addNote(&Notes::id3v1BeforeAudio());
    addNote(&Notes::id3v1TooShort());
    addNote(&Notes::twoId3V1());
    //addNote(&Notes::zeroInId3V1());
    addNote(&Notes::mixedPaddingInId3V1());
    addNote(&Notes::mixedFieldPaddingInId3V1());
    addNote(&Notes::id3v1InvalidName());
    addNote(&Notes::id3v1InvalidArtist());
    addNote(&Notes::id3v1InvalidAlbum());
    addNote(&Notes::id3v1InvalidYear());
    addNote(&Notes::id3v1InvalidComment());

    // broken
    addNote(&Notes::brokenAtTheEnd());
    addNote(&Notes::brokenInTheMiddle());

    // unsupp
    addNote(&Notes::unsupportedFound());

    // trunc
    addNote(&Notes::truncAudioWithWholeFile());
    addNote(&Notes::truncAudio());

    // unknown
    addNote(&Notes::unknTooShort());
    addNote(&Notes::unknownAtTheEnd());
    addNote(&Notes::unknownInTheMiddle());
    addNote(&Notes::foundNull());

    // lyrics
    addNote(&Notes::lyrTooShort());
    addNote(&Notes::twoLyr());
    addNote(&Notes::lyricsNotSupported());

    // ape
    addNote(&Notes::apeItemTooShort());
    addNote(&Notes::apeFlagsNotSupported());
    addNote(&Notes::apeItemTooBig());
    addNote(&Notes::apeMissingTerminator());
    addNote(&Notes::apeUnsupported());
    addNote(&Notes::apeFoundFooter());
    addNote(&Notes::apeTooShort());
    addNote(&Notes::apeFoundHeader());
    addNote(&Notes::apeHdrFtMismatch());
    addNote(&Notes::twoApe());

    // misc
    addNote(&Notes::fileWasChanged());
    addNote(&Notes::noInfoTag());
    addNote(&Notes::tooManyTraceNotes());
    addNote(&Notes::tooManyNotes());
    addNote(&Notes::tooManyStreams());


    qDebug("%d errors, %d warnings, %d support notes", cSize(s_vpErrNotes), cSize(s_vpWarnNotes), cSize(s_vpSuppNotes));
}




/*static*/ const Note* Notes::getNote(const std::string& strDescr)
{
    initVec();

    Note::SharedData d (strDescr.c_str()); // !!! sev doesn't matter
    Note n (d);
    NoteSet::iterator it;
    it= (s_spAllNotes.find(&n));
    if (s_spAllNotes.end() == it)
    {
        if (strDescr == s_szPlaceholderDescr)
        {
            return getMissingNote();
        }

        return 0;
    }
    return *it;
}


/*static*/ const Note* Notes::getNote(int n) // returns 0 if n is out of range
{
    initVec();

    if (n < 0 || n >= cSize(s_vpAllNotes)) { return 0; }
    return s_vpAllNotes[n];
}


/*static*/ const std::vector<const Note*>& Notes::getAllNotes()
{
    initVec();
    return s_vpAllNotes;
}


/*static*/ const vector<int>& Notes::getDefaultIgnoredNoteIds()
{
    initVec();

    static bool bInit (false);
    static vector<int> v;
    if (!bInit)
    {
        bInit = true;
        //v.push_back(zeroInId3V1().getNoteId());
        v.push_back(mixedPaddingInId3V1().getNoteId());
        v.push_back(mixedFieldPaddingInId3V1().getNoteId());
        v.push_back(lyricsNotSupported().getNoteId());
        v.push_back(tooManyTraceNotes().getNoteId());
        v.push_back(tooManyNotes().getNoteId());
    }

    return v;
}



//============================================================================================================
//============================================================================================================
//============================================================================================================



//ttt1 maybe new type for Note::Severity: BROKEN, which is basically the same as ERR, but shown in UI with a different color

//ttt1 maybe new type for Note::Severity: INFO, to be used for searches; normally they are "ignored", but can be used to search for, e.g., "CBR files"

//======================================================================================================
//======================================================================================================


Note::Note(const Note& note, std::streampos pos, const std::string& strDetail /*= ""*/) :
        m_pSharedData(note.m_pSharedData),
        m_pos(pos),
        m_strDetail(strDetail)
{
}

Note::Note(SharedData& sharedData, std::streampos pos, const std::string& strDetail /*= ""*/) :
        m_pSharedData(&sharedData),
        m_pos(pos),
        m_strDetail(strDetail)
{
}

Note::Note(SharedData& sharedData) :
        m_pSharedData(&sharedData),
        m_pos(-1)
{
}

Note::~Note()
{
    //qDebug("destroyed note at %p", this);
}

//ttt1 maybe get rid of some/most ser-specific contructors, revert const changes, and call real constructors from the parent (adding serialization as member functions required switching from references to pointers and from const to non-const data members)



bool Note::operator==(const Note& other) const
{
    return
        m_pSharedData == other.m_pSharedData &&
        m_pos == other.m_pos &&
        m_strDetail == other.m_strDetail;
}

// returns an empty string for an invalid position (i.e. one initialized from -1)
string Note::getPosHex() const
{
    if (-1 == m_pos) { return ""; }
    ostringstream s;
    s << "0x" << hex << m_pos;
    return s.str();
}




//======================================================================================================
//======================================================================================================


NoteColl::~NoteColl()
{
    pearl::clearPtrContainer(m_vpNotes);
}



void NoteColl::add(Note* pNote)
{
    if (Note::TRACE == pNote->getSeverity())
    {
        if (m_nMaxTrace == m_nTraceCount)
        {
            m_vpNotes.push_back(new Note(Notes::tooManyTraceNotes(), -1));
            ++m_nTraceCount;
        }

        if (m_nTraceCount > m_nMaxTrace)
        {
            delete pNote;
            return;
        }

        ++m_nTraceCount;
    }
    else
    {
        if (200 == m_nCount)
        {
            m_vpNotes.push_back(new Note(Notes::tooManyNotes(), -1));
            ++m_nCount;
        }

        if (m_nCount > 200)
        {
            delete pNote;
            return;
        }

        ++m_nCount;
    }


    trace(pNote->getPosHex() + string(": ") + pNote->getDescription());
    if (!pNote->getDetail().empty())
    {
        trace(pNote->getDetail()); // ttt2 perhaps log the description only if the detail is empty (so strDetail would be expected to hold all the info in strDescription)
    }

    // try to avoid adding duplicates by comparing pNote to the last 10 notes
    for (int i = 10, n = cSize(m_vpNotes) - 1; i > 0 && n >= 0; --i, --n)
    {
        const Note* pLast (m_vpNotes[n]);
        if (*pLast == *pNote)
        {
            delete pNote;
            return;
        }
    }

    m_vpNotes.push_back(pNote);
}



void NoteColl::sort()
{
    std::sort(m_vpNotes.begin(), m_vpNotes.end(), CmpNotePtrById()); // !!! needed when applying filters
}


void NoteColl::removeTraceNotes()
{
    vector<Note*> v;
    for (int i = 0, n = cSize(m_vpNotes); i < n; ++i)
    {
        Note* p (m_vpNotes[i]);
        if (Note::TRACE == p->getSeverity())
        {
            delete p;
        }
        else
        {
            v.push_back(p);
        }
    }
    m_vpNotes.swap(v);
}



bool Notes::CompNoteByName::operator()(const Note* p1, const Note* p2) const
{
    return strcmp(p1->getDescription(), p2->getDescription()) < 0;
}

//ttt1 perhaps change color codes to one per group (audio, xing, vbri, ...) and either drop the distinction between errors and warnings or use uppercase labels for errors and / or use a checkered/split background, red frame, or something else
//ttt1 if not doing the above and users want to add their own notes, perhaps add "info" notes


