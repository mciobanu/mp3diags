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


#ifndef NotesH
#define NotesH

#include  <string>
#include  <vector>
#include  <set>
#include  <iosfwd>
#include  <stdexcept>

#include  <QApplication> // for translation

#include  "SerSupport.h"
#include  <boost/serialization/split_member.hpp>
#include  <boost/lexical_cast.hpp>

/*
Stores a message that is added while parsing a file. Multiple messages may be added, depending on the issues found.

Notes are added to an Mp3Handler:
    1) while creating (or attempting to create) new streams
    2) after the streams were created, addressing the relations between streams (this may include non-existing streams that should have been present).

Calling MP3_CHECK(), MP3_THROW() or MP3_NOTE() adds a note to an Mp3Handler.

Notes have a "Severity":
    ERR: Something was seriously wrong and it should be reported to the user.
    WARNING: There may be some issues, in some cases.
    SUPPORT: A stream tries to use a feature that is not currently supported. The program needs an update.
    TRACE: Meant for debugging only. Most operations generate TRACE notes, so that it should be easy to follow the steps that led to an unexpected situation while processing a given file.

A note may be attached to a stream. This happens implicitly, by using the "pos" value: if this is -1, the note is non-attached; otherwise it is attached to the stream that contains that address. As a result, it is possible for ERR and SUPPORT notes to be attached to apparently unrelated streams (usually they would be "Unknown" streams). This happens because they belong to streams that can't be constructed, because they are either broken or not currently supported. When the construction fails, "Unknown" streams are most likely to be constructed instead, and the note gets attached to them.

Note that not all exceptions thrown on streams' constructors with MP3_CHECK() or MP3_THROW are ERR. Indeed, most are TRACE. This is because the algorithm for identifying the streams simply tries to construct all known streams (for a given position). If the actual stream doesn't match the constructor, a TRACE exception is thrown and the constructor for the next stream type is called. On the other hand, if the stream can be identified as matching by the constructor but has errors preventing the constructor to complete successfully, ERR is the severity that should be used by the note. (And similarly for not supported features, but depending on the particular case, it may be decided to continue loading with that support missing and partial functionality.)

SUPPORT notes end up in the UI, so care should be taken to avoid false positives (e.g. if "MPEG 2.5 not supported" is thrown as SUPPORT, we should check that an MPEG 2.5 stream is present, not just a few bits at the beginning"; anyway, for this reason, "MPEG 2.5 not supported" is thrown as TRACE).

Since SUPPORT notes are usually attached to "Unknown" streams, the original stream (which couldn't be constructed because of the unsupported feature) should be identifiable from the error message. The same is true for ERR exceptions thrown on the constructors, but it isn't necessary for the other notes (at least for those that are linked to streams).

Reformulated: there are 2 situations for exceptions in constructors:
    1) obviously not the stream that was expected, no need to report it; use TRACE;
    2) the beginning was OK, several checks went OK, but then something went wrong; should be reported, and the type of the stream should be specified in the message; use ERR or SUPPORT;

"Detail" is better avoided, because it takes space. It is supposed to provide context-dependent detail for "description", but most of the time the only detail that matters is the position, which is already available. The code that uses Note should call getDetail() first, and if that returns an empty string use getDescription() to present the user with all the information available.

*/
struct Note
{
    enum Severity { ERR, WARNING, SUPPORT, TRACE }; // !!! the reason "ERR" is used (and not "ERROR") is that "ERROR" is a macro in MSVC
    enum Category { AUDIO, XING, VBRI, ID3V2, APIC, ID3V230, ID3V240, ID3V1, BROKEN, TRUNCATED, UNKNOWN, LYRICS, APE, MISC, CUSTOM, CATEG_CNT }; // !!! CUSTOM must be just before CATEG_CNT (which is just a counter)

    static const std::string severityToString(Severity s);

    struct SharedData // the purpose of this is to make the Note class as small as possible, by storing shared info only once
    {
        Severity m_eSeverity;
        Category m_eCategory;
        int m_nLabelIndex; // position in a given category
        int m_nNoteId; // position in the global vector for non-trace notes; -1 for trace
        bool m_bAllowErase; // needed for the option to remove offending streams;

        std::string m_strDescription;

        SharedData(const std::string& strDescription, bool bAllowErase) : m_eSeverity(TRACE), m_eCategory(CUSTOM), m_nLabelIndex(-1), m_nNoteId(-1), m_bAllowErase(bAllowErase), m_strDescription(strDescription) {}
    private:
        friend struct Notes;
        SharedData(Severity eSeverity, Category eCategory, const std::string& strDescription, bool bAllowErase) : m_eSeverity(eSeverity), m_eCategory(eCategory), m_nLabelIndex(-1), m_nNoteId(-1), m_bAllowErase(bAllowErase), m_strDescription(strDescription) {}

