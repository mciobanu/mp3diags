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


#ifndef TransformationH
#define TransformationH

#include  <string>

#include  <QApplication> // for translation

/*

Processing a file by MainFormDlgImpl:

There is a list of Transformation instances. Their apply() functions get called, creating a new file from an existing one each time they return "true", until all of them return "false".

There is a "source root folder", where all the original files come from. Not all the files inside that folder and its subdirectories are going to be processed; some filtering may be used. It may be "/", but care should be taken to not try to process files from m_strTempRootDir, m_strProcRootDir or m_strCompRootDir.

Various Transformation instances get called. If they decide that they will modify the file, they call TransfConfig::getTempName() (which uses m_strTempRootDir) to get the name of the file where they should put their result. The next Transformation to get called will receive an Mp3Handler for this, just created file, along with the name of the original file.

After a Transformation::apply() returns "true", the list with Transformation gets called from the beginning, until every element in the list return "false".

After that, the last file that got created is moved to the "destination folder", determined by m_strProcRootDir. It may be the same as m_strSrcRootDir, in which case the original file is moved to the corresponding place in m_strTempRootDir. Based on settings, the original and/or all the intermediate files may be deleted.

There may also be "comparison" files, which are meant for cases like this: Some bits are modified inside an audio stream, now the file "looks" ok, but we want to see the impact this had on the audio. So the affected frames and several frames before and after them are saved in 2 "comp" files, "before" and "after", so they can be listened to in a separate app.

The get<...>Name() functions derive a name from the original name, the purpose of the folder, and a counter, such that it is located in the appropriate folder and there is no name conflict with an existing file.

Example 1:
    source:     /media/mp3/new/
    dest:       /tests/audio/
    proc:       /tests/proc/
    comp:       /tests/tmp/

    orig. name: /media/mp3/new/pop/act_two.mp3
    proc:       /tests/proc/pop/act_two.proc.001.mp3
    comp:       /tests/tmp/pop/act_two.tmp.before.003.mp3   and   /tests/tmp/pop/act_two.tmp.after.003.mp3


Example 2:
    source:     /media/mp3/new/
    dest:       /media/mp3/new/
    proc:       /tests/proc/
    comp:       /tests/tmp/

    orig. name: /media/mp3/new/pop/act_two.mp3
    proc:       /tests/proc/pop/act_two.proc.001.mp3
    comp:       /tests/tmp/pop/act_two.tmp.before.003.mp3   and   /tests/tmp/pop/act_two.tmp.after.003.mp3
    backup:     /tests/proc/pop/act_two.mp3  (or /tests/proc/pop/act_two.orig.002.mp3)



It's better for the directories to not overlap, but this is not enforced, as there may be valid reasons to use a folder for multiple purposes. One thing that should be avoided is to end up with comp or intermediate files in the same folder as source files.

Folder names must not end with the path separator character ("/" or "\"), or an IncorrectDirName is thrown (defined in OsFile.h).

Note: getTempName() and getCompNames() create the directories, if needed, so files can be created using the returned names.

Note: all file names are absolute.



//ttt2 more options to change what getBackupName() and getDestName() return and some other things:
    1) the name can be the original one or changed;
    2) the orig file may be left where it is, moved or deleted
    3) the dest file may be left where it is (in temp), moved to src, moved to dest, or perhaps even deleted (so only the "comp" files are left, for comparison)
    4) the comp and proc file may be left or removed; if they get deleted, it's probably better to not create a tree structure for directories
    5) perhaps "flatten" file names: "/tests/tmp/pop/JoQ/act_two.tmp.before.003.mp3" -> /tests/tmp/pop - JoQ - act_two.tmp.before.003.mp3"

*/
class TransfConfig
{
    Q_DECLARE_TR_FUNCTIONS(TransfConfig)

    //bool m_bRename
    std::string m_strSrcDir; // all scanned files are supposed to be here; // 2009.04.08 - for now it seems better to force this to be empty, by not allowing it to be edited // ttt2 perhaps put back, or calculate it from a session's folders in saveTransfConfig(), but seems likely to create confusion; can be edited manually in the config file to see what it does (or by commenting out "m_pSourceDirF->hide()")
    std::string m_strProcOrigDir;
    std::string m_strUnprocOrigDir;
    std::string m_strProcessedDir;
    std::string m_strTempDir;
    std::string m_strCompDir;

