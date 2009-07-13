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


#ifndef CommonDataH
#define CommonDataH

#include  <deque>
#include  <set>

//#include  <QColor>
#include  <QFont>

#include  "SerSupport.h"

#include  "Notes.h"
#include  "Mp3Manip.h"
#include  "Helpers.h"
#include  "FileEnum.h"

//class QFont;


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

class QToolButton;
class FilesModel;
class NotesModel;
class StreamsModel;
class UniqueNotesModel;

extern int CELL_WIDTH; // ttt1 perhaps replace with functions
extern int CELL_HEIGHT;

extern const int CUSTOM_TRANSF_CNT; // search for this to find all places that need changes to add another custom transform list



// Holds together a collection of Notes (All) and a subset of it (Flt). Flt is used when filtering, when All holds all the notes that exist in all the handlers (CommonData::m_vpAllHandlers), while Flt holds the notes from the handlers that are selected by the current filter (CommonData::m_vpFltHandlers).
// Uses both a vector and a set to provide fast access, althought this doesn't seem like the best idea. //ttt2 perhaps switch from <m_spAll, m_vpAll, m_bAllDirty> to a treap
class UniqueNotes
{
    // all pointers are from Notes::, so none are owned;
    std::set<const Note*, CmpNotePtrById> m_spAll; // owns the pointers
    std::set<const Note*, CmpNotePtrById> m_spFlt; // subset of m_spAll, so it doesn't own these pointers

    mutable std::vector<const Note*> m_vpAll; // only has elements of m_spAll, so it doesn't own these pointers
    mutable std::vector<const Note*> m_vpFlt; // only has elements of m_spAll, so it doesn't own these pointers

    mutable bool m_bAllDirty;
    mutable bool m_bFltDirty;

    void updateVAll() const;
    void updateVFlt() const;

public:
    UniqueNotes();
    //UniqueNotes(const UniqueNotes&);
    ~UniqueNotes();

    void clear();

    void clearFlt()
    {
        m_spFlt.clear();
        m_bFltDirty = true;
    }

    bool addNote(const Note* pNote); // if the note doesn't exist in m_spAll, it adds the corresponding note from Notes; returns true if the param really was added;
    //void add(const std::vector<const Note*>& vpNotes);
    //void add(const std::vector<const Note*>& vpNotes);

    // returns true if anything got added
    template <class T>
    bool addColl(const T& coll)
    {
        bool bRes (false);
        for (typename T::const_iterator it = coll.begin(), end = coll.end(); it != end; ++it)
        {
            if (0 == m_spAll.count(*it))
            {
                const Note* p (Notes::getMaster(*it));
                CB_ASSERT (0 != p);
                m_spAll.insert(p);
                m_bAllDirty = true;
                bRes = true;
            }
        }
        return bRes;
    }


    void _selectAll() { m_spFlt = m_spAll; m_bFltDirty = true; }

    // throws NoteNotFound if a note is not in m_spAll;
    template <class T>
    void setFlt(const T& coll)
    {
        m_spFlt.clear();
        m_bFltDirty = true;

        for (typename T::const_iterator it = coll.begin(), end = coll.end(); it != end; ++it)
        {
            int nPos (getPos(*it));
            CB_CHECK1 (-1 != nPos, NoteNotFound());
            m_spFlt.insert(get(nPos));
        }
    }

    struct NoteNotFound {};

    int getFltCount() const { return cSize(m_spFlt); }
    const Note* getFlt(int n) const;

    int getCount() const { return cSize(m_spAll); }
    const Note* get(int n) const;

    int getPos(const Note*) const; // position in the "all" notes; -1 if the note wasn't found;
    int getFltPos(const Note*) const; // position in the "flt" notes; -1 if the note wasn't found;

    const std::vector<const Note*>& getAllVec() const { updateVAll(); return m_vpAll; }
    const std::vector<const Note*>& getFltVec() const { updateVFlt(); return m_vpFlt; }
};




// Owners for notes:
//   DescrOwnerNote::m_spAll owns DescrOwnerNote*
//   Mp3Handler owns Note*, through NoteColl
//
// Filter::m_vpSelNotes references CommonData::m_uniqueNotes.m_spAll
// DescrOwnerNote::m_spSel, DescrOwnerNote::m_spSel and DescrOwnerNote::m_spSel all referece the notes in DescrOwnerNote::m_spAll