    private:
        friend class boost::serialization::access;
        SharedData() {}
        template<class Archive>
        void serialize(Archive& ar, const unsigned int nVersion)
        {
            if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

            ar & m_strDescription;
            // !!! don't care about other attributes, because after loading, a SharedData object is replaced with one from Notes::getNote(strDescr)
        }
    };

    Note(const Note&, std::streampos pos, const std::string& strDetail = "");
    Note(SharedData& sharedData, std::streampos pos, const std::string& strDetail = ""); // Severity is forced to TRACE
    ~Note();
private:
    friend struct Notes;
    Note(SharedData& sharedData);
    SharedData* m_pSharedData; // !!! can't be const because Notes needs to set indexes when adding notes to vectors, and also because of serialization

public:

    //id // to sort columns
    const char* getDescription() const { return m_pSharedData->m_strDescription.c_str(); }
    std::string getDetail() const { return m_strDetail; }
    std::streampos getPos() const { return m_pos; }
    std::string getPosHex() const; // returns an empty string for an invalid position (i.e. one initialized from -1)
    Severity getSeverity() const { return m_pSharedData->m_eSeverity; }
    Category getCategory() const { return m_pSharedData->m_eCategory; }
    int getLabelIndex() const { return m_pSharedData->m_nLabelIndex; } // index in a given category; used for displaying a label
    int getNoteId() const { return m_pSharedData->m_nNoteId; } // index across categories; used for sorting
    bool allowErase() const { return m_pSharedData->m_bAllowErase; }

    //bool operator<(const Note&) const; // error before warning, ... ; alpha by descr ; alpha by detail
    bool operator==(const Note& other) const;

protected:
    std::streampos m_pos;
    std::string m_strDetail;
    //mutable char m_szPosBfr [20];

private:
    friend class boost::serialization::access;
    Note();
    /*template<class Archive>
    void serialize(Archive& ar, const unsigned int / *nVersion* /)
    {
    }*/

    template<class Archive> void save(Archive& ar, const unsigned int nVersion) const;
    template<class Archive> void load(Archive& ar, const unsigned int nVersion);

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};



/*

The order of DECL_NOTE_INFO() in struct Notes is irrelevant. What matters is what happens in initVec(). The current approach is to group together related notes, regardless of their severity. So audio-related notes come first, then Xing, then ID3V2 ...

The labels are used for convenience. They may change between releases, and are not used internally for anything (e.g. "ignored" notes are stored as full-text in the config file, so they remain ignored as long as their text doesn't change, even if their label changes.)

For a given color / severity the labels should be ordered alphabetically, though (well, as long as there are enough letters; then it depends on what m_nLabelIndex gets mapped to)

*/

// translation of a note
#define noteTr(MSG_ID) QCoreApplication::translate("Notes", Notes::MSG_ID().getDescription())

#define DECL_NOTE_INFO(NAME, SEV, CATEG, MSG, ALLOW_ERASE) static Note& NAME() { static Note::SharedData d (Note::SEV, Note::CATEG, MSG, ALLOW_ERASE); static Note n (d); return n; }

struct Notes
{
    Q_DECLARE_TR_FUNCTIONS(Notes)

public:
    static const Note* getNote(const std::string& strDescr); // returns 0 if not found
    static const Note* getNote(int n); // returns 0 if n is out of range
    static const Note* getMaster(const Note* p); // asserts there is a master

    static const Note* getMissingNote(); // to be used by serialization: if a description is no longer found, the note gets replace with a default, "missing", one
    static void addToDestroyList(Note::SharedData* p) { s_spDestroyList.insert(p); } // while loading from disk, Notes replace the SharedData objects from the disk with instances from getNote(strDescr); after the loading is done, the list (actually a set) must be cleared;
    static void clearDestroyList(); // destoys the pointers added by addToDestroyList(); to be called after loading is complete;

    // a list with all known notes
    static const std::vector<const Note*>& getAllNotes();

    static const std::vector<int>& getDefaultIgnoredNoteIds();

