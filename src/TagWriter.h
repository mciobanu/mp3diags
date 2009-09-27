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


#ifndef TagWriterH
#define TagWriterH

#include  <vector>
#include  <string>
#include  <set>

#include  "DataStream.h"
#include  "CommonTypes.h"

class QToolButton;

class TagWriter;
class ImageInfoPanelWdgImpl;

// wrapper for the 3-state "Assign" button in the tag editor
struct AssgnBtnWrp
{
    enum State { ALL_ASSGN, SOME_ASSGN, NONE_ASSGN };
    void setState(State eState); // sets m_eState and changes the button icon accordingly
    State getState() const { return m_eState; }
    AssgnBtnWrp(QToolButton* pButton) : m_pButton(pButton) { setState(NONE_ASSGN); }
private:
    State m_eState;
    QToolButton* m_pButton;
};


struct TagWrtImageInfo
{
    ImageInfo m_imageInfo;
    std::set<std::string> m_sstrFiles;
    bool operator==(const ImageInfo& imgInf) const { return m_imageInfo == imgInf; }

    TagWrtImageInfo(const ImageInfo& imageInfo, const std::string& strFile) : m_imageInfo(imageInfo)
    {
        if (!strFile.empty())
        {
            m_sstrFiles.insert(strFile);
        }
    }
};

class ImageColl
{
    std::vector<TagWrtImageInfo> m_vTagWrtImageInfo;
    std::vector<ImageInfoPanelWdgImpl*> m_vpWidgets;
    int m_nCurrent;
public:
    ImageColl();
    int addImage(const ImageInfo& img, const std::string& strFile = ""); // returns the index of the image; if it already exists it's not added again; if it's invalid returns -1
    void addWidget(ImageInfoPanelWdgImpl*); // first addImage gets called by TagWriter and after it's done it tells MainFormDlgImpl to create widgets, which calls this;
    void clear(); // clears both m_vTagWrtImageInfo and m_vpWidgets
    void select(int n); // -1 deselects all
    const TagWrtImageInfo& operator[](int n) const { return m_vTagWrtImageInfo.at(n); }
    int find(const ImageInfo& img) const;
    int size() const { return (int)m_vTagWrtImageInfo.size(); }
    const TagWrtImageInfo& back() const { return m_vTagWrtImageInfo.at(size() - 1); }
};



namespace SongInfoParser
{
    class TrackTextParser;
}


// There's one of these for each pair of <file, pattern> in the current album (or rather <file, TrackTextParsers>, because patterns are not seen directly). They merely "use" the corresponding TrackTextParser to set up the data in the constructor, so the hierarchy of Reader objects is kept only once, in TagWriter. A TrackTextReader extracts track information from a string, which is either a file name or a single line from a multi-line string pasted from the clipboard.
class TrackTextReader : public TagReader
{
    std::string m_strTitle;
    std::string m_strArtist;
    std::string m_strTrackNumber;
    TagTimestamp m_timeStamp;
    std::string m_strGenre;
    std::string m_strAlbumName;
    double m_dRating;
    std::string m_strComposer;

    bool m_bHasTitle;
    bool m_bHasArtist;
    bool m_bHasTrackNumber;
    bool m_bHasTimeStamp;
    bool m_bHasGenre;
    bool m_bHasAlbumName;
    bool m_bHasRating;
    bool m_bHasComposer;

    const char* m_szType;
public:
    /*override*/ std::string getTitle(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_bHasTitle; } return m_strTitle; }

    /*override*/ std::string getArtist(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_bHasArtist; } return m_strArtist; }

    /*override*/ std::string getTrackNumber(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_bHasTrackNumber; } return m_strTrackNumber; }

    /*override*/ TagTimestamp getTime(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_bHasTimeStamp; } return m_timeStamp; }

    /*override*/ std::string getGenre(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_bHasGenre; } return m_strGenre; }

    /*override*/ ImageInfo getImage(bool* /*pbFrameExists*/ = 0) const { throw NotSupportedOp(); }

    /*override*/ std::string getAlbumName(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_bHasAlbumName; } return m_strAlbumName; }

    /*override*/ double getRating(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_bHasRating; } return m_dRating; }

    /*override*/ std::string getComposer(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_bHasComposer; } return m_strComposer; }

    /*override*/ SuportLevel getSupport(Feature) const;

    DECL_RD_NAME("Pattern");
    const char* getType() const { return m_szType; }

    TrackTextReader(SongInfoParser::TrackTextParser* pTrackTextParser, const std::string& s);
    /*override*/ ~TrackTextReader();
};