class Filter : public QObject
{
    Q_OBJECT

    bool m_bNoteFilter;
    bool m_bDirFilter;

    bool m_bSavedNoteFilter;
    bool m_bSavedDirFilter;

    std::vector<std::string> m_vDirs;
    std::vector<const Note*> m_vpNotes; // doesn't own the pointers, because it is just a subset of CommonData::m_uniqueNotes.getAllVec(), which in turn uses pointers from Notes:: // !!! it's OK for this to be a vector (rather than a set), because the way it is used most of the time, namely to determine if the intersection of 2 sets (represented as sorted vectors) is empty or not
public:
    Filter() : m_bNoteFilter(false), m_bDirFilter(false), m_bSavedNoteFilter(false), m_bSavedDirFilter(false) {}

    const std::vector<std::string>& getDirs() const { return m_vDirs; }
    const std::vector<const Note*>& getNotes() const { return m_vpNotes; }
    void setDirs(const std::vector<std::string>&);
    void setNotes(const std::vector<const Note*>&);
    void disableDir(); // !!! needed because we need m_vDirs next time we press the filter button, so we don't want to delete it with a "setDirs(vector<string>())", but just ignore it
    void disableNote();

    void disableAll(); // saves m_bNoteFilter to m_bSavedNoteFilter and m_bDirFilter to m_bSavedDirFilter, then disables the filters
    void restoreAll(); // loads m_bNoteFilter from m_bSavedNoteFilter and m_bDirFilter from m_bSavedDirFilter, then enables the filters, if they are true

    bool isNoteEnabled() const { return m_bNoteFilter; }
    bool isDirEnabled() const { return m_bDirFilter; }

signals:
    void filterChanged();

private:
    friend class boost::serialization::access;

    /*template<class Archive>
    void serialize(Archive& ar, const unsigned int / *nVersion* /)
    {
        ar & m_bNoteFilter;
        ar & m_bDirFilter;

        ar & m_bSavedNoteFilter;
        ar & m_bSavedDirFilter;

        ar & m_vDirs;
        ar & m_vpNotes;
    }*/

    template<class Archive> void save(Archive& ar, const unsigned int /*nVersion*/) const
    {
        ar << m_bNoteFilter;
        ar << m_bDirFilter;

        ar << m_bSavedNoteFilter;
        ar << m_bSavedDirFilter;

        ar << m_vDirs;
        //qDebug("saved dirs sz %d", cSize(m_vDirs));
        //ar << m_vpNotes; //ttt1 weird behaviour: this compiles and doesn't trigger runtime errros, and neither does the loading, but when loading a vector<Note*> it always ends up empty; it's probably some incorrect use of the ser library (share / global / const pointers), but the library is broken too, because it should have failed to compile or at least crashed when running instead of just failing to load anything (the files are different, so something is saved)
        //ar << (const std::vector<const Note*>&)m_vpNotes; qDebug("saved notes sz %d", cSize(m_vpNotes));

        std::vector<std::string> v;
        for (int i = 0; i < cSize(m_vpNotes); ++i)
        {
            v.push_back(m_vpNotes[i]->getDescription());
        }
        ar << (const std::vector<std::string>&)v;//*/
    }

    template<class Archive> void load(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar >> m_bNoteFilter;
        ar >> m_bDirFilter;

        ar >> m_bSavedNoteFilter;
        ar >> m_bSavedDirFilter;

        ar >> m_vDirs;
        //qDebug("loaded dirs sz %d", cSize(m_vDirs));

        /*std::vector<Note*> v;
        ar >> v;
        qDebug("loaded note sz %d", cSize(v));
        for (int i = 0; i < cSize(v); ++i)
        {
            const Note* p (Notes::getMaster(v[i]));
            CB_ASSERT (0 != p);
            m_vpNotes.push_back(p);
        }
        pearl::clearPtrContainer(v);// */

        std::vector<std::string> v;
        ar >> v;
        //qDebug("loaded note sz %d", cSize(v));
        for (int i = 0; i < cSize(v); ++i)
        {
            const Note* p (Notes::getNote(v[i]));
            if (0 != p) // !!! when a new version loads an old one's data, some notes might be gone
            {
                m_vpNotes.push_back(p);
            }
        }// */
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER();
};


class ImageInfoPanelWdgImpl;
class SessionSettings;
class Transformation;
class QTableView;


// doesn't own the QTableView pointers;
// doesn't own the model pointers
class CommonData : public QObject
{
    Q_OBJECT

public:
    CommonData(
            SessionSettings& settings,
            QTableView* pFilesG,
            QTableView* pNotesG,
            QTableView* pStreamsG,

            QTableView* pUniqueNotesG,

            QToolButton* pNoteFilterB,
            QToolButton* pDirFilterB,
            QToolButton* pModeAllB,
            QToolButton* pModeAlbumB,
            QToolButton* pModeSongB);