    // audio // a
    DECL_NOTE_INFO(twoAudio, ERR, AUDIO, QT_TR_NOOP("Two MPEG audio streams found, but a file should have exactly one."), false) // a
    DECL_NOTE_INFO(lowQualAudio, WARNING, AUDIO, QT_TR_NOOP("Low quality MPEG audio stream. (What is considered \"low quality\" can be changed in the configuration dialog, under \"Quality thresholds\".)"), false) // b
    DECL_NOTE_INFO(noAudio, ERR, AUDIO, QT_TR_NOOP("No MPEG audio stream found."), false) // c
    DECL_NOTE_INFO(vbrUsedForNonMpg1L3, WARNING, AUDIO, QT_TR_NOOP("VBR with audio streams other than MPEG1 Layer III might work incorrectly."), false) // d
    DECL_NOTE_INFO(incompleteFrameInAudio, ERR, AUDIO, QT_TR_NOOP("Incomplete MPEG frame at the end of an MPEG stream."), false) // e
    DECL_NOTE_INFO(validFrameDiffVer, ERR, AUDIO, QT_TR_NOOP("Valid frame with a different version found after an MPEG stream."), false) // f
    DECL_NOTE_INFO(validFrameDiffLayer, ERR, AUDIO, QT_TR_NOOP("Valid frame with a different layer found after an MPEG stream."), false) // g
    DECL_NOTE_INFO(validFrameDiffMode, ERR, AUDIO, QT_TR_NOOP("Valid frame with a different channel mode found after an MPEG stream."), false) // h
    DECL_NOTE_INFO(validFrameDiffFreq, ERR, AUDIO, QT_TR_NOOP("Valid frame with a different frequency found after an MPEG stream."), false) // i
    DECL_NOTE_INFO(validFrameDiffCrc, ERR, AUDIO, QT_TR_NOOP("Valid frame with a different CRC policy found after an MPEG stream."), false) // j
    DECL_NOTE_INFO(audioTooShort, ERR, AUDIO, QT_TR_NOOP("Invalid MPEG stream. Stream has fewer than 10 frames."), false) // k
    DECL_NOTE_INFO(diffBitrateInFirstFrame, ERR, AUDIO, QT_TR_NOOP("Invalid MPEG stream. First frame has different bitrate than the rest."), false) // l
    DECL_NOTE_INFO(noMp3Gain, WARNING, AUDIO, QT_TR_NOOP("No normalization undo information found. The song is probably not normalized by MP3Gain or a similar program. As a result, it may sound too loud or too quiet when compared to songs from other albums."), false) // n
    DECL_NOTE_INFO(untestedEncoding, SUPPORT, AUDIO, QT_TR_NOOP("Found audio stream in an encoding other than \"MPEG-1 Layer 3\" or \"MPEG-2 Layer 3.\" While MP3 Diags understands such streams, very few tests were run on files containing them (because they are not supposed to be found inside files with the \".mp3\" extension), so there is a bigger chance of something going wrong while processing them."), false) // o

    // xing // b
    DECL_NOTE_INFO(twoLame, ERR, XING, QT_TR_NOOP("Two Lame headers found, but a file should have at most one of them."), true) // a
    DECL_NOTE_INFO(xingAddedByMp3Fixer, WARNING, XING, QT_TR_NOOP("Xing header seems added by Mp3Fixer, which makes the first frame unusable and causes a 16-byte unknown or null stream to be detected next."), false) // b
    DECL_NOTE_INFO(xingFrameCountMismatch, ERR, XING, QT_TR_NOOP("Frame count mismatch between the Xing header and the audio stream."), false) // c
    DECL_NOTE_INFO(twoXing, ERR, XING, QT_TR_NOOP("Two Xing headers found, but a file should have at most one of them."), false) // d
    DECL_NOTE_INFO(xingNotBeforeAudio, ERR, XING, QT_TR_NOOP("The Xing header should be located immediately before the MPEG audio stream."), false) // e
    DECL_NOTE_INFO(incompatXing, ERR, XING, QT_TR_NOOP("The Xing header should be compatible with the MPEG audio stream, meaning that their MPEG version, layer and frequency must be equal."), false) // f
    DECL_NOTE_INFO(missingXing, WARNING, XING, QT_TR_NOOP("The MPEG audio stream uses VBR but a Xing header wasn't found. This will confuse some players, which won't be able to display the song duration or to seek."), false) // g

    // vbri // c
    DECL_NOTE_INFO(twoVbri, ERR, VBRI, QT_TR_NOOP("Two VBRI headers found, but a file should have at most one of them."), true) // a
    DECL_NOTE_INFO(vbriFound, WARNING, VBRI, QT_TR_NOOP("VBRI headers aren't well supported by some players. They should be replaced by Xing headers."), true) // b
    DECL_NOTE_INFO(foundVbriAndXing, WARNING, VBRI, QT_TR_NOOP("VBRI header found alongside Xing header. The VBRI header should probably be removed."), true) // c