// information downloaded from sites and passed in AlbumInfo
class WebReader : public TagReader
{
    std::string m_strTitle;
    std::string m_strArtist;
    std::string m_strTrackNumber;
    TagTimestamp m_timeStamp;
    std::string m_strGenre;
    ImageInfo m_imageInfo;
    std::string m_strAlbumName;
    double m_dRating;
    std::string m_strComposer;
    AlbumInfo::VarArtists m_eVarArtists;

    //bool m_bSuppTitle;
    //bool m_bSuppArtist;
    //bool m_bSuppTrackNumber;
    //bool m_bSuppTimeStamp;
    bool m_bSuppGenre;
    //bool m_bSuppAlbumName;
    //bool m_bSuppRating;
    bool m_bSuppComposer;
    bool m_bSuppVarArtists;

    std::string m_strType;
    int convVarArtists() const; // converts m_eVarArtists to an int is either 0 or contains all VA-enabled values, based on configuration
public:
    /*override*/ std::string getTitle(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = !m_strTitle.empty(); } return m_strTitle; }

    /*override*/ std::string getArtist(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = !m_strArtist.empty(); } return m_strArtist; }

    /*override*/ std::string getTrackNumber(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = !m_strTrackNumber.empty(); } return m_strTrackNumber; }

    /*override*/ TagTimestamp getTime(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = 0 != *m_timeStamp.asString(); } return m_timeStamp; }

    /*override*/ std::string getGenre(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = !m_strGenre.empty(); } return m_strGenre; }

    /*override*/ ImageInfo getImage(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = !m_imageInfo.isNull(); } return m_imageInfo; }

    /*override*/ std::string getAlbumName(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = !m_strAlbumName.empty(); } return m_strAlbumName; }

    /*override*/ double getRating(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = m_dRating >= 0; } return m_dRating; }

    /*override*/ std::string getComposer(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = !m_strComposer.empty(); } return m_strComposer; }

    /*override*/ int getVariousArtists(bool* pbFrameExists = 0) const { if (0 != pbFrameExists) { *pbFrameExists = AlbumInfo::VA_NOT_SUPP != m_eVarArtists; } return convVarArtists(); }

    /*override*/ SuportLevel getSupport(Feature) const;

    DECL_RD_NAME("Web");
    const std::string& getType() const { return m_strType; }

    WebReader(const AlbumInfo& albumInfo, int nTrackNo); // nTrackNo is 0-based, just an index in albumInfo.m_vTracks
    ~WebReader();
};


class Mp3Handler;
class CommonData;


// tag data for a file/Mp3Handler
// fields are in the order given by TagReader::Feature, and not FEATURE_ON_POS, so UI components must use FEATURE_ON_POS themselves to determine the "row" they are passing
class Mp3HandlerTagData
{
    Mp3HandlerTagData(const Mp3HandlerTagData&);
    Mp3HandlerTagData& operator=(const Mp3HandlerTagData&);
public:
    Mp3HandlerTagData(TagWriter* pTagWriter, const Mp3Handler* pMp3Handler, int nCrtPos, int nOrigPos, const std::string& strPastedVal);
    ~Mp3HandlerTagData();

    enum Status { EMPTY, ID3V2_VAL, NON_ID3V2_VAL, ASSIGNED };

    std::string getData(int nField) const { return m_vValueInfo[nField].m_strValue; }
    int getImage() const; // 0-based; -1 if there-s no image;
    double getRating() const;
    Status getStatus(int nField) const { return m_vValueInfo[nField].m_eStatus; }

    void setData(int nField, const std::string& s); // may throw InvalidValue
    void setStatus(int nField, Status);

    std::string getData(int nField, int k) const; // returns the data corresponding to the k-th element in m_pTagWriter->m_vTagReaderInfo; returns "\n" if it doesn't have a corresponding stream (e.g. 2nd ID3V1 tag) or if the given feature is not supported (e.g. picture in ID3V1)
    // nField is the "internal" row for which data is retrieved, so TagReader::FEATURE_ON_POS[] has to be used by the UI caller

    const Mp3Handler* getMp3Handler() const { return m_pMp3Handler; }

    void refreshReaders(); // updates m_vpTagReader to reflect m_pTagWriter->m_vTagReaderInfo, then updates unassigned values
    void print(std::ostream&) const;