    ~CommonData();

    std::string save(const std::string& strFile) const; // returns an error message (or empty string if there's no error)
    std::string load(const std::string& strFile); // returns an error message (or empty string if there's no error)

    FilesModel* m_pFilesModel;
    NotesModel* m_pNotesModel;
    StreamsModel* m_pStreamsModel;
    UniqueNotesModel* m_pUniqueNotesModel;


    QTableView* m_pFilesG;
    QTableView* m_pNotesG;
    QTableView* m_pStreamsG;

    QTableView* m_pUniqueNotesG;

    const std::deque<const Mp3Handler*>& getViewHandlers() const { return m_vpViewHandlers; }
    const std::deque<const Mp3Handler*>& getSelHandlers(); // results are sorted

    const UniqueNotes& getUniqueNotes() const { return m_uniqueNotes; }

    // "albums" for the tag editor; independent of the main window;
    void setCrtAlbum(const std::string& strName) const;
    std::deque<const Mp3Handler*> getCrtAlbum() const;
    bool nextAlbum() const;
    bool prevAlbum() const;

    // finds the position of a note in the global vector with notes sorted by severity and description;
    // the position can then be used to find the corresponding label;
    // returns -1 if the note is not found; (needed because trace and info notes aren't shown in the grid, so they are not found)
    int findPos(const Note* pNote) const;

    bool m_bChangeGuard; // used with a NonblockingGuard to avoid recursive calls when updating the UI

    int getFilesGCrtRow() const; // returns -1 if no current element exists (e.g. because the table is empty)
    int getFilesGCrtCol() const; // returns -1 if no current element exists (e.g. because the table is empty)

    const std::vector<const Note*>& getCrtNotes() const { return m_vpCrtNotes; } // notes for the "current" file
    const std::vector<DataStream*>& getCrtStreams() const;

    const std::vector<Transformation*>& getTransf() const { return m_vpTransf; }
    const QualThresholds& getQualThresholds() const { return m_qualThresholds; }
    void setQualThresholds(const QualThresholds&);

    const std::vector<std::vector<int> >& getCustomTransf() const { return m_vvCustomTransf; }
    void setCustomTransf(const std::vector<std::vector<int> >&);
    void setCustomTransf(int nTransf, const std::vector<int>&);

    const std::vector<int>& getIgnoredNotes() const { return m_vnIgnoredNotes; }
    void setIgnoredNotes(const std::vector<int>&);

    int getTransfPos(const char* szTransfName) const; // the index in m_vpTransf for a transformation with a given name; throws if the name doesn't exist;

    std::set<std::string> getAllDirs() const; // needed by the dir filter

    enum KeepWhenUpdate { NOTHING = 0x00, CURRENT = 0x01, SEL = 0x02 };
    void mergeHandlerChanges(const std::vector<const Mp3Handler*>& vpAdd, const std::vector<const Mp3Handler*>& vpDel, int nKeepWhenUpdate); // elements from vpAdd that are not "included" are discarded; when a new version of a handler is in vpAdd, the old version may be in vpDel, but this is optional; takes ownership of elements from vpAdd; deletes the pointers from vpDel;


    enum ViewMode { ALL, FOLDER, FILE };

    // although the current elem can be identified (so it shouldn't be passed) and most of the time pMp3Handler will be just that, sometimes it will deliberately be 0, so a param is actually needed;
    void setViewMode(ViewMode eViewMode, const Mp3Handler* pMp3Handler = 0);
    ViewMode getViewMode() const { return m_eViewMode; }