    // id3 v2 // d
    DECL_NOTE_INFO(id3v2FrameTooShort, ERR, ID3V2, QT_TR_NOOP("Invalid ID3V2 frame. File too short."), false) // a
    DECL_NOTE_INFO(id3v2InvalidName, ERR, ID3V2, QT_TR_NOOP("Invalid frame name in ID3V2 tag."), true) // b
    DECL_NOTE_INFO(id3v2IncorrectFlg1, WARNING, ID3V2, QT_TR_NOOP("Flags in the first flag group that are supposed to always be 0 are set to 1. They will be ignored."), true) // c
    DECL_NOTE_INFO(id3v2IncorrectFlg2, WARNING, ID3V2, QT_TR_NOOP("Flags in the second flag group that are supposed to always be 0 are set to 1. They will be ignored."), true) // d
    DECL_NOTE_INFO(id3v2TextError, ERR, ID3V2, QT_TR_NOOP("Error decoding the value of a text frame while reading an Id3V2 Stream."), true) // e
    DECL_NOTE_INFO(id3v2HasLatin1NonAscii, WARNING, ID3V2, QT_TR_NOOP("ID3V2 tag has text frames using Latin-1 encoding that contain characters with a code above 127. While this is valid, those frames may have their content set or displayed incorrectly by software that uses the local code page instead of Latin-1. Conversion to Unicode (UTF16) is recommended."), true) // f
    DECL_NOTE_INFO(id3v2EmptyTcon, WARNING, ID3V2, QT_TR_NOOP("Empty genre frame (TCON) found."), true) // g
    DECL_NOTE_INFO(id3v2MultipleFramesWithSameName, WARNING, ID3V2, QT_TR_NOOP("Multiple frame instances found, but only the first one will be used."), true) // h
    DECL_NOTE_INFO(id3v2PaddingTooLarge, WARNING, ID3V2, QT_TR_NOOP("The padding in the ID3V2 tag is too large, wasting space. (Large padding improves the tag editor saving speed, if fast saving is enabled, so you may want to delay compacting the tag until after you're done with the tag editor.)"), false) // i
    DECL_NOTE_INFO(id3v2UnsuppVer, SUPPORT, ID3V2, QT_TR_NOOP("Unsupported ID3V2 version."), true) // j
    DECL_NOTE_INFO(id3v2UnsuppFlag, SUPPORT, ID3V2, QT_TR_NOOP("Unsupported ID3V2 tag. Unsupported flag."), true) // k
    DECL_NOTE_INFO(id3v2UnsuppFlags1, SUPPORT, ID3V2, QT_TR_NOOP("Unsupported value for Flags1 in ID3V2 frame. (This may also indicate that the file contains garbage where it was supposed to be zero.)"), true) // l
    DECL_NOTE_INFO(id3v2UnsuppFlags2, SUPPORT, ID3V2, QT_TR_NOOP("Unsupported value for Flags2 in ID3V2 frame. (This may also indicate that the file contains garbage where it was supposed to be zero.)"), true) // n
    DECL_NOTE_INFO(id3v2DuplicatePopm, SUPPORT, ID3V2, QT_TR_NOOP("Multiple instances of the POPM frame found in ID3V2 tag. The current version discards all the instances except the first when processing this tag."), true) // o
    DECL_NOTE_INFO(id3v2EmptyTag, WARNING, ID3V2, QT_TR_NOOP("ID3V2 tag contains no frames, which is invalid. This note will disappear once you add track information in the tag editor."), true) //p
    DECL_NOTE_INFO(id3v2EmptyTextFrame, WARNING, ID3V2, QT_TR_NOOP("ID3V2 tag contains an empty text frame, which is invalid."), true) //q

    // apic // e
    DECL_NOTE_INFO(id3v2NoApic, WARNING, APIC, QT_TR_NOOP("ID3V2 tag doesn't have an APIC frame (which is used to store images)."), true) // a
    DECL_NOTE_INFO(id3v2CouldntLoadPic, WARNING, APIC, QT_TR_NOOP("ID3V2 tag has an APIC frame (which is used to store images), but the image couldn't be loaded."), true) // b
    DECL_NOTE_INFO(id3v2NotCoverPicture, WARNING, APIC, QT_TR_NOOP("ID3V2 tag has at least one valid APIC frame (which is used to store images), but no frame has a type that is associated with an album cover."), true) // c
    DECL_NOTE_INFO(id3v2ErrorLoadingApic, WARNING, APIC, QT_TR_NOOP("Error loading image in APIC frame."), true) // d
    DECL_NOTE_INFO(id3v2ErrorLoadingApicTooShort, WARNING, APIC, QT_TR_NOOP("Error loading image in APIC frame. The frame is too short anyway to have space for an image."), true) // e
    DECL_NOTE_INFO(id3v2DuplicatePic, ERR, APIC, QT_TR_NOOP("ID3V2 tag has multiple APIC frames with the same picture type."), true) // f
    DECL_NOTE_INFO(id3v2MultipleApic, WARNING, APIC, QT_TR_NOOP("ID3V2 tag has multiple APIC frames. While this is valid, players usually use only one of them to display an image, discarding the others."), true) // g
    //DECL_NOTE_INFO(id3v2LinkNotSupported, SUPPORT, APIC, QT_TR_NOOP("ID3V2 tag has an APIC frame (which is used to store images), but it uses a link to an external file, which is not supported."), false)
    DECL_NOTE_INFO(id3v2UnsupApicTextEnc, SUPPORT, APIC, QT_TR_NOOP("Unsupported text encoding for APIC frame in ID3V2 tag."), true) // h
    DECL_NOTE_INFO(id3v2LinkInApic, SUPPORT, APIC, QT_TR_NOOP("APIC frame uses a link to a file as a MIME Type, which is not supported."), true) // i
    DECL_NOTE_INFO(id3v2PictDescrIgnored, SUPPORT, APIC, QT_TR_NOOP("Picture description is ignored in the current version."), true) // j