    int getOrigPos() const { return m_nOrigPos; }
    const TagReader* getMatchingReader(int i) const; // returns 0 if i is out of range

    void adjustVarArtists(bool b); // if VARIOUS_ARTISTS is not ASSIGNED, sets m_strValue and m_eStatus

    struct InvalidValue {};
private:
    void setUp(); // to be called initially and each time the priority of tag readers changes
    TagWriter* m_pTagWriter;
    const Mp3Handler* m_pMp3Handler;
    int m_nCrtPos;
    int m_nOrigPos; // m_nOrigSong gives the position in m_pCommonData->m_vpViewHandlers; the current position may change after sorting by track number

    std::vector<TagReader*> m_vpTagReaders; // needed for the "current album" grid; tag readers used to get the data for track, title, ... ; doesn't own the pointers; there are 2 related differences between it and m_vpMatchingTagReaders: 1) m_vpTagReaders doesn't contain null elements; 2) some of the null entries in m_vpMatchingTagReaders are replaced with other entries in m_vpTagReaders
    /*
        there is a file that has 2 ID3V1 tags in the current album, so 2 ID3V1 columns appear, along with an ID3V2 and an Ape;
        the user chooses this order "ID3V1 2", "ID3V2", "Ape", "ID3V1 1" (which internally are seen as "ID3V1 1", "ID3V2 0", "Ape 0", "ID3V1 0", because counting starts at 0)
        the current file only has an ID3V1 and an ID3V2
        m_vpMatchingTagReaders  will end up with: <null>,    "ID3V2 0", <null>, "ID3V1 0" (matching m_pTagWriter->m_vTagReaderInfo's size of 4)
        m_vpTagReaders          will end up with: "ID3V1 0", "ID3V2 0",         "ID3V1 0" (size is 3)

        see also the big comment in refreshReaders()
    */
    std::vector<TagReader*> m_vpMatchingTagReaders; // needed for the "current file" grid; has the same size as m_pTagWriter->m_vTagReaderInfo; some elements are null, if there is no corresponding reader; doesn't own the pointers;
    std::vector<TrackTextReader> m_vTrackTextReaders; // pointers to TrackTextReader need to be owned, so it's easiest to put the objects in a vector, while also putting the pointers in m_vpTagReaders
    std::vector<WebReader> m_vWebReaders; // pointers to TrackTextReader need to be owned, so it's easiest to put the objects in a vector, while also putting the pointers in m_vpTagReaders

    struct ValueInfo
    {
        std::string m_strValue;
        Status m_eStatus;
        ValueInfo() : m_eStatus(EMPTY) {}
    };

    std::vector<ValueInfo> m_vValueInfo; // PictureIndex is stored as a string

    std::string m_strPastedVal; // cleared on reload(), assigned when pasting multi-line content from the clipboard
    mutable std::vector<std::string> m_vstrImgCache;
};



struct TagReaderInfo
{
    std::string m_strName;
    int m_nPos; // usually this is 0; has other values if there are more than 1 instance of a TagReader type (e.g. TrackTextReader, Id3V230Stream, ...), to tell them apart
    bool m_bAlone; // if it's the only TagReaderInfo with its name

    enum { ONE_OF_MANY, ALONE };

    TagReaderInfo(const std::string& strName, int nPos, bool bAlone) : m_strName(strName), m_nPos(nPos), m_bAlone(bAlone) {}
    bool operator==(const TagReaderInfo& other) const { return m_nPos == other.m_nPos && m_strName == other.m_strName; }
};




class TagWriter : public QObject
{
    Q_OBJECT

    std::vector<TagReaderInfo> m_vSortedKnownTagReaders; // used to remember the sort order between albums and/or sessions; after m_vTagReaderInfo is populated, it should be sorted so that it matches the order in m_vSortedKnownTagReaders, if possible; items not found are to be added to the end
    void sortTagReaders(); // sorts m_vTagReaderInfo so that it matches m_vSortedKnownTagReaders

    std::vector<SongInfoParser::TrackTextParser*> m_vpTrackTextParsers; // one TrackTextParser for each pattern
    std::set<int> m_snActivePatterns;
    std::vector<AlbumInfo> m_vAlbumInfo; // one AlbumInfo for every album data downloaded from structured web sites