    // bit mapping:
    //
    //----------------------------
    //
    //  0-2:    000 = don't change processed orig file
    //          001 = erase processed orig file
    //          010 = move processed orig file to m_strProcOrigDir; always rename
    //          011 = move processed orig file to m_strProcOrigDir; rename only if name is in use
    //          100 = rename in the same dir
    //          101 = move w/o renaming, if doesn't exist; discard if it exists; (so there's only 1 orig)
    //              ??? move w/ renaming, if doesn't exist
    //  3:  0 = don't use identifying label when renaming proc orig
    //      1 = use identifying label when renaming proc orig
    //  4:  0 = use counter if name is in use when renaming proc orig
    //      1 = always use counter when renaming proc orig
    //
    //----------------------------
    //
    //  5-7:    000 = don't change unprocessed orig file
    //          001 = erase unprocessed orig file
    //          010 = move unprocessed orig file to m_strUnprocOrigDir; always rename
    //          011 = move unprocessed orig file to m_strUnprocOrigDir; rename only if name is in use
    //          100 = rename in the same dir
    //  8:  0 = don't use identifying label when renaming unproc orig
    //      1 = use identifying label when renaming unproc orig
    //  9: 0 = use counter if name is in use when renaming unproc orig
    //      1 = always use counter when renaming unproc orig
    //
    //----------------------------
    //
    //  10-11:  00 = don't create proc files
    //          01 = create proc files and always rename
    //          10 = create proc files and rename if the name is in use
    //  12: 0 = don't use identifying label when renaming proc
    //      1 = use identifying label when renaming proc
    //  13: 0 = use counter if name is in use when renaming proc
    //      1 = always use counter when renaming proc
    //  14: 0 = use the source dir as destination for proc files
    //      1 = use m_strProcessedDir as destination for proc files
    //
    //----------------------------
    //
    //  15: 0 = don't create temp files
    //      1 = create temp files in m_strTempDir
    //
    //----------------------------
    //
    //  16: 0 = don't create comp files
    //      1 = create comp files in m_strCompDir
    //
    //----------------------------
    //
    //  17: 0 = keep orig time
    //      1 = don't keep orig time
    //
    /*int m_nOptions;

    int getOpt(int nStartBit, int nCount) const { return (m_nOptions >> nStartBit) & ((1 << nCount) - 1); }*/

    void splitOrigName(const std::string& strOrigSrcName, std::string& strRelDir, std::string& strBaseName, std::string& strExt) const; // last '/' goes to dir; last '.' goes to ext (if extension present)

    enum { USE_COUNTER_IF_NEEDED, ALWAYS_USE_COUNTER };
    enum { RENAME_IF_NEEDED, ALWAYS_RENAME };
    enum { DONT_ALLOW_DUPLICATES, ALLOW_DUPLICATES };

    static std::string addLabelAndCounter(const std::string& s1, const std::string& s2, const std::string& strLabel, bool bAlwaysUseCounter, bool bAlwaysRename); // adds a counter and/or a label, such that a file with the resulting name doesn't exist;

    // normally makes sure that the name returned doesn't exist, by applying any renaming specified in the params; there's an exception, though: is bAllowDup is true, duplicates are allowed
    std::string getRenamedName(const std::string& strOrigSrcName, const std::string& strNewRootDir, const std::string& strLabel, bool bAlwayUseCounter, bool bAlwaysRename, bool bAllowDup = false) const;


public:
    TransfConfig(
        const std::string& strSrcDir,
        const std::string& strProcOrigDir,
        const std::string& strUnprocOrigDir,
        const std::string& strProcessedDir,
        const std::string& strTempDir,
        const std::string& strCompDir,
        int nOptions // may be -1 to indicate "default" values
        );
    TransfConfig();