    // id3 v2.3.0 // f
    DECL_NOTE_INFO(noId3V230, WARNING, ID3V230, QT_TR_NOOP("No ID3V2.3.0 tag found, although this is the most popular tag for storing song information."), false) // a
    DECL_NOTE_INFO(twoId3V230, ERR, ID3V230, QT_TR_NOOP("Two ID3V2.3.0 tags found, but a file should have at most one of them."), true) // b
    DECL_NOTE_INFO(bothId3V230_V240, WARNING, ID3V230, QT_TR_NOOP("Both ID3V2.3.0 and ID3V2.4.0 tags found, but there should be only one of them."), true) // c
    DECL_NOTE_INFO(id3v230AfterAudio, ERR, ID3V230, QT_TR_NOOP("The ID3V2.3.0 tag should be the first tag in a file."), true) // d
    DECL_NOTE_INFO(id3v230UsesUtf8, WARNING, ID3V230, QT_TR_NOOP("ID3V2.3.0 tag contains a text frame encoded as UTF-8, which is valid in ID3V2.4.0 but not in ID3V2.3.0."), true) // e
    DECL_NOTE_INFO(id3v230UnsuppText, SUPPORT, ID3V230, QT_TR_NOOP("Unsupported value of text frame while reading an Id3V2 Stream."), true) // f
    DECL_NOTE_INFO(id3v230CantReadFrame, ERR, ID3V230, QT_TR_NOOP("Invalid ID3V2.3.0 frame. Incorrect frame size or file too short."), true) // g

    // id3 v2.4.0 // g
    DECL_NOTE_INFO(twoId3V240, ERR, ID3V240, QT_TR_NOOP("Two ID3V2.4.0 tags found, but a file should have at most one of them."), true) // a
    DECL_NOTE_INFO(id3v240CantReadFrame, ERR, ID3V240, QT_TR_NOOP("Invalid ID3V2.4.0 frame. Incorrect frame size or file too short."), true) // b
    DECL_NOTE_INFO(id3v240IncorrectSynch, WARNING, ID3V240, QT_TR_NOOP("Invalid ID3V2.4.0 frame. Frame size is supposed to be stored as a synchsafe integer, which uses only 7 bits in a byte, but the size uses all 8 bits, as in ID3V2.3.0. This will confuse some applications"), true) // c
    DECL_NOTE_INFO(id3v240DeprTyerAndTdrc, WARNING, ID3V240, QT_TR_NOOP("Deprecated TYER frame found in 2.4.0 tag alongside a TDRC frame."), true) // d
    DECL_NOTE_INFO(id3v240DeprTyer, WARNING, ID3V240, QT_TR_NOOP("Deprecated TYER frame found in 2.4.0 tag. It's supposed to be replaced by a TDRC frame."), true) // e
    DECL_NOTE_INFO(id3v240DeprTdatAndTdrc, WARNING, ID3V240, QT_TR_NOOP("Deprecated TDAT frame found in 2.4.0 tag alongside a TDRC frame."), true) // f
    DECL_NOTE_INFO(id3v240DeprTdat, WARNING, ID3V240, QT_TR_NOOP("Deprecated TDAT frame found in 2.4.0 tag. It's supposed to be replaced by a TDRC frame."), true) // g
    DECL_NOTE_INFO(id3v240IncorrectDli, WARNING, ID3V240, QT_TR_NOOP("Invalid ID3V2.4.0 frame. Mismatched Data length indicator. Frame value is probably incorrect"), true) // h
    DECL_NOTE_INFO(id3v240IncorrectFrameSynch, WARNING, ID3V240, QT_TR_NOOP("Invalid ID3V2.4.0 frame. Incorrect unsynchronization bit."), true) // i
    DECL_NOTE_INFO(id3v240UnsuppText, SUPPORT, ID3V240, QT_TR_NOOP("Unsupported value of text frame while reading an Id3V2.4.0 stream. It may be using an unsupported text encoding."), true) // j