    // "original value" of a selected field; not necessarily related to the fields in a file's tags, but merely holds whatever happened to be in a given field when it gets selected; the "original value" from Mp3HandlerTagData's point of view is not stored anywhere, but it is recovered as needed, because toggleAssigned() calls "reloadAll("", DONT_CLEAR)"
    struct OrigValue
    {
        int m_nSong, m_nField; // nField is an index in TagReader::Feature, not affected by TagReader::FEATURE_ON_POS
        std::string m_strVal;
        Mp3HandlerTagData::Status m_eStatus;

        OrigValue(int nSong, int nField, const std::string& strVal, Mp3HandlerTagData::Status eStatus) : m_nSong(nSong), m_nField(nField), m_strVal(strVal), m_eStatus(eStatus) {}
        bool operator<(const OrigValue& other) const
        {
            if (m_nSong < other.m_nSong) { return true; }
            if (other.m_nSong < m_nSong) { return false; }
            return m_nField < other.m_nField;
        }
    };
    friend std::ostream& operator<<(std::ostream& out, const TagWriter::OrigValue& val);
    std::set<OrigValue> m_sSelOrigVal; // original values for selected fields (so it can also be used to determine which fields are selected)
    bool m_bSomeSel;
    bool m_bNonStandardTrackNo;

    std::vector<int> m_vnMovedTo; // the position on screen corresponding to m_pCommonData->m_vpViewHandlers elements after they are sorted by track number

    void sortSongs(); // sorts by track number; shows a warning if issues are detected (should be exactly one track number, from 1 to the track count)
    bool addImgFromFile(const QString& qs, bool bConsiderAssigned); // see also addImage()

    std::vector<std::string> m_vstrPastedValues;

    CommonData* m_pCommonData;
    QWidget* m_pParentWnd; // for QMessageBox
    int m_nCurrentFile;

    ImageColl m_imageColl;

    bool m_bShowedNonSeqWarn;
    std::set<int> m_snUnassignedImages;

    const bool& m_bIsFastSaving;
    bool m_bShouldShowPatternsNote;

    int m_nFileToErase;

    bool m_bVariousArtists;
    bool m_bAutoVarArtists; // true at first, until the "toggle" button is clicked
    void adjustVarArtists();

    bool m_bDelayedAdjVarArtists;
public:
    TagWriter(CommonData* pCommonData, QWidget* pParentWnd, const bool& bIsFastSaving);
    ~TagWriter();

    enum ClearData { DONT_CLEAR_DATA, CLEAR_DATA };
    enum ClearAssigned { DONT_CLEAR_ASSGN, CLEAR_ASSGN };

    // called in 3 cases: 1) when going to a new album; 2) when changing the order of readers; 3) when adding/changing/removing TrackTextReaders; all require the reader list to be updated (there are 3 kinds of readers: 1) those that the current album uses, which are stored in Mp3Handler; 2) TrackTextReader instances, which are built manually from m_vpTrackTextParsers; and 3) those with data read from the web);
    // nPos tells which should be the current file; if nPos is <0 it is ignored and the current file remains the same; a value <0 should only be used when changing reader priorities;
    // if eReloadOption is CLEAR, everyting is reloaded; if it's something else, the call is supposed to be for the same album, after changing tag priorities, so ASSIGNED values shouldn't change; if it's UPDATE_SEL, the selection is changed to the first cell for the current song; if it's DONT_UPDATE_SEL the selection isn't changed
// if strCrt is empty and eReloadOption is DONT_CLEAR, the current position is kept; if strCrt is empty and eReloadOption is CLEAR, the current position is first song;
    //void reloadAll(std::string strCrt, ReloadOption eReloadOption/*, bool bKeepUnassgnImg*/); // bKeepUnassgnImg matters only if eReloadOption is CLEAR

    void reloadAll(std::string strCrt, bool bClearData, bool bClearAssgn);

    void setCrt(const std::string& strCrt); // makes current a file with a given name; if name is not found (may also be empty), makes current the first file; doesn't cause the grid selection to change;
    void setCrt(int nCrt); // asserts nCrt is valid; doesn't cause the grid selection to change;

    std::vector<Mp3HandlerTagData*> m_vpMp3HandlerTagData; // one for each file in the current album
    std::vector<TagReaderInfo> m_vTagReaderInfo; // has only readers that correspond to the current album, so it's a subset of m_vSortedKnownTagReaders; (sortTagReaders() adds to m_vSortedKnownTagReaders whatever new Readers are found in m_vTagReaderInfo)

    SongInfoParser::TrackTextParser* getTrackTextParser(int n) { return m_vpTrackTextParsers.at(n); }
    int getTrackTextParsersCnt() const { return (int)m_vpTrackTextParsers.size(); }
    const AlbumInfo& getAlbumInfo(int n) const { return m_vAlbumInfo.at(n); }
    int getAlbumInfoCnt() const { return (int)m_vAlbumInfo.size(); }