    class Options
    {
    public:
        enum ProcOrig { PO_DONT_CHG, PO_ERASE, PO_MOVE_ALWAYS_RENAME, PO_MOVE_RENAME_IF_USED, PO_RENAME_SAME_DIR, PO_MOVE_OR_ERASE };
        enum UnprocOrig { UPO_DONT_CHG, UPO_ERASE, UPO_MOVE_ALWAYS_RENAME, UPO_MOVE_RENAME_IF_USED, UPO_RENAME_SAME_DIR }; // !!! the values in UnprocOrig must match those in ProcOrig for getChangedOrigNameHlp() to work correctly
        enum Processed { PR_DONT_CREATE, PR_CREATE_ALWAYS_RENAME, PR_CREATE_RENAME_IF_USED };

    private:
        static unsigned uns(bool b) { return b ? 1 : 0; }
        static unsigned uns(unsigned x) { return x; }
        static unsigned uns(ProcOrig x) { return x; }
        static unsigned uns(UnprocOrig x) { return x; }
        static unsigned uns(Processed x) { return x; }

    public:

        Options(); // initializes the fields to a "non-backup" state

        ProcOrig m_eProcOrigChange : 4; // !!! really "3", but declares more bits than used to avoid issues with signed/unsigned enums
        bool m_bProcOrigUseLabel : 1;
        bool m_bProcOrigAlwayUseCounter : 1;

        UnprocOrig m_eUnprocOrigChange : 4; // !!! really "3", but declares more bits than used to avoid issues with signed/unsigned enums
        bool m_bUnprocOrigUseLabel : 1;
        bool m_bUnprocOrigAlwayUseCounter : 1;

        Processed m_eProcessedCreate : 3; // !!! really "2", but declares more bits than used to avoid issues with signed/unsigned enums
        bool m_bProcessedUseLabel : 1;
        bool m_bProcessedAlwayUseCounter : 1;
        bool m_bProcessedUseSeparateDir : 1;

        bool m_bTempCreate : 1;

        bool m_bCompCreate : 1;

        bool m_bKeepOrigTime : 1;


        int getVal() const
        {
            unsigned x (0); unsigned s (0);

            x ^= (uns(m_eProcOrigChange) << s); s += 3; // unsigned m_nProcOrigChange : 3;
            x ^= (uns(m_bProcOrigUseLabel) << s); s += 1; // bool m_bProcOrigUseLabel : 1;
            x ^= (uns(m_bProcOrigAlwayUseCounter) << s); s += 1; // bool m_bProcOrigAlwayUseCounter : 1;

            x ^= (uns(m_eUnprocOrigChange) << s); s += 3; // unsigned m_nUnprocOrigChange : 3;
            x ^= (uns(m_bUnprocOrigUseLabel) << s); s += 1; // bool m_bUnprocOrigUseLabel : 1;
            x ^= (uns(m_bUnprocOrigAlwayUseCounter) << s); s += 1; // bool m_bUnprocOrigAlwayUseCounter : 1;

            x ^= (uns(m_eProcessedCreate) << s); s += 2; // unsigned m_nProcessedCreate : 2;
            x ^= (uns(m_bProcessedUseLabel) << s); s += 1; // bool m_bProcessedUseLabel : 1;
            x ^= (uns(m_bProcessedAlwayUseCounter) << s); s += 1; // bool m_bProcessedAlwayUseCounter : 1;
            x ^= (uns(m_bProcessedUseSeparateDir) << s); s += 1; // bool m_bProcessedUseSeparateDir : 1;

            x ^= (uns(m_bTempCreate) << s); s += 1; // bool m_bTempCreate : 1;

            x ^= (uns(m_bCompCreate) << s); s += 1; // bool m_bCompCreate : 1;

            x ^= (uns(m_bKeepOrigTime) << s); s += 1; // bool m_bKeepOrigTime : 1;

            return int(x);
        }

