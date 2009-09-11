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


#ifndef SongInfoParserH
#define SongInfoParserH

#include  <vector>
#include  <string>

namespace SongInfoParser
{

/*

Getting song details from file names:

The main class is TrackTextParser, but it is mereley an interface to other classes:
    Reader,
        BoundReader
            StaticReader
            RatingReader
            TrackNoReader
            YearReader
        UnboundReader
        SequenceReader
        OptionalReader


Reader is the base class in the hierarchy, representing the idea of "something" that can match (or not) a part of a string. I declares 2 main functions:
1) init(const char* pcLeft, const char* pcRight, bool bLeftFirst) - tries to match the string beggining at pcLeft and ending at pcRight and sets the internal state accordingly (m_pcLeft, m_pcRight, m_eLeftBound, m_eRightBound, m_eState). The matching doesn't always have to cover the whole string. A substring is sufficient in many cases. bLeftFirst indicates where the search for a substring should start.

2) set(vector<string>& v) may be called only if init() succeeded; v's size must be "TagReader::LIST_END+1", one element for each supported field and an additional one for ignored strings; each Reader has at most 1 corresponding field; set() changes v's element that corresponds to that field (the order is given by TagReader::Feature); note that some Reader's don't have a field, so they don't change anything.


Readers that have a corresponding field are either UnboundReader or something derived from BoundReader.

BoundReader is for fields that have clearly identifiable limits (track number, year, rating or some static string), while UnboundReader is for fields without such limits (artist, album, track name).

Usage: TagWriter has a list of "patterns". For each pattern it creates a FileTagReader, for which init() gets called, filling in field values, which are then displayed. A FileTagReader has a list of trees of Reader instances, and while it processes its init() it calls init() and set() for those Reader's.

To build the tree, a pattern is parsed. First of all it is split at the file separator limit; for each segment a top-level SequenceReader gets created, and the segment is further parsed to determine what will be part of the SequenceReader. When assign() is called for a particular file name, it too is split by file separators and each of the top-level SequenceReader tries a match for its corresponding name segment when init() is called. The match may fail or succeed. If it succeeds, set() is then called, to assign values to fields. A failure in one top-level SequenceReader doesn't affect the others.

Patterns are strings containing file name separators, static text and these placeholder:
%n track number
%a artist
%t title
%b album
%y year
%g genre
%r rating (a lowercase letter)
%c composer
%i ignored (there's no predefined field that would take this value)

For example "%a/%b/%n.-.%t" is a pattern made up of these elements:
    %a  (artist placeholder)
    /   (file name separator)
    %b  (album)
    /   (file name separator)
    %n  (track number)
    .-. (static text)
    %t  (title)

During matching, corresponding file name separators and static texts are identified, and what's left must match the placeholders and give the name of the corresponding fields.

By matching "%a/%b/%n.-.%t" to a file named ".../dir1/artist1/album1.(1995)/02.-.track1.mp3" we get:
    title = track1
    track number = 02
    album = album1.(1995)
    artist = artist1

Matching "%a/%b/%n.-.%t" to a file named ".../dir1/artist1/album1.(1995)/02.track1.mp3" we get:
    album = album1.(1995)
    artist = artist1

The last segment caused an error, because "02.track1.mp3" doesn't contain ".-." . Now this pattern would give the same results as in the previous case: "%a/%b/%n.%t"


To be able to deal with the different naming conventions that are likely to be used for different albums, 2 options are available:
1) Have more instances of TrackTextParser, to accomodate those conventions.
2) Include optional elements in some of those TrackTextParser instances. Optional elements are delimited by '[' and ']'. For example "%a/%b/%n.[-.]%t" matches correctly ".../dir1/artist1/album1 (1995)/02.-.track1.mp3" as well as ".../dir1/artist1/album1 (1995)/02.track1.mp3". That is, if you consider "album1 (1995)" as a proper album name. Probably makes more sense to use "%a/%b (%y)/%n.[-.]%t" instead, which gives:
    title = track1
    track number = 02
    album = album1
    artist = artist1
    year = 1995

If some albums end with the year enclosed in parantheses and some don't have a year in the directory name, this may work for all the cases: "%a/%b[.(%y)]/%n.[-.]%t"


An OptionalReader has a single field, which is a SequenceReader. It's not worth the trouble to have the option to include some other fields, even though in many cases the SequenceReader will only contain an element.

Objects created for "%a/%b[.(%y)]/%n.[-.]%t" :
    TrackTextParser
        SequenceReader "%a"
            UnboundReader<artist>
        SequenceReader "%b[.(%y)]"
            UnboundReader<album>
            OptionalReader
                SequenceReader ".(%y)"
                    StaticReader ".("
                    YearReader
                    StaticReader ")"
        SequenceReader (%n.[-.]%t)
            TrackNoReader
            StaticReader "."
            OptionalReader
                SequenceReader "-."
                    StaticReader "-."
            UnboundReader<title>


SequenceReader is a lot more complex than the others, because it has all the different Readers that is has to determine recursively if they match or not. So this is how it basically works: First, it sorts the readers in a way that allows them to be processed sequentially (bounded readers should be matched before unbounded ones, but it's not so simple, because a reader may be bound at none, one or both sides). Then it calls init() on all of them, marking some as failed, if needed. A failure in a non-optional reader makes the whole SequenceReader fail. Any gap in the string to be matched makes the whole SequenceReader fail (but remaining substrings at the margins are OK).


While the other readers are either bound or unbound, a SequenceReader may be bound at one side and unbound at the other ("%n %t" corresponds to a SequenceReader that's bound at the left and unbound at the right.)


Optional readers may be nested. However, more complex patterns may be harder to understand and to get right. Also, there are limitations, as some backtracking would make sense, but it's not currently implemented. //ttt2 implement
Consider the case of "[[.].]%t" : for ".tst1.mp3" the title will be ".tst1", while for "..tst1.mp3" it will be "tst1" . This happens because the inner optional reader first takes up the '.' on the right, so the outer optional doesn't have a "." for itself, which makes the outer reader to fail. What should happen is this: the outer optional reader should discard the inner reader and try again without it, in which case it would succeed. However, this looks time consuming and doesn't provide such great benefits: "[.[.]]%t" achieves what "[[.].]%t" tries to do without backtracking and making the second "." optional is what it takes to do the trick. Sure, this doesn't work in all cases, but it seems to be good enough.



Some patterns may look OK but are invalid. Aside from mismatched '[' and ']', which should come in pairs, there's this issue with a succesfully parsed pattern: a SequenceReader isn't allowed to have 2 consecutive readers r1 and r2 if r1 is unbound to the right and r2 is unbound to the left, because there's no way to tell where one ends and the other begins. Optional readers are considered both as missing and present. So these are invalid:
    "%a%b"
    "%a[%b]"
    "%a[-]%b"
    "[%a][%b]"
while these are valid
    "%a-%b"
    "%a[-%b]"
    "[%a-]%b"


Since "%", "[" and "]" are used as special characters, they can't appear on their own in patterns. To include them in a pattern, they should be preceded by another "%": "%%", "%[" and "%]"


*/

struct SequenceReader;
struct Reader;


// gets track info from a full file name or from a generic string, normally a line from a multi-line track listing
// if m_bSplitAtPathSep is true, the string passed to assign is broken into independent pieces, matching the independent pieces from m_strPattern, which may fail individually without triggering the failure of the whole; if it's false, the whole strPattern must match the pattern; the decision to make m_bSplitAtPathSep true or false is based on the pattern containing an unescaped path separator (keeping in mind that to include such a separator in a "table line parser" it has to be escaped)
class TrackTextParser
{
    std::vector<SequenceReader*> m_vpSeqReaders;
    std::vector<Reader*> m_vpAllReaders; // all instances of Reader-derived classes, to be easily deleted
    void construct(const std::string& strPattern);
    std::string m_strPattern;
    bool m_bSplitAtPathSep;
public:
    TrackTextParser(const std::string& strPattern); // throws InvalidPattern for invalid pattern
    ~TrackTextParser();

