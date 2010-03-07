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


#ifndef OsFileH
#define OsFileH

#include  <memory>
#include  <string>


#define CB_LIB_CALL

class FileSearcherImpl;

class FileSearcher
{
    CB_LIB_CALL FileSearcher(const FileSearcher&);
    FileSearcher& CB_LIB_CALL operator=(const FileSearcher&);

    std::auto_ptr<FileSearcherImpl> m_pImpl;
    bool CB_LIB_CALL goToNextValidEntry(); // skips "." and "..", as well as any invalid item (usually file that has been removed after entryInfoList() got called)
    std::string m_strDir;
public:
    CB_LIB_CALL FileSearcher();
    CB_LIB_CALL FileSearcher(const std::string& strDirName); // accepts both "/"-terminated and non-'/'-terminated names

    CB_LIB_CALL ~FileSearcher();

    //ttt2 this seems bad design: getName() & Co shouldn't be callable if the search is not open
    std::string CB_LIB_CALL getName() const; // returns the full name found; throws if nothing found
    std::string CB_LIB_CALL getShortName() const; // returns the short name found; throws if nothing found //ttt2 have short/long/default (default is for "../tst/1" and is what is implemented now; long should be "/dir/tst/1")
    int CB_LIB_CALL getAttribs() const; //ddd maybe filter out volume attributes / or include UNIX attribs
    long long CB_LIB_CALL getSize() const;
    //long long CB_LIB_CALL getCreationTime() const; //int64FromFileDateTime(sr.sr.FindData.ftCreationTime)  //ttt2 replace "long long" with a class representing time with nanosecond resolution, with conversions to what various OSs are using for various tasks
    long long CB_LIB_CALL getChangeTime() const; //int64FromFileDateTime(sr.sr.FindData.ftLastWriteTime),
    bool CB_LIB_CALL isFile() const;
    bool CB_LIB_CALL isDir() const;
    bool CB_LIB_CALL isSymLink() const;

    CB_LIB_CALL operator bool() const;

    // it's easier to use the constructor, but sometimes may be more convenient to leave the object in an outer loop
    bool CB_LIB_CALL findFirst(const std::string& strDirName);

    bool CB_LIB_CALL findNext();

    void CB_LIB_CALL close();

    struct InvalidOperation {};
};


// result is in UTC
//TDateTime dateTimeFromFileDateTime(const FILETIME& utcFileTime);

//long long int64FromFileDateTime(const FILETIME& utcFileTime);
//FILETIME fileDateTimeFromint64(__int64 intTime);


/*struct CantOpenFile// : public RuntimeError
{
//    CB_LIB_CALL CantOpenFile(const char* szFileName, int nLn, const char* szFile, RuntimeError* pPrv = 0) :
//            RuntimeError(TMP_PARAM_CHG2, "Error opening file", "Couldn't open file " + strFileName, nLn, szFile, pPrv) {}
};*/



void CB_LIB_CALL getFileInfo(const std::string& strFileName, long long& nChangeTime, long long& nSize); // throws NameNotFound // ttt2 currently doesn't really matter the format of nChangeTime, because it is only used in equality comparisons; so it uses time_t; however, this should be reviewed and probably choose nanoseconds from 01.01.1970 (QDateTime is better avoided, because of several issues, like serialization or low resolution)
void CB_LIB_CALL setFileDate(const std::string& strFileName, long long nChangeTime);


struct CannotCreateDir
{
    std::string m_strDir;
    CannotCreateDir(const std::string& strDir) : m_strDir(strDir) {}
};


// creates all intermediate dirs; throws CannotCreateDir on failure (if the directory already exists it's a success)
void CB_LIB_CALL createDir(const std::string& strDirName);

void CB_LIB_CALL createDirForFile(const std::string& strFileName); // creates the directory where the given file can be created

std::string replaceDriveLetter(const std::string& strFileName); // does nothing on Linux; replaces "D:" with "/D" on Windows, only when "D:" isn't at the beggining of the string;

// returns true if there is a file with that name;
// returns false if the name doesn't exist or it's a directory; doesn't throw
bool CB_LIB_CALL fileExists(const std::string& strFileName);

// returns true if there is a directory with that name;
// returns false if the name doesn't exist or it's a file; //ttt2 throws IncorrectDirName
bool CB_LIB_CALL dirExists(const std::string& strDirName);


struct IncorrectDirName {}; // invalid dir name (e.g. ends with a path separator)

// throws IncorrectDirName if the name ends with a path separator
void CB_LIB_CALL checkDirName(const std::string& strDirName);

std::string getSepTerminatedDir(const std::string& strDirName); // adds a path separator at the end if none is present; throws IncorrectDirName on invalid file names
std::string getNonSepTerminatedDir(const std::string& strDirName); // removes the path separator at the end if present; throws IncorrectDirName on invalid file names


struct FoundDir {};
struct AlreadyExists {};
struct NameNotFound {};
struct CannotRenameFile {};
struct CannotCopyFile {};

// renames a file;
// throws FoundDir, AlreadyExists, NameNotFound, CannotRenameFile, ?IncorrectDirName,
void CB_LIB_CALL renameFile(const std::string& strOldName, const std::string& strNewName);


// creates a copy a file;
// doesn't preserve any attributes; // ttt2 add option
// throws WriteError or EndOfFile from Helpers //ttt2 switch to: throws FoundDir, AlreadyExists, NameNotFound, CannotCopyFile, ?IncorrectDirName,
void CB_LIB_CALL copyFile(const std::string& strSourceName, const std::string& strDestName /*, OverwriteOption eOverwriteOption*/);

// throws FoundDir, AlreadyExists, NameNotFound, CannotCopyFile, ?IncorrectDirName, //ttt2 unify with copyFile(), or rather restructure all file operations (perhaps see what Qt has)
void CB_LIB_CALL copyFile2(const std::string& strSourceName, const std::string& strDestName /*, OverwriteOption eOverwriteOption*/);


// deletes a file; throws FoundDir,
// CannotDeleteFile, ?IncorrectDirName; it is OK if the file didn't exist to begin with
struct CannotDeleteFile {};
void CB_LIB_CALL deleteFile(const std::string& strFileName);

// just a name that doesn't exist; the file won't be deleted automatically; normally the name is obtained by appending something to strMasterFileName, but a more generic temp is used if the name is too long on wnd
std::string getTempFile(const std::string& strMasterFileName);

std::string getParent(const std::string& strName); // the name of a parent directory; returns an empty string if no parent exists; may throw IncorrectDirName

bool isInsideDir(const std::string& strName, const std::string& strDirName); // if strName is in strDirName or in one of its subdirectories; may throw IncorrectDirName for either param

std::string getExistingDir(const std::string& strName); // if strName exists and is a dir, it is returned; otherwise it returns the closest ancestor that exists; accepts names ending with file separator

#endif // #ifndef OsFileH