        void setVal(int x)
        {
            m_eProcOrigChange = ProcOrig(x & 0x07); x >>= 3; // unsigned m_nProcOrigChange : 3;
            m_bProcOrigUseLabel = x & 0x01; x >>= 1; // bool m_bProcOrigUseLabel : 1;
            m_bProcOrigAlwayUseCounter = x & 0x01; x >>= 1; // bool m_bProcOrigAlwayUseCounter : 1;

            m_eUnprocOrigChange = UnprocOrig(x & 0x07); x >>= 3; // unsigned m_nUnprocOrigChange : 3;
            m_bUnprocOrigUseLabel = x & 0x01; x >>= 1; // bool m_bUnprocOrigUseLabel : 1;
            m_bUnprocOrigAlwayUseCounter = x & 0x01; x >>= 1; // bool m_bUnprocOrigAlwayUseCounter : 1;

            m_eProcessedCreate = Processed(x & 0x03); x >>= 2; // unsigned m_nProcessedCreate : 2;
            m_bProcessedUseLabel = x & 0x01; x >>= 1; // bool m_bProcessedUseLabel : 1;
            m_bProcessedAlwayUseCounter = x & 0x01; x >>= 1; // bool m_bProcessedAlwayUseCounter : 1;
            m_bProcessedUseSeparateDir = x & 0x01; x >>= 1; // bool m_bProcessedUseSeparateDir : 1;

            m_bTempCreate = x & 0x01; x >>= 1; // bool m_bTempCreate : 1;

            m_bCompCreate = x & 0x01; x >>= 1; // bool m_bCompCreate : 1;

            m_bKeepOrigTime = x & 0x01; x >>= 1; // bool m_bKeepOrigTime : 1;
        }

        bool operator==(const Options& opt) const
        {
            return getVal() == opt.getVal();
        }

        // backup is 3-state; based on various fields, options may be "backup", "non-backup", or neither of them
        Options asBackup() const; // returns *this, with some fields changed to match a "backup" configuration; touches many field, but ignores others (m_bProcOrigUseLabel, m_bProcOrigAlwayUseCounter, m_bUnprocOrigUseLabel, m_bUnprocOrigAlwayUseCounter, m_bKeepOrigTime)
        Options asNonBackup() const; // returns *this, with some fields changed to match a "non-backup" configuration; touches many field, but ignores others (m_bProcOrigUseLabel, m_bProcOrigAlwayUseCounter, m_bUnprocOrigUseLabel, m_bUnprocOrigAlwayUseCounter, m_bKeepOrigTime)
    };

    Options m_options;

/*
    // the name for an "intermediate" file; it's obtained from the "temp dir" and from the short name of the original file, with a ".temp.<NNN>" inserted before the extension, where <NNN> is a number chosen such that a file with this new name doesn't exist;
    std::string getTempName(const std::string& strOrigSrcName, const std::string& strOpName) const;

    // the names for a pair of "compare", "before and after", files; they are obtained from the "comp dir" and from the short name of the original file, with ".before.<NNN>" / ".after.<NNN>" inserted before the extension, where <NNN> is a number chosen such that files with the new names don't exist;
    void getCompNames(const std::string& strOrigSrcName, const std::string& strOpName, std::string& strBefore, std::string& strAfter) const;

    // to what an original file should be renamed, if it was "processed";
    // if m_strSrcRootDir and m_strProcRootDir are different, it returns ""; (the original file doesn't get touched);
    // if m_strSrcRootDir and m_strProcRootDir are equal, it returns a name based on the original, with an ".orig.<NNN>" inserted before the extension, where <NNN> is a number chosen such that a file with this new name doesn't exist;
    std::string getBackupName(const std::string& strOrigSrcName) const;

    // to what the last "processed" file should be renamed;
    // if m_strSrcRootDir and m_strProcRootDir are different, it returns strOrigSrcName, moved to m_strProcRootDir;
    // if m_strSrcRootDir and m_strProcRootDir are equal, it returns strOrigSrcName;
    // if the name that it would return already exists, it inserts a ".Proc.<NNN>" such that a file with this new name doesn't exist; (so if m_strSrcRootDir and m_strProcRootDir are equal, the original file should be renamed before calling this)
    std::string getProcName(const std::string& strOrigSrcName) const;
*/