    int getIndex(const ImageInfo&) const; // asserts that the picture exists

    const Mp3Handler* getCurrentHndl() const; // returns 0 if there's no current handler
    std::string getCurrentName() const; // returns "" if there's no current handler
    const Mp3HandlerTagData* getCrtMp3HandlerTagData() const; // returns 0 if there's no current handler

    void moveReader(int nOldVisualIndex, int nNewVisualIndex);

    void addKnownInf(const std::vector<TagReaderInfo>& v); // should be called on startup by MainFormDlgImpl, to get config data; asserts that m_vSortedKnownTagReaders is empty;
    const std::vector<TagReaderInfo>& getSortedKnownTagReaders() const { return m_vSortedKnownTagReaders; }

    // the int tells which position a given pattern occupied before; (it's -1 for new patterns);
    // doesn't throw, but invalid patterns are discarded; it returns false if at least one pattern was discarded;
    bool updatePatterns(const std::vector<std::pair<std::string, int> >&);
    std::vector<std::string> getPatterns() const;
    void setActivePatterns(const std::set<int>&);
    std::set<int> getActivePatterns() const { return m_snActivePatterns; }

    // model-based nField; UI components should pass the UI index through TagReader::FEATURE_ON_POS[] before calling this
    Mp3HandlerTagData::Status getStatus(int nSong, int nField) const { return m_vpMp3HandlerTagData[nSong]->getStatus(nField); }
    std::string getData(int nSong, int nField) const { return m_vpMp3HandlerTagData[nSong]->getData(nField); }
    void setData(int nSong, int nField, const std::string& s) { m_vpMp3HandlerTagData[nSong]->setData(nField, s); } // may throw InvalidValue
    void setStatus(int nSong, int nField, Mp3HandlerTagData::Status eStatus) { m_vpMp3HandlerTagData[nSong]->setStatus(nField, eStatus); }
    void hasUnsaved(int nSong, bool& bAssigned, bool& bNonId3V2); // sets bAssigned and bNonId3V2 if at least one field has the corresponding status;
    void hasUnsaved(bool& bAssigned, bool& bNonId3V2); // sets bAssigned and bNonId3V2 if at least one field in at least a song has the corresponding status;

    void getAlbumInfo(std::string& strArtist, std::string& strAlbum); // artist and album for the current song; empty if they don't exist

    void addAlbumInfo(const AlbumInfo&);

    // should be called when the selection changes; updates m_sSelOrigVal and returns the new state of m_pAssignedB;
    AssgnBtnWrp::State updateAssigned(const std::vector<std::pair<int, int> >& vFields);

    // should be called when the user clicks on the assign button; changes status of selected cells and returns the new state of m_pAssignedB
    AssgnBtnWrp::State toggleAssigned(AssgnBtnWrp::State eCrtState);

    void copyFirst();
    void paste();
    void sort();

    void eraseFields(const std::vector<std::pair<int, int> >& vFields);

    bool isFastSaving() const { return m_bIsFastSaving; }

    bool shouldShowPatternsNote() const { return m_bShouldShowPatternsNote; }

    enum ConsiderAssigned { CONSIDER_UNASSIGNED, CONSIDER_ASSIGNED };
    int addImage(const ImageInfo& img, bool bConsiderAssigned); // returns the index of the image; if it already exists it's not added again; if it's invalid returns -1 // "unassigned" images cause warnings when going to another album
    void addImgWidget(ImageInfoPanelWdgImpl*);
    const ImageColl& getImageColl() const { return m_imageColl; }
    void selectImg(int n);

    void clearShowedNonSeqWarn() { m_bShowedNonSeqWarn = false; }
    int getUnassignedImagesCount() const { return int(m_snUnassignedImages.size()); }

    void toggleVarArtists();
    void delayedAdjVarArtists();

private slots:
    void onAssignImage(int);
    void onEraseFile(int);
    void onEraseFileDelayed();
    void onDelayedTrackSeqWarn();
    void onDelayedAdjVarArtists();

signals:
    void albumChanged(/*bool bContentOnly*/); // the selection may be kept iff bContentOnly is true
    void fileChanged();
    void imagesChanged();
    void requestSave();
    void varArtistsUpdated(bool bVarArtists);
};


#endif // #ifndef TagWriterH