    const Mp3Handler* getCrtMp3Handler() const; // returns 0 if the list is empty

    // called after config change or filter change or mergeHandlerChanges(), mainly to update unique notes (which is reflected in columns in the file grid and in lines in the unique notes list); also makes sure that something is displayed if there are any files in m_vpFltHandlers (e.g. if a transform was applied that removed all the files in an album, the next album gets loaded);
    // this is needed after transforms/normalization/tag editing, but there's no need for an explicit call, because all these call mergeHandlerChanges() (directly or through MainFormDlgImpl::scan())
    void updateWidgets(const std::string& strCrtName = "", const std::vector<std::string>& vstrSel = std::vector<std::string>());

    void next();
    void previous();

    void resizeFilesGCols(); // resizes the first column to use all the available space; doesn't shrink it to less than 400px, though

    void setSongInCrtAlbum();

    const std::vector<std::string>& getIncludeDirs() const { return m_vstrIncludeDirs; }
    const std::vector<std::string>& getExcludeDirs() const { return m_vstrExcludeDirs; }
    void setDirectories(const std::vector<std::string>& vstrIncludeDirs, const std::vector<std::string>& vstrExcludeDirs); // keeps existing handlers as long as they are still "included"
    DirTreeEnumerator m_dirTreeEnum;
    void resetFileEnum() { m_dirTreeEnum.reset(); }

    const Mp3Handler* getHandler(const std::string& strName) const; // looks in m_vpAllHandlers; returns 0 if there's no such handler
    const std::string& getCrtName() const; // returns the file name of the current handler; returns "" if the list is empty

    void setFontInfo(const std::string& strGenName, int nGenSize, int nLabelFontSizeDecr, const std::string& strFixedName, int nFixedSize);

    const QFont& getGeneralFont() const { CB_ASSERT (!m_strGenFontName.empty()); return m_generalFont; }
    const QFont& getLabelFont() const { CB_ASSERT (!m_strFixedFontName.empty()); return m_labelFont; }
    const QFont& getFixedFont() const { CB_ASSERT (!m_strGenFontName.empty()); return m_fixedFont; }
    int getLabelFontSizeDecr() const { return m_nLabelFontSizeDecr; }

    QFont getNewGeneralFont() const;
    QFont getNewFixedFont() const;

    void setCrtAtStartup() { updateWidgets(m_strLoadCrtName); } // to be called from the main thread at startup

    //QString getNoteLabel(int nPosInFlt); // gets the label of a note based on its position in m_uniqueNotes.m_vpFlt

    // color is normally the category color, but for support notes it's a "support" color; if the note isn't found in vpNoteSet, dGradStart and dGradEnd are set to -1, but normally they get a segment obtained by dividing [0, 1] in equal parts;
    void getNoteColor(const Note& note, const std::vector<const Note*>& vpNoteSet, QColor& color, double& dGradStart, double& dGradEnd) const;

public:
    enum Case { LOWER, UPPER, TITLE, PHRASE };
    Case m_eCaseForArtists;
    Case m_eCaseForOthers;

    bool m_bWarnOnNonSeqTracks, m_bWarnPastingToNonSeqTracks;
    bool m_bShowDebug, m_bShowSessions;
    bool m_bScanAtStartup;
    std::string m_strNormalizeCmd;
    bool m_bKeepNormWndOpen;
    QByteArray m_locale;
    QTextCodec* m_pCodec;

    Filter m_filter;

    enum Save { SAVE, DISCARD, ASK };
    Save m_eAssignSave;
    Save m_eNonId3v2Save;

    //AssgnBtnWrp m_assgnBtnWrp;

    bool m_bUseAllNotes;

    bool m_bTraceEnabled;
    void trace(const std::string& s);
    void clearLog();
    const std::deque<std::string>& getLog() const { return m_vLogs; }

    bool m_bAutoSizeIcons;
    int m_nMainWndIconSize;

    SessionSettings& m_settings;

    bool m_bKeepOneValidImg;

    std::string m_strTransfLog; // log file with transformations;
    bool m_bLogTransf;

    bool m_bSaveDownloadedData;