    enum OrigFile { ORIG_DONT_CHANGE, ORIG_ERASE, ORIG_MOVE, ORIG_MOVE_OR_ERASE };
    enum TransfFile { TRANSF_DONT_CREATE, TRANSF_CREATE };

    OrigFile getProcOrigAction() const;
    OrigFile getUnprocOrigAction() const;
    TransfFile getProcessedAction() const;
    TransfFile getTempAction() const;
    TransfFile getCompAction() const;

    OrigFile getProcOrigName(std::string strOrigSrcName, std::string& strNewName) const;
    OrigFile getUnprocOrigName(std::string strOrigSrcName, std::string& strNewName) const;
    TransfFile getProcessedName(std::string strOrigSrcName, std::string& strName) const;
    TransfFile getTempName(std::string strOrigSrcName, const std::string& strOpName, std::string& strName) const;
    TransfFile getCompNames(std::string strOrigSrcName, const std::string& strOpName, std::string& strBefore, std::string& strAfter) const;


    //these are needed for saving the configuration
    const std::string& getSrcDir() const { return m_strSrcDir; }
    const std::string& getProcOrigDir() const { return m_strProcOrigDir; }
    const std::string& getUnprocOrigDir() const { return m_strUnprocOrigDir; }
    const std::string& getProcessedDir() const { return m_strProcessedDir; }
    const std::string& getTempDir() const { return m_strTempDir; }
    const std::string& getCompDir() const { return m_strCompDir; }

    void setProcOrigDir(const std::string& s) { m_strProcOrigDir = s; } // needed by the session dialog //ttt2 perhaps some checks

    int getOptions() const { return m_options.getVal(); }

    bool hadInitError() const { return m_bInitError; }

    //struct DirOverlap {};
    //struct InvalidName {}; // something is wrong with the file name
    struct IncorrectPath {}; // thrown if a "source file" is outside of the "source folder" (takes care of this case too, where it's not enough to check for a substring: src="/tst/dir1", origName="/tst/dir10/file1.mp3")

    void testRemoveSuffix() const;
private:
    OrigFile getChangedOrigNameHlp(const std::string& strOrigSrcName, const std::string& strDestDir, int nChange, bool bUseLabel, bool bAlwayUseCounter, std::string& strNewName) const; // !!! nChange will get initialized from either ProcOrig or UnprocOrig, so their values must match
    void removeSuffix(std::string& s) const;
    bool m_bInitError;
};


class Mp3Handler;

class Transformation
{
    Q_DECLARE_TR_FUNCTIONS(Transformation)
public:
    virtual ~Transformation() {}

    enum Result { NOT_CHANGED, CHANGED, CHANGED_NO_RECALL };
    /*struct Result {};
    static Result NOT_CHANGED;
    static Result CHANGED;
    static Result CHANGED_NO_RECALL;*/

    virtual Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName) = 0; // this may throw
    virtual const char* getActionName() const = 0; // should return the same thing for all objects of a class, as this is used in string comparisons in several places (visible transformations, custom transformation lists, ...)
    virtual QString getVisibleActionName() const { return tr(getActionName()); } // to be used only by the UI, providing more details to the user
    virtual const char* getDescription() const = 0;

    virtual bool acceptsFastSave() const { return false; } // whether to consider Mp3Handler::m_nFastSaveTime as a match when deciding if a file was changed (so a transformation can't be applied)

    struct InvalidInputFile {}; // thrown if the input file was changed so it can no longer be processed
};


class IdentityTransformation : public Transformation
{
public:
    /*override*/ Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return QT_TRANSLATE_NOOP("Transformation", "Doesn't actually change the file, but it creates a temporary copy and it reports that it does change it. This is not as meaningless as it might first seem: if the configuration settings indicate some action (i.e. rename, move or erase) to be taken for processed files, then that action will be performed for these files. While the same can be achieved by changing the settings for unprocessed files, this is easier to use when it is executed on a subset of all the files (filtered or selected)."); }

    static const char* getClassName() { return QT_TRANSLATE_NOOP("Transformation", "No change"); }
};





#endif // #ifndef TransformationH