    // id3 v1 // h
    DECL_NOTE_INFO(onlyId3V1, WARNING, ID3V1, QT_TR_NOOP("The only supported tag found that is capable of storing song information is ID3V1, which has pretty limited capabilities."), false) // a
    DECL_NOTE_INFO(id3v1BeforeAudio, ERR, ID3V1, QT_TR_NOOP("The ID3V1 tag should be located after the MPEG audio stream."), true) // b
    DECL_NOTE_INFO(id3v1TooShort, ERR, ID3V1, QT_TR_NOOP("Invalid ID3V1 tag. File too short."), false) // c
    DECL_NOTE_INFO(twoId3V1, ERR, ID3V1, QT_TR_NOOP("Two ID3V1 tags found, but a file should have at most one of them."), true) // d
    //DECL_NOTE_INFO(zeroInId3V1, WARNING, ID3V1, QT_TR_NOOP("ID3V1 tag contains characters with the code 0, although this is not allowed by the standard (yet used by some tools)."), false)
    DECL_NOTE_INFO(mixedPaddingInId3V1, WARNING, ID3V1, QT_TR_NOOP("ID3V1 tag contains fields padded with spaces alongside fields padded with zeroes. The standard only allows zeroes, but some tools use spaces. Even so, zero-padding and space-padding shouldn't be mixed."), true) // e
    DECL_NOTE_INFO(mixedFieldPaddingInId3V1, WARNING, ID3V1, QT_TR_NOOP("ID3V1 tag contains fields that are padded with spaces mixed with zeroes. The standard only allows zeroes, but some tools use spaces. Even so, one character should be used for padding for the whole tag."), true) // f
    DECL_NOTE_INFO(id3v1InvalidName, ERR, ID3V1, QT_TR_NOOP("Invalid ID3V1 tag. Invalid characters in Name field."), true) // g
    DECL_NOTE_INFO(id3v1InvalidArtist, ERR, ID3V1, QT_TR_NOOP("Invalid ID3V1 tag. Invalid characters in Artist field."), true) // h
    DECL_NOTE_INFO(id3v1InvalidAlbum, ERR, ID3V1, QT_TR_NOOP("Invalid ID3V1 tag. Invalid characters in Album field."), true) // i
    DECL_NOTE_INFO(id3v1InvalidYear, ERR, ID3V1, QT_TR_NOOP("Invalid ID3V1 tag. Invalid characters in Year field."), true) // j
    DECL_NOTE_INFO(id3v1InvalidComment, ERR, ID3V1, QT_TR_NOOP("Invalid ID3V1 tag. Invalid characters in Comment field."), true) // k

    // broken // i
    DECL_NOTE_INFO(brokenAtTheEnd, ERR, BROKEN, QT_TR_NOOP("Broken stream found."), true) // a
    DECL_NOTE_INFO(brokenInTheMiddle, ERR, BROKEN, QT_TR_NOOP("Broken stream found. Since other streams follow, it is possible that players and tools will have problems using the file. Removing the stream is recommended."), true) // b

    // trunc // j
    DECL_NOTE_INFO(truncAudioWithWholeFile, ERR, TRUNCATED, QT_TR_NOOP("Truncated MPEG stream found. The cause for this seems to be that the file was truncated."), false) // a
    DECL_NOTE_INFO(truncAudio, ERR, TRUNCATED, QT_TR_NOOP("Truncated MPEG stream found. Since other streams follow, it is possible that players and tools will have problems using the file. Removing the stream or padding it with 0 to reach its declared size is strongly recommended."), false) // b

    // unknown // k
    DECL_NOTE_INFO(unknTooShort, WARNING, UNKNOWN, QT_TR_NOOP("Not enough remaining bytes to create an UnknownDataStream."), false) // a
    DECL_NOTE_INFO(unknownAtTheEnd, ERR, UNKNOWN, QT_TR_NOOP("Unknown stream found."), true) // b
    DECL_NOTE_INFO(unknownInTheMiddle, ERR, UNKNOWN, QT_TR_NOOP("Unknown stream found. Since other streams follow, it is possible that players and tools will have problems using the file. Removing the stream is recommended."), true) // c
    DECL_NOTE_INFO(foundNull, WARNING, UNKNOWN, QT_TR_NOOP("File contains null streams."), true) // d

    // lyrics // l
    DECL_NOTE_INFO(lyrTooShort, ERR, LYRICS, QT_TR_NOOP("Invalid Lyrics stream tag. File too short."), false) // a
    DECL_NOTE_INFO(twoLyr, SUPPORT, LYRICS, QT_TR_NOOP("Two Lyrics tags found, but only one is supported."), true) // b // ttt2 see if this is error
    //DECL_NOTE_INFO(lyricsNotSupported, SUPPORT, LYRICS, QT_TR_NOOP("Lyrics tags cannot be processed in the current version. Some players don't understand them."), false)
    DECL_NOTE_INFO(invalidLyr, ERR, LYRICS, QT_TR_NOOP("Invalid Lyrics stream tag. Unexpected characters found."), false) // c
    DECL_NOTE_INFO(duplicateFields, SUPPORT, LYRICS, QT_TR_NOOP("Multiple fields with the same name were found in a Lyrics tag, but only one is supported."), true) // d
    //DECL_NOTE_INFO(imgInLyrics, SUPPORT, LYRICS, QT_TR_NOOP("Currently images referenced from Lyrics tags are ignored."), true) // (e)
    DECL_NOTE_INFO(infInLyrics, SUPPORT, LYRICS, QT_TR_NOOP("Currently INF fields in Lyrics tags are not fully supported."), true) // e

