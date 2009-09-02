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

/*static*/ vector<vector<const Note*> > Notes::s_vpNotesByCateg (int(Note::CATEG_CNT));
/*static*/ vector<const Note*> Notes::s_vpAllNotes;


static const char* s_szPlaceholderDescr ("<Placeholder for a note that can no longer be found, most likely as a result of a software upgrade. You should rescan the file.>");

// to be used by serialization: if a description is no longer found, the note gets replace with a default, "missing", one
/*static*/ const Note* Notes::getMissingNote()
{
    static Note::SharedData sd (Note::SUPPORT, Note::CUSTOM, s_szPlaceholderDescr); // !!! m_nLabelIndex is initialized to -1, which will result in an empty label;
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
    int nCateg (p->getCategory());
    CB_ASSERT (0 <= nCateg && nCateg < Note::CATEG_CNT);
    p->m_pSharedData->m_nLabelIndex = cSize(s_vpNotesByCateg[nCateg]);
    s_vpNotesByCateg[nCateg].push_back(p);

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
    addNote(&Notes::lowQualAudio()); // w //ttt0 Low quality MPEG audio stream (see Configuration>Quality thresholds) 
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
    addNote(&Notes::id3v2IncorrectFlg1()); // w
    addNote(&Notes::id3v2IncorrectFlg2()); // w
    addNote(&Notes::id3v2TextError()); // e
    addNote(&Notes::id3v2HasLatin1NonAscii()); // w
    addNote(&Notes::id3v2EmptyTcon()); // w
    addNote(&Notes::id3v2MultipleFramesWithSameName()); // w
    addNote(&Notes::id3v2PaddingTooLarge()); // w
    addNote(&Notes::id3v2UnsuppVer()); // s
    addNote(&Notes::id3v2UnsuppFlag()); // s
    addNote(&Notes::id3v2UnsuppFlags1()); // s
    addNote(&Notes::id3v2UnsuppFlags2()); // s
    addNote(&Notes::id3v2DuplicatePopm()); //s

    // apic
    addNote(&Notes::id3v2NoApic()); // w
    addNote(&Notes::id3v2CouldntLoadPic()); // w
    //addNote(&Notes::id3v2LinkNotSupported()); // s
    addNote(&Notes::id3v2NotCoverPicture()); // w
    addNote(&Notes::id3v2ErrorLoadingApic()); // w
    addNote(&Notes::id3v2ErrorLoadingApicTooShort()); // w
    addNote(&Notes::id3v2DuplicatePic()); // e
    addNote(&Notes::id3v2MultipleApic()); // w
    addNote(&Notes::id3v2UnsupApicTextEnc()); //s
    addNote(&Notes::id3v2LinkInApic()); //s
    addNote(&Notes::id3v2PictDescrIgnored()); //s

    // id3 v2.3.0
    addNote(&Notes::noId3V230()); // w
    addNote(&Notes::twoId3V230()); // e
    addNote(&Notes::bothId3V230_V240()); // w
    addNote(&Notes::id3v230AfterAudio()); // e
    addNote(&Notes::id3v230UnsuppText()); // s

    // id3 v2.4.0
    addNote(&Notes::twoId3V240()); // e
    addNote(&Notes::id3v240FrameTooShort()); // e
    addNote(&Notes::id3v240IncorrectSynch()); // w
    addNote(&Notes::id3v240DeprTyerAndTdrc()); // w
    addNote(&Notes::id3v240DeprTyer()); // w
    addNote(&Notes::id3v240DeprTdatAndTdrc()); // w
    addNote(&Notes::id3v240DeprTdat()); // w
    addNote(&Notes::id3v240UnsuppText()); // s

    // id3 v1
    addNote(&Notes::onlyId3V1()); // w
    addNote(&Notes::id3v1BeforeAudio()); // w
    addNote(&Notes::id3v1TooShort()); // e
    addNote(&Notes::twoId3V1()); // e
    //addNote(&Notes::zeroInId3V1());
    addNote(&Notes::mixedPaddingInId3V1()); // w
    addNote(&Notes::mixedFieldPaddingInId3V1()); // w
    addNote(&Notes::id3v1InvalidName()); // e
    addNote(&Notes::id3v1InvalidArtist()); // e
    addNote(&Notes::id3v1InvalidAlbum()); // e
    addNote(&Notes::id3v1InvalidYear()); // e
    addNote(&Notes::id3v1InvalidComment()); // e

    // broken
    addNote(&Notes::brokenAtTheEnd()); // e
    addNote(&Notes::brokenInTheMiddle()); // e

    // trunc
    addNote(&Notes::truncAudioWithWholeFile()); // e
    addNote(&Notes::truncAudio()); // e

    // unknown
    addNote(&Notes::unknTooShort()); // w
    addNote(&Notes::unknownAtTheEnd()); // e
    addNote(&Notes::unknownInTheMiddle()); // e
    addNote(&Notes::foundNull()); // w

    // lyrics
    addNote(&Notes::lyrTooShort()); // e
    addNote(&Notes::twoLyr()); // s
    addNote(&Notes::invalidLyr()); // e
    addNote(&Notes::duplicateFields()); // s
    addNote(&Notes::imgInLyrics()); // s
    addNote(&Notes::infInLyrics()); // s

    // ape
    addNote(&Notes::apeItemTooShort()); // e
    addNote(&Notes::apeItemTooBig()); // e
    addNote(&Notes::apeMissingTerminator()); // e
    addNote(&Notes::apeFoundFooter()); // e
    addNote(&Notes::apeTooShort()); // e
    addNote(&Notes::apeFoundHeader()); // e
    addNote(&Notes::apeHdrFtMismatch()); // e
    addNote(&Notes::twoApe()); // s
    addNote(&Notes::apeFlagsNotSupported()); // s
    addNote(&Notes::apeUnsupported()); // s

    // misc
    addNote(&Notes::fileWasChanged()); // w
    addNote(&Notes::noInfoTag()); // w //ttt0 perhaps change to "No SUPPORTED tag found that is capable of storing song information."
    addNote(&Notes::tooManyTraceNotes()); // w
    addNote(&Notes::tooManyNotes()); // w
    addNote(&Notes::tooManyStreams()); // w
    addNote(&Notes::unsupportedFound()); // w
    addNote(&Notes::rescanningNeeded()); // w

    {
        CB_ASSERT (Note::CUSTOM == Note::CATEG_CNT - 1);

        for (int i = 1; i < cSize(s_vpAllNotes); ++i)
        {
            const Note* p1 (s_vpAllNotes[i - 1]);
            const Note* p2 (s_vpAllNotes[i]);
            CB_ASSERT (p1->getCategory() <= p2->getCategory());
            CB_ASSERT (p1->getNoteId() <= p2->getNoteId());
        }
    }
//    qDebug("%d errors, %d warnings, %d support notes", cSize(s_vpErrNotes), cSize(s_vpWarnNotes), cSize(s_vpSuppNotes));
}


//ttt0 warn that file has multiple pictures, so will get deleted

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
        //v.push_back(lyricsNotSupported().getNoteId());
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


bool NoteColl::hasFastSaveWarn() const
{
    for (int i = cSize(m_vpNotes) - 1; i >= 0; --i)
    {
        Note* pNote (m_vpNotes[i]);
        if (*pNote == Notes::rescanningNeeded()) { return true; }
    }

    return false;
}


void NoteColl::addFastSaveWarn()
{
    if (hasFastSaveWarn()) { return; }
    add(new Note(Notes::rescanningNeeded(), -1));
}


void NoteColl::removeNotes(const std::streampos& posFrom, const std::streampos& posTo) // removes notes with addresses in the given range; posFrom is included, but posTo isn't
{
    for (int i = cSize(m_vpNotes) - 1; i >= 0; --i)
    {
        Note* pNote (m_vpNotes[i]);
        if (pNote->getPos() >= posFrom && pNote->getPos() < posTo)
        {
            delete pNote;
            m_vpNotes.erase(m_vpNotes.begin() + i);
        }
    }
}





bool Notes::CompNoteByName::operator()(const Note* p1, const Note* p2) const
{
    return strcmp(p1->getDescription(), p2->getDescription()) < 0;
}