    enum { COLOR_ALB_NORM, COLOR_ALB_NONID3V2, COLOR_ALB_ASSIGNED, COLOR_FILE_NORM, COLOR_FILE_TAG_MISSING, COLOR_FILE_NA, COLOR_FILE_NO_DATA, COLOR_COL_CNT };
    std::vector<QColor> m_vTagEdtColors;
    std::vector<QColor> m_vNoteCategColors;

private:
    std::deque<const Mp3Handler*> m_vpAllHandlers; // owns the pointers; sorted by CmpMp3HandlerPtrByName;

    std::deque<const Mp3Handler*> m_vpFltHandlers; // filtered m_vpHandlers (so it doesn't own the pointers);
    std::deque<const Mp3Handler*> m_vpViewHandlers; // subset of m_vpFltHandlers (so it doesn't own the pointers), containing the handlers that are currently visible; depending on the view mode, it may be equal to m_vpFltHandlers, contain a single directory or a single file
    std::deque<const Mp3Handler*> m_vpSelHandlers; // subset of m_vpViewHandlers (so it doesn't own the pointers), containing the handlers that are currently selected; it is not regularly kept up-to-date, instead being updated only when needed, with updateSelList()

    // !!! m_vpAllHandlers, m_vpFltHandlers and m_vpViewHandlers must be kept sorted at all times, so binary search can be run on them

    std::vector<std::string> getSelNames(); // calls updateSelList() and returns the names ot the selected files;

    void updateSelList();

    UniqueNotes m_uniqueNotes;  // a copy of every "relevant" note; basically notes with a severity other than TRACE; also, notes that should be ignored don't get added here; "Sel" contains the notes that are shown after filtering (by note or by dir) is applied (so by default "Sel" is equal to "All"); has pointers from Notes:: not dynamically created ones;
    std::vector<int> m_vnIgnoredNotes; // indexes into Notes::getAllNotes(); not sorted;

    std::vector<const Note*> m_vpCrtNotes; // notes for the "current" file

    std::vector<Transformation*> m_vpTransf; // owns the pointers

    QualThresholds m_qualThresholds;

    // copies to spUniqueNotes all the unique notes from commonData.m_vpFltHandlers, with comparison done by CmpNotePtrById;
    // ignored notes (given by m_vnIgnoredNotes) are not included;
    // not actual pointers from vpHandlers are stored, but the corresponding ones from given by Notes::getMaster(); so they must not be freed;
    void getUniqueNotes(const std::deque<const Mp3Handler*>& vpHandlers, std::set<const Note*, CmpNotePtrById>& spUniqueNotes);

    void printFilesCrt() const; // for debugging; prints what's current in m_pFilesG, using several methods

    std::vector<std::vector<int> > m_vvCustomTransf;


    enum { APPROX_MATCH, EXACT_MATCH };
    // if bExactMatch is false, it finds the nearest position in m_vpViewHandlers (so even if a file was deleted, it still finds something close);
    // returns -1 only if not found (either m_vpViewHandlers is empty or bExactMatch is true and the file is missing);
    int getPosInView(const std::string& strName/*, bool bUsePrevIfNotFound = true*/, bool bExactMatch = false) const;

    void updateCurrentStreams(); // resizes the rows to fit the data and notifies that the model has changed and there are new streams
    void updateCurrentNotes(); // updates m_pCommonData->m_vpCrtNotes, to hold the notes corresponding to the current file and resizes the rows to fit the data

    // returns the position of the "current" elem in m_vpViewHandlers (so it can be selected in the grid); if m_vpViewHandlers is empty, it returns -1; if pMp3Handler is 0, it returns 0 (unless m_vpViewHandlers is empty);
    int setViewModeHlp(ViewMode eViewMode, const Mp3Handler* pMp3Handler);
    int nextHlp(); // returns the position of the "current" elem in m_vpViewHandlers (see setViewMode() for details)
    int previousHlp();

    void updateUniqueNotes(); // updates m_uniqueNotes to reflect the current m_vpAllHandlers and m_vpFltHandlers;

    mutable int m_nSongInCrtAlbum; // something in the "current album" used by the tag editor; might be first, last or in the middle;

    std::string m_strGenFontName;
    int m_nGenFontSize;
    int m_nLabelFontSizeDecr;
    std::string m_strFixedFontName;
    int m_nFixedFontSize;
    QFont m_generalFont;
    QFont m_fixedFont;
    QFont m_labelFont;