    // ape // n
    DECL_NOTE_INFO(apeItemTooShort, ERR, APE, QT_TR_NOOP("Invalid Ape Item. File too short."), false) // a
    DECL_NOTE_INFO(apeItemTooBig, ERR, APE, QT_TR_NOOP("Ape Item seems too big. Although the size may be any 32-bit integer, 256 bytes should be enough in practice. If this note is determined to be incorrect, it will be removed in the future."), false) // b
    DECL_NOTE_INFO(apeMissingTerminator, ERR, APE, QT_TR_NOOP("Invalid Ape Item. Terminator not found for item name."), false) // c
    DECL_NOTE_INFO(apeFoundFooter, ERR, APE, QT_TR_NOOP("Invalid Ape tag. Header expected but footer found."), false) // d
    DECL_NOTE_INFO(apeTooShort, ERR, APE, QT_TR_NOOP("Not an Ape tag. File too short."), false) // e
    DECL_NOTE_INFO(apeFoundHeader, ERR, APE, QT_TR_NOOP("Invalid Ape tag. Footer expected but header found."), false) // f
    DECL_NOTE_INFO(apeHdrFtMismatch, ERR, APE, QT_TR_NOOP("Invalid Ape tag. Mismatch between header and footer."), false) // g
    DECL_NOTE_INFO(twoApe, SUPPORT, APE, QT_TR_NOOP("Two Ape tags found, but only one is supported."), true) // h // ttt2 see if this is error
    DECL_NOTE_INFO(apeFlagsNotSupported, SUPPORT, APE, QT_TR_NOOP("Ape item flags not supported."), false) // i
    DECL_NOTE_INFO(apeUnsupported, SUPPORT, APE, QT_TR_NOOP("Unsupported Ape tag. Currently a missing header or footer are not supported."), false) // j

    // misc // o
    DECL_NOTE_INFO(fileWasChanged, WARNING, MISC, QT_TR_NOOP("The file seems to have been changed in the (short) time that passed between parsing it and the initial search for pictures. If you think that's not the case, report a bug."), false) // a
    DECL_NOTE_INFO(noInfoTag, WARNING, MISC, QT_TR_NOOP("No supported tag found that is capable of storing song information."), false) // b
    DECL_NOTE_INFO(tooManyTraceNotes, WARNING, MISC, QT_TR_NOOP("Too many TRACE notes added. The rest will be discarded."), false) // c
    DECL_NOTE_INFO(tooManyNotes, WARNING, MISC, QT_TR_NOOP("Too many notes added. The rest will be discarded."), false) // d
    DECL_NOTE_INFO(tooManyStreams, WARNING, MISC, QT_TR_NOOP("Too many streams found. Aborting processing."), false) // e
    DECL_NOTE_INFO(unsupportedFound, WARNING, MISC, QT_TR_NOOP("Unsupported stream found. It may be supported in the future if there's a real need for it."), false) // f
    DECL_NOTE_INFO(rescanningNeeded, WARNING, MISC, QT_TR_NOOP("The file was saved using the \"fast\" option. While this improves the saving speed, it may leave the notes in an inconsistent state, so you should rescan the file."), false) // g

    struct CompNoteByName // needed for searching
    {
        bool operator()(const Note* p1, const Note* p2) const;
    };
    typedef std::set<Note*, CompNoteByName> NoteSet;

private:

    //static std::vector<const Note*> s_vpErrNotes; // pointers are to static variables defined in DECL_NOTE_INFO
    //static std::vector<const Note*> s_vpWarnNotes; // pointers are to static variables defined in DECL_NOTE_INFO
    //static std::vector<const Note*> s_vpSuppNotes; // pointers are to static variables defined in DECL_NOTE_INFO
    static std::vector<std::vector<const Note*> > s_vpNotesByCateg;

    static std::vector<const Note*> s_vpAllNotes; // pointers are to static variables defined in DECL_NOTE_INFO

    static NoteSet s_spAllNotes; // pointers are to static variables defined in DECL_NOTE_INFO

    static void addNote(Note* p);
    static void initVec();

    static std::set<Note::SharedData*> s_spDestroyList; // !!! sorted by pointers
};


//============================================================================================================


template<class Archive>
inline void Note::save(Archive& ar, const unsigned int nVersion) const
{
    if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

    ar << m_pSharedData;
    ar << m_pos;
    ar << m_strDetail;
}

template<class Archive>
inline void Note::load(Archive& ar, const unsigned int nVersion)
{
    if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

    SharedData* pSharedData;
    ar >> pSharedData;
    ar >> m_pos;
    ar >> m_strDetail;

    Notes::addToDestroyList(pSharedData);
    const Note* pNote (Notes::getNote(pSharedData->m_strDescription));

    m_pSharedData = 0 == pNote ? Notes::getMissingNote()->m_pSharedData : pNote->m_pSharedData; // 2012.02.03 - the test seems pointless: Notes::getNote(const std::string& strDescr) uses getMissingNote() if it cannot find the description
}


