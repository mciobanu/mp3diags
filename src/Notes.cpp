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
#include  <vector>
#include  <algorithm>

#include  "Notes.h"

#include  "Helpers.h"

using namespace std;
using namespace pearl;


/*static*/ Notes::NoteSet Notes::s_spAllNotes;

/*static*/ vector<vector<const Note*> > Notes::s_vpNotesByCateg (static_cast<int>(Note::CATEG_CNT));
/*static*/ vector<const Note*> Notes::s_vpAllNotes;


static const char* s_szPlaceholderDescr (QT_TRANSLATE_NOOP("Notes", "<Placeholder for a note that can no longer be found, most likely as a result of a software upgrade. You should rescan the file.>"));

// to be used by serialization: if a description is no longer found, the note gets replace with a default, "missing", one
/*static*/ const Note* Notes::getMissingNote()
{
    static Note::SharedData sd (Note::SUPPORT, Note::CUSTOM, s_szPlaceholderDescr, false); // !!! m_nLabelIndex is initialized to -1, which will result in an empty label;
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


//ttt2 if one of the addNote() is missing, the program just crashes instead of showing an assertion; the reason seems to be that the UI will ask for the color of an invalid note;
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
    addNote(&Notes::id3v2EmptyTag()); //w // ttt2 perhaps move up in a new release, so it isn't shown after support notes; better: assign ids to support notes at the end of the alphabet;
    addNote(&Notes::id3v2EmptyTextFrame()); //w // ttt2 perhaps move up in a new release, so it isn't shown after support notes; better: assign ids to support notes at the end of the alphabet;

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
    addNote(&Notes::id3v230UsesUtf8()); // w
    addNote(&Notes::id3v230UnsuppText()); // s
    addNote(&Notes::id3v230CantReadFrame()); // e

    // id3 v2.4.0
    addNote(&Notes::twoId3V240()); // e
    addNote(&Notes::id3v240CantReadFrame()); // e
    addNote(&Notes::id3v240IncorrectSynch()); // w
    addNote(&Notes::id3v240DeprTyerAndTdrc()); // w
    addNote(&Notes::id3v240DeprTyer()); // w
    addNote(&Notes::id3v240DeprTdatAndTdrc()); // w
    addNote(&Notes::id3v240IncorrectDli()); // w
    addNote(&Notes::id3v240IncorrectFrameSynch()); // w
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
    //addNote(&Notes::imgInLyrics()); // s
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
    addNote(&Notes::noInfoTag()); // w
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


//ttt2 perhaps warn that file has multiple pictures, so will get deleted; probably like unsupportedFound; anyway after deciding on some standard way to tell the user about features and limitations; a class is probably a better answer than the current approach of "told/warned/..." settings scattered over the config file; should not show the messages too soon one after another, should get rid of all the static variables, ...

/*static*/ const Note* Notes::getNote(const std::string& strDescr)
{
    initVec();

    Note::SharedData d (strDescr.c_str(), false); // !!! sev doesn't matter
    Note n (d);
    NoteSet::iterator it;
    it = (s_spAllNotes.find(&n));
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

#if 0
// doesn't actually do anything, but it is merely to provide context for note translation
// the content is generated by MakeArchives.sh from Notes.h
// the reason for its existence is that the use of
/*static*/ void Notes::translNoOp()
{
    //tr("Two MPEG audio streams found, but a file should have exactly one."); // AUTOTRANSL
    tr("Low quality MPEG audio stream. (What is considered \"low quality\" can be changed in the configuration dialog, under \"Quality thresholds\".)"); // AUTOTRANSL
    tr("No MPEG audio stream found."); // AUTOTRANSL
    tr("VBR with audio streams other than MPEG1 Layer III might work incorrectly."); // AUTOTRANSL
    tr("Incomplete MPEG frame at the end of an MPEG stream."); // AUTOTRANSL
    tr("Valid frame with a different version found after an MPEG stream."); // AUTOTRANSL
    tr("Valid frame with a different layer found after an MPEG stream."); // AUTOTRANSL
    tr("Valid frame with a different channel mode found after an MPEG stream."); // AUTOTRANSL
    tr("Valid frame with a different frequency found after an MPEG stream."); // AUTOTRANSL
    tr("Valid frame with a different CRC policy found after an MPEG stream."); // AUTOTRANSL
    tr("Invalid MPEG stream. Stream has fewer than 10 frames."); // AUTOTRANSL
    tr("Invalid MPEG stream. First frame has different bitrate than the rest."); // AUTOTRANSL
    tr("No normalization undo information found. The song is probably not normalized by MP3Gain or a similar program. As a result, it may sound too loud or too quiet when compared to songs from other albums."); // AUTOTRANSL
    tr("Found audio stream in an encoding other than \"MPEG-1 Layer 3\" or \"MPEG-2 Layer 3.\" While MP3 Diags understands such streams, very few tests were run on files containing them (because they are not supposed to be found inside files with the \".mp3\" extension), so there is a bigger chance of something going wrong while processing them."); // AUTOTRANSL
    tr("Two Lame headers found, but a file should have at most one of them."); // AUTOTRANSL
    tr("Xing header seems added by Mp3Fixer, which makes the first frame unusable and causes a 16-byte unknown or null stream to be detected next."); // AUTOTRANSL
    tr("Frame count mismatch between the Xing header and the audio stream."); // AUTOTRANSL
    tr("Two Xing headers found, but a file should have at most one of them."); // AUTOTRANSL
    tr("The Xing header should be located immediately before the MPEG audio stream."); // AUTOTRANSL
    tr("The Xing header should be compatible with the MPEG audio stream, meaning that their MPEG version, layer and frequency must be equal."); // AUTOTRANSL
    tr("The MPEG audio stream uses VBR but a Xing header wasn't found. This will confuse some players, which won't be able to display the song duration or to seek."); // AUTOTRANSL
    tr("Two VBRI headers found, but a file should have at most one of them."); // AUTOTRANSL
    tr("VBRI headers aren't well supported by some players. They should be replaced by Xing headers."); // AUTOTRANSL
    tr("VBRI header found alongside Xing header. The VBRI header should probably be removed."); // AUTOTRANSL
    tr("Invalid ID3V2 frame. File too short."); // AUTOTRANSL
    tr("Invalid frame name in ID3V2 tag."); // AUTOTRANSL
    tr("Flags in the first flag group that are supposed to always be 0 are set to 1. They will be ignored."); // AUTOTRANSL
    tr("Flags in the second flag group that are supposed to always be 0 are set to 1. They will be ignored."); // AUTOTRANSL
    tr("Error decoding the value of a text frame while reading an Id3V2 Stream."); // AUTOTRANSL
    tr("ID3V2 tag has text frames using Latin-1 encoding that contain characters with a code above 127. While this is valid, those frames may have their content set or displayed incorrectly by software that uses the local code page instead of Latin-1. Conversion to Unicode (UTF16) is recommended."); // AUTOTRANSL
    tr("Empty genre frame (TCON) found."); // AUTOTRANSL
    tr("Multiple frame instances found, but only the first one will be used."); // AUTOTRANSL
    tr("The padding in the ID3V2 tag is too large, wasting space. (Large padding improves the tag editor saving speed, if fast saving is enabled, so you may want to delay compacting the tag until after you're done with the tag editor.)"); // AUTOTRANSL
    tr("Unsupported ID3V2 version."); // AUTOTRANSL
    tr("Unsupported ID3V2 tag. Unsupported flag."); // AUTOTRANSL
    tr("Unsupported value for Flags1 in ID3V2 frame. (This may also indicate that the file contains garbage where it was supposed to be zero.)"); // AUTOTRANSL
    tr("Unsupported value for Flags2 in ID3V2 frame. (This may also indicate that the file contains garbage where it was supposed to be zero.)"); // AUTOTRANSL
    tr("Multiple instances of the POPM frame found in ID3V2 tag. The current version discards all the instances except the first when processing this tag."); // AUTOTRANSL
    tr("ID3V2 tag contains no frames, which is invalid. This note will disappear once you add track information in the tag editor."); // AUTOTRANSL
    tr("ID3V2 tag contains an empty text frame, which is invalid."); // AUTOTRANSL
    tr("ID3V2 tag doesn't have an APIC frame (which is used to store images)."); // AUTOTRANSL
    tr("ID3V2 tag has an APIC frame (which is used to store images), but the image couldn't be loaded."); // AUTOTRANSL
    tr("ID3V2 tag has at least one valid APIC frame (which is used to store images), but no frame has a type that is associated with an album cover."); // AUTOTRANSL
    tr("Error loading image in APIC frame."); // AUTOTRANSL
    tr("Error loading image in APIC frame. The frame is too short anyway to have space for an image."); // AUTOTRANSL
    tr("ID3V2 tag has multiple APIC frames with the same picture type."); // AUTOTRANSL
    tr("ID3V2 tag has multiple APIC frames. While this is valid, players usually use only one of them to display an image, discarding the others."); // AUTOTRANSL
    tr("Unsupported text encoding for APIC frame in ID3V2 tag."); // AUTOTRANSL
    tr("APIC frame uses a link to a file as a MIME Type, which is not supported."); // AUTOTRANSL
    tr("Picture description is ignored in the current version."); // AUTOTRANSL
    tr("No ID3V2.3.0 tag found, although this is the most popular tag for storing song information."); // AUTOTRANSL
    tr("Two ID3V2.3.0 tags found, but a file should have at most one of them."); // AUTOTRANSL
    tr("Both ID3V2.3.0 and ID3V2.4.0 tags found, but there should be only one of them."); // AUTOTRANSL
    tr("The ID3V2.3.0 tag should be the first tag in a file."); // AUTOTRANSL
    tr("ID3V2.3.0 tag contains a text frame encoded as UTF-8, which is valid in ID3V2.4.0 but not in ID3V2.3.0."); // AUTOTRANSL
    tr("Unsupported value of text frame while reading an Id3V2 Stream."); // AUTOTRANSL
    tr("Invalid ID3V2.3.0 frame. Incorrect frame size or file too short."); // AUTOTRANSL
    tr("Two ID3V2.4.0 tags found, but a file should have at most one of them."); // AUTOTRANSL
    tr("Invalid ID3V2.4.0 frame. Incorrect frame size or file too short."); // AUTOTRANSL
    tr("Invalid ID3V2.4.0 frame. Frame size is supposed to be stored as a synchsafe integer, which uses only 7 bits in a byte, but the size uses all 8 bits, as in ID3V2.3.0. This will confuse some applications"); // AUTOTRANSL
    tr("Deprecated TYER frame found in 2.4.0 tag alongside a TDRC frame."); // AUTOTRANSL
    tr("Deprecated TYER frame found in 2.4.0 tag. It's supposed to be replaced by a TDRC frame."); // AUTOTRANSL
    tr("Deprecated TDAT frame found in 2.4.0 tag alongside a TDRC frame."); // AUTOTRANSL
    tr("Deprecated TDAT frame found in 2.4.0 tag. It's supposed to be replaced by a TDRC frame."); // AUTOTRANSL
    tr("Invalid ID3V2.4.0 frame. Mismatched Data length indicator. Frame value is probably incorrect"); // AUTOTRANSL
    tr("Invalid ID3V2.4.0 frame. Incorrect unsynchronization bit."); // AUTOTRANSL
    tr("Unsupported value of text frame while reading an Id3V2.4.0 stream. It may be using an unsupported text encoding."); // AUTOTRANSL
    tr("The only supported tag found that is capable of storing song information is ID3V1, which has pretty limited capabilities."); // AUTOTRANSL
    tr("The ID3V1 tag should be located after the MPEG audio stream."); // AUTOTRANSL
    tr("Invalid ID3V1 tag. File too short."); // AUTOTRANSL
    tr("Two ID3V1 tags found, but a file should have at most one of them."); // AUTOTRANSL
    tr("ID3V1 tag contains fields padded with spaces alongside fields padded with zeroes. The standard only allows zeroes, but some tools use spaces. Even so, zero-padding and space-padding shouldn't be mixed."); // AUTOTRANSL
    tr("ID3V1 tag contains fields that are padded with spaces mixed with zeroes. The standard only allows zeroes, but some tools use spaces. Even so, one character should be used for padding for the whole tag."); // AUTOTRANSL
    tr("Invalid ID3V1 tag. Invalid characters in Name field."); // AUTOTRANSL
    tr("Invalid ID3V1 tag. Invalid characters in Artist field."); // AUTOTRANSL
    tr("Invalid ID3V1 tag. Invalid characters in Album field."); // AUTOTRANSL
    tr("Invalid ID3V1 tag. Invalid characters in Year field."); // AUTOTRANSL
    tr("Invalid ID3V1 tag. Invalid characters in Comment field."); // AUTOTRANSL
    tr("Broken stream found."); // AUTOTRANSL
    tr("Broken stream found. Since other streams follow, it is possible that players and tools will have problems using the file. Removing the stream is recommended."); // AUTOTRANSL
    tr("Truncated MPEG stream found. The cause for this seems to be that the file was truncated."); // AUTOTRANSL
    tr("Truncated MPEG stream found. Since other streams follow, it is possible that players and tools will have problems using the file. Removing the stream or padding it with 0 to reach its declared size is strongly recommended."); // AUTOTRANSL
    tr("Not enough remaining bytes to create an UnknownDataStream."); // AUTOTRANSL
    tr("Unknown stream found."); // AUTOTRANSL
    tr("Unknown stream found. Since other streams follow, it is possible that players and tools will have problems using the file. Removing the stream is recommended."); // AUTOTRANSL
    tr("File contains null streams."); // AUTOTRANSL
    tr("Invalid Lyrics stream tag. File too short."); // AUTOTRANSL
    tr("Two Lyrics tags found, but only one is supported."); // AUTOTRANSL
    tr("Invalid Lyrics stream tag. Unexpected characters found."); // AUTOTRANSL
    tr("Multiple fields with the same name were found in a Lyrics tag, but only one is supported."); // AUTOTRANSL
    tr("Currently INF fields in Lyrics tags are not fully supported."); // AUTOTRANSL
    tr("Invalid Ape Item. File too short."); // AUTOTRANSL
    tr("Ape Item seems too big. Although the size may be any 32-bit integer, 256 bytes should be enough in practice. If this note is determined to be incorrect, it will be removed in the future."); // AUTOTRANSL
    tr("Invalid Ape Item. Terminator not found for item name."); // AUTOTRANSL
    tr("Invalid Ape tag. Header expected but footer found."); // AUTOTRANSL
    tr("Not an Ape tag. File too short."); // AUTOTRANSL
    tr("Invalid Ape tag. Footer expected but header found."); // AUTOTRANSL
    tr("Invalid Ape tag. Mismatch between header and footer."); // AUTOTRANSL
    tr("Two Ape tags found, but only one is supported."); // AUTOTRANSL
    tr("Ape item flags not supported."); // AUTOTRANSL
    tr("Unsupported Ape tag. Currently a missing header or footer are not supported."); // AUTOTRANSL
    tr("The file seems to have been changed in the (short) time that passed between parsing it and the initial search for pictures. If you think that's not the case, report a bug."); // AUTOTRANSL
    tr("No supported tag found that is capable of storing song information."); // AUTOTRANSL
    tr("Too many TRACE notes added. The rest will be discarded."); // AUTOTRANSL
    tr("Too many notes added. The rest will be discarded."); // AUTOTRANSL
    tr("Too many streams found. Aborting processing."); // AUTOTRANSL
    tr("Unsupported stream found. It may be supported in the future if there's a real need for it."); // AUTOTRANSL
    tr("The file was saved using the \"fast\" option. While this improves the saving speed, it may leave the notes in an inconsistent state, so you should rescan the file."); // AUTOTRANSL
}
#endif

//============================================================================================================
//============================================================================================================
//============================================================================================================



//ttt2 maybe new type for Note::Severity: BROKEN, which is basically the same as ERR, but shown in UI with a different color

//ttt2 maybe new type for Note::Severity: INFO, to be used for searches; normally they are "ignored", but can be used to search for, e.g., "CBR files"

//======================================================================================================
//======================================================================================================


Note::Note(const Note& note, std::streampos pos, const std::string& strDetail /* = ""*/) :
        m_pSharedData(note.m_pSharedData),
        m_pos(pos),
        m_strDetail(strDetail)
{
    //char a [30]; sprintf(a, "1 Note::Note() %p", this); TRACER(a);
}

Note::Note(SharedData& sharedData, std::streampos pos, const std::string& strDetail /* = ""*/) :
        m_pSharedData(&sharedData),
        m_pos(pos),
        m_strDetail(strDetail)
{
    //char a [30]; sprintf(a, "2 Note::Note() %p", this); TRACER(a);
}

Note::Note(SharedData& sharedData) :
        m_pSharedData(&sharedData),
        m_pos(-1)
{
    //char a [30]; sprintf(a, "3 Note::Note() %p", this); TRACER(a);
}

Note::Note()
{
    //char a [30]; sprintf(a, "4 Note::Note() %p", this); TRACER(a);
}

Note::~Note()
{
    //qDebug("destroyed note at %p", this);
    //char a [30]; sprintf(a, "Note::~Note() %p", this); TRACER(a);
}

//ttt2 maybe get rid of some/most ser-specific constructors, revert const changes, and call real constructors from the parent (adding serialization as member functions required switching from references to pointers and from const to non-const data members)



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


/*static*/ const std::string Note::severityToString(Severity s) {
    if (s == ERR) return convStr(Notes::tr("ERROR"));
    if (s == WARNING) return convStr(Notes::tr("WARNING"));
    if (s == SUPPORT) return convStr(Notes::tr("SUPPORT"));
    throw std::invalid_argument(boost::lexical_cast<std::string>((int)s));
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