    std::string m_strLoadCrtName; // needed by setCrtAtStartup(), because calling updateWidgets() from a secondary thread has issues; so instead, the name of the current file is saved here for later, to be set from the main thread, through setCrtAtStartup()

    //bool m_bDirty; // seemed like a good idea, but since we also save filters and what's current, pretty much all the time the data will need to be saved

private:
    ViewMode m_eViewMode;
    int getPosInFlt(const Mp3Handler* pMp3Handler) const; // finds the position in m_vpFltHandlers; returns -1 if not found; 0 is a valid argument, in which case -1 is returned; needed by next() and previous(), to help with navigation when going into "folder" mode

    QToolButton* m_pNoteFilterB;
    QToolButton* m_pDirFilterB;

    QToolButton* m_pModeAllB;
    QToolButton* m_pModeAlbumB;
    QToolButton* m_pModeSongB;

    std::deque<std::string> m_vLogs;

    std::vector<std::string> m_vstrIncludeDirs, m_vstrExcludeDirs;

public slots:
    void onCrtFileChanged();
    void onFilterChanged(); // updates m_vpFltHandlers and m_vpViewHandlers; also updates the state of the filter buttons (deselecting them if the user chose empty filters)

private:
    friend class boost::serialization::access;

    template<class Archive> void save(Archive& ar, const unsigned int /*nVersion*/) const;
    template<class Archive> void load(Archive& ar, const unsigned int /*nVersion*/);
    BOOST_SERIALIZATION_SPLIT_MEMBER();
};


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


//=====================================================================================================================


// For the vertical header of a QTableView whose labels are the current row number, it determines the width necessary to accomodate any of those labels.
// Currently it uses a hard-coded value to add to the width. //ttt2 fix
// It makes 2 assumptions:
//   - the TableView uses the same FontMetrics as the ones returned by QApplication::fontMetrics() (this is intended to be called from a TableModel's headerData(), for which finding the table is not easy; and anyway, a model can be connected to several tables)
//   - digits in the font have the same size, or at least there is no digit with a size larger than that of '9'; (in many fonts all the digits do have the same size, so it should be OK most of the time)
// Only the width is calculated; the height is returned as "1". This allows the content of a cell to determine the width of a row. Returning the height actually needed to draw the label would cause the rows to be to large, because significant spacing is added to the result. This is the opposite of what happens to the width, where a big number of pixels must be added by getNumVertHdrSize() just to have everything displayed. // ttt3 is this a Qt bug? (adds spacing where it's not needed and doesn't add it where it should)
//
// If nRowCount<=0 it behaves as if nRowCount==1
//
// Returns QVariant() for horizontal headers.
//
// The real reason this is needed: Qt can easily resize the header to accomodate all the header labels, and that's enough for fixed-height rows, whose height is set with verticalHeader()->setDefaultSectionSize(). However, if resizeRowsToContents() gets called (and it seems that it must be called to get variable-height working, and some flag to enable this doesn't seem to exist) the height of each row's header becomes too big. Using getNumVertHdrSize() we force the height to be 1, thus allowing the data cells to tell the height (the final height is the maximum between data cells and the header cell for each row). //ttt2 At some point it seemed that rows would get larger even without calling getNumVertHdrSize(). This should be looked at again.
QVariant getNumVertHdrSize(int nRowCount, Qt::Orientation eOrientation); // ttt2 add optional param QTableView to take the metrics from


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

// returns a label for a note; first 26 notes get labels "a" .. " z", next 26 get "A" .. "Z", then "aa" .. "az", "aA" .. "aZ", "ba", ...
QString getNoteLabel(const Note* pNote);

class QColor;
const QColor& ERROR_PEN_COLOR();
const QColor& SUPPORT_PEN_COLOR();
//QColor getNoteColor(const Note& note); // color based on severity



void defaultResize(QDialog& dlg); // resizes a dialog with inexisting/invalid size settings, so it covers an area slightly smaller than MainWnd; however, if the dialog is alrady bigger than that, it doesn't get shrinked

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


void printFontInfo(const char* szLabel, const QFont& font);

#endif // #ifndef CommonDataH