//============================================================================================================
//============================================================================================================
//============================================================================================================



// notes associated with a Mp3Handler (but is used stand-alone too, to pass as a param to some functions and then discard the results)
class NoteColl
{
    std::vector<Note*> m_vpNotes; // owns the pointers
    int m_nCount;
    int m_nTraceCount;
    int m_nMaxTrace;
public:
    NoteColl(int nMaxTrace) : m_nCount(0), m_nTraceCount(0), m_nMaxTrace(nMaxTrace) {}
    NoteColl() {} // serialization-only constructor
    ~NoteColl();

    void add(Note*);
    const std::vector<Note*>& getList() const { return m_vpNotes; }
    void resetCounter() { m_nCount = 0; } // normally only 200 notes are added and anything following is discarded; this allows to reset the counter
    void sort(); // sorts by CmpNotePtrById, which is needed for filtering
    void removeTraceNotes();

    bool hasFastSaveWarn() const;
    void addFastSaveWarn();
    void removeNotes(const std::streampos& posFrom, const std::streampos& posTo); // removes notes with addresses in the given range; posFrom is included, but posTo isn't

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & m_vpNotes;

        // these don't really matter, but they don't take a lot of space and can be useful at debugging
        ar & m_nCount;
        ar & m_nTraceCount;
        ar & m_nMaxTrace;
    }
};



/*struct CmpNotePtrBySevAndLabel
{
    bool operator()(const Note* p1, const Note* p2) const
    {
        if (p1->getSeverity() < p2->getSeverity()) { return true; }
        if (p1->getSeverity() > p2->getSeverity()) { return false; }
        if (p1->getNoteId() < p2->getNoteId()) { return true; }
        return false;
    }

    static bool equals(const Note* p1, const Note* p2)
    {
        if (p1->getSeverity() != p2->getSeverity() || p1->getNoteId() != p2->getNoteId()) { return false; }
        return true;
    }
};*/

struct CmpNotePtrById // needed for unique notes, where equality doesn't include the position
{
    bool operator()(const Note* p1, const Note* p2) const
    {
        if (p1->getNoteId() < p2->getNoteId()) { return true; }
        return false;
    }

    static bool equals(const Note* p1, const Note* p2)
    {
        if (p1->getNoteId() != p2->getNoteId()) { return false; }
        return true;
    }
};



/*
struct CmpNotePtrBySevLabelAndPos
{
    bool operator()(const Note* p1, const Note* p2) const
    {
        if (p1->getSeverity() < p2->getSeverity()) { return true; }
        if (p1->getSeverity() > p2->getSeverity()) { return false; }
        if (p1->getNoteId() < p2->getNoteId()) { return true; }
        if (p1->getNoteId() > p2->getNoteId()) { return false; }
        if (p1->getPos() < p2->getPos()) { return true; }
        return false;
    }
};*/


/*struct CmpNotePtrByPosSevAndLabel
{
    bool operator()(const Note* p1, const Note* p2) const
    {
        if (p1->getPos() < p2->getPos()) { return true; }
        if (p1->getPos() > p2->getPos()) { return false; }
        if (p1->getSeverity() < p2->getSeverity()) { return true; }
        if (p1->getSeverity() > p2->getSeverity()) { return false; }
        if (p1->getNoteId() < p2->getNoteId()) { return true; }
        return false;
    }
};*/

/*
struct CmpNotePtrByIdAndPos
{
    bool operator()(const Note* p1, const Note* p2) const
    {
        if (p1->getNoteId() < p2->getNoteId()) { return true; }
        if (p1->getNoteId() > p2->getNoteId()) { return false; }
        if (p1->getPos() < p2->getPos()) { return true; }
        return false;
    }
};*/


struct CmpNotePtrByPosAndId
{
    bool operator()(const Note* p1, const Note* p2) const
    {
        if (p1->getPos() < p2->getPos()) { return true; }
        if (p1->getPos() > p2->getPos()) { return false; }
        if (p1->getNoteId() < p2->getNoteId()) { return true; }
        return false;
    }
};


// allows strings to be shared (as pointers)
struct StringWrp
{
    std::string s;
    StringWrp(const std::string& s) : s(s) {}
private:
    friend class boost::serialization::access;
    StringWrp() {}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int nVersion)
    {
        if (nVersion > 0) { throw std::runtime_error("invalid version of serialized file"); }

        ar & s;
    }
};

//BOOST_CLASS_TRACKING(StringWrp, boost::serialization::track_always);
//BOOST_CLASS_TRACKING(StringWrp, boost::serialization::track_never);


#endif // #ifndef NotesH