    // parses strFileName, identifying track info details and setting v accordingly; v's size must be TagReader::LIST_END+1, with elements in the order given by TagReader::Feature, while the last one is reserved for "ignored"; v is not erased; if there is no value for a field, the corresponding element doesn't change;
    void assign(const std::string& s, std::vector<std::string>& v);

    std::string getPattern() const { return m_strPattern; }

    bool isFileNameBased() const { return m_bSplitAtPathSep; }

    struct InvalidPattern
    {
        int m_nPos;
        InvalidPattern(int nPos) : m_nPos(nPos) {}
    };
};

/*
// gets track info from a string, normally a line from a multi-line track listing
class TrackTextParser
{
    SequenceReader* m_pSeqReader;
    std::vector<Reader*> m_vpAllReaders; // all instances of Reader-derived classes, to be easily deleted
    void construct(const std::string& strPattern);
    std::string m_strPattern;
public:
    TrackTextParser(const std::string& strPattern); // throws InvalidPattern for invalid pattern
    ~TrackTextParser();

    // parses strTrackInfo, identifying track info details and setting v accordingly; v's size must be TagReader::LIST_END+1, with elements in the order given by TagReader::Feature, while the last one is reserved for "ignored"; v is not erased; if there is no value for a field, the corresponding element doesn't change;
    void assign(const std::string& strTrackInfo, std::vector<std::string>& v);

    std::string getPattern() const { return m_strPattern; }

    struct InvalidPattern
    {
        int m_nPos;
        InvalidPattern(int nPos) : m_nPos(nPos) {}
    };
};
*/

std::string testPattern(const std::string& strPattern); // returns an empty string if the pattern is valid and an error message (including the column) if it's invalid



} // namespace SongInfoParser



#endif // #ifndef SongInfoParserH


