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


#include  <iostream>
#include  <fstream>
#include  <cerrno>
#include  <algorithm>

#ifndef WIN32
    #include  <glob.h>
#else
    #include <QFileInfo>
    #include <QDateTime>
#endif

#include  <sys/stat.h>
#include  <utime.h>
//#include  <sys/types.h>
//#include  <unistd.h>
#include <dirent.h>

#include  "OsFile.h"

#include  "Helpers.h"

//---------------------------------------------------------------------------

using namespace std;
using namespace pearl;

//namespace ciobi_utils {


//ttt1 Linux-specifc; see how it works with MinGW



class FileSearcherImpl
{
    CB_LIB_CALL FileSearcherImpl(const FileSearcherImpl&);
    FileSearcherImpl& CB_LIB_CALL operator=(const FileSearcherImpl&);

public:
    //CB_LIB_CALL FileSearcherImpl() : m_hFind(INVALID_HANDLE_VALUE), m_findData() {}
    CB_LIB_CALL FileSearcherImpl() : m_bOpen(false), m_pDir(0), m_pEnt(0) {}

//    HANDLE m_hFind;
//    WIN32_FIND_DATA m_findData;
    bool m_bOpen;
    //const char* m_szCurrentPath;

#ifndef WIN32
    struct stat64 m_crtStat;
#endif

    //glob_t m_findData;
    //size_t m_nCrtMatch;

    DIR* m_pDir;
    dirent* m_pEnt;
};



CB_LIB_CALL FileSearcher::FileSearcher() : m_pImpl(new FileSearcherImpl)
{
}


CB_LIB_CALL FileSearcher::FileSearcher(const string& strDir) : m_pImpl(new FileSearcherImpl)
{
    findFirst(strDir);
}


//TSearchRec sr;
CB_LIB_CALL FileSearcher::~FileSearcher()
{
    //FindClose(sr);
    close();
}




CB_LIB_CALL FileSearcher::operator bool() const
{
    return m_pImpl->m_bOpen;
}

#ifndef WIN32

bool CB_LIB_CALL FileSearcher::isFile() const
{
//cout << "#######################" << getName() << " " << m_pImpl->m_crtStat.st_mode << endl;
    //return 0 == (FILE_ATTRIBUTE_DIRECTORY & m_pImpl->m_findData.dwFileAttributes);
    //CB_CHECK (
    //const char* szCrtPath(m_pImpl->getCurrentPath());
    return S_ISREG(m_pImpl->m_crtStat.st_mode);
}

bool CB_LIB_CALL FileSearcher::isDir() const
{
//cout << "#######################" << getName() << " " << m_pImpl->m_crtStat.st_mode << endl;
    return /* !S_ISLNK(m_pImpl->m_crtStat.st_mode) &&*/ S_ISDIR(m_pImpl->m_crtStat.st_mode);
    //return false;
}


bool CB_LIB_CALL FileSearcher::isSymLink() const
{
//cout << "#######################" << getName() << " " << m_pImpl->m_crtStat.st_mode << endl;
    return S_ISLNK(m_pImpl->m_crtStat.st_mode);
    //return false;
}

#else

bool CB_LIB_CALL FileSearcher::isFile() const
{
    return QFileInfo(convStr(getName())).isFile();
}

bool CB_LIB_CALL FileSearcher::isDir() const
{
    return QFileInfo(convStr(getName())).isDir();
}


bool CB_LIB_CALL FileSearcher::isSymLink() const
{
    return false;
}
#endif


// it's easier to use the constructor, but sometimes may be more convenient to leave the object in an outer loop
bool CB_LIB_CALL FileSearcher::findFirst(const string& strDir)
{
    close();
#if 0
    int nLen (strlen(szName));
    const char* szEscName;
    char* pNewStr;
    int nParCnt (count(szName, szName + nLen, '[') + count(szName, szName + nLen, ']'));
    if (0 == nParCnt)
    {
        szEscName = szName;
        pNewStr = 0;
    }
    else
    {
        // '[' and ']' need to be escaped
        pNewStr = new char[nParCnt + nLen + 1];
        char* q (pNewStr);
        for (int i = 0; i < nLen; ++i)
        {
            if ('[' == szName[i] || ']' == szName[i])
            {
                *q++ = '\\';
            }
            *q++ = szName[i];
        }
        *q = 0;

        /*pNewStr = new char[nParCnt + 2*nLen + 1];
        char* q (pNewStr);
        for (int i = 0; i < nLen; ++i)
        {
            if ('[' == szName[i] || ']' == szName[i])
            {
                *q++ = '[';
                *q++ = szName[i];
                *q++ = ']';
            }
            else
            {
                *q++ = szName[i];
            }
        }*/

        szEscName = pNewStr;
    }

    ArrayPtrRelease<char> rel (pNewStr);

//cout << "############### find in " << szEscName << endl;

    CB_CHECK1 (!(*this), InvalidOperation()/*"findFirst() may only be called on a closed searcher"*/);
    m_pImpl->m_findData.gl_offs = 0;
    //bool bRes (0 == glob(szEscName, 0, 0, &m_pImpl->m_findData));
    bool bRes (0 == glob(szEscName, GLOB_PERIOD, 0, &m_pImpl->m_findData)); // GLOB_PERIOD is GNU-specific, but without it names starting with a period aren't included //ttt2 make sure that m_pImpl->m_findData doesn't need deallocation upon failure

    if (bRes)
    {
        m_pImpl->m_bOpen = true;
        CB_ASSERT(0 < m_pImpl->m_findData.gl_pathc);

        //CB_ASSERT (0 == strcmp(".", m_pImpl->m_findData.gl_pathv[0]) && 0 == strcmp("..", m_pImpl->m_findData.gl_pathv[1])); //ttt2 see how valid this assert is (it seems to work on SuSE 10.3 64bit, including the root dir)
        m_pImpl->m_nCrtMatch = 0;
        advanceToValidEntry();
        if (!*this)
        {
            return false;
        }

        if (0 != lstat64(m_pImpl->m_findData.gl_pathv[m_pImpl->m_nCrtMatch], &m_pImpl->m_crtStat))
        {
            throw 1; //ttt1 deal with the error
        }
    }

    return bRes;
#endif
    if (endsWith(strDir, getPathSepAsStr()))
    {
        m_strDir = strDir;
    }
    else
    {
        m_strDir = strDir + getPathSepAsStr();
    }

    m_pImpl->m_pDir = opendir(m_strDir.c_str());
    if (0 == m_pImpl->m_pDir) { return false; }

    m_pImpl->m_bOpen = true;
    return goToNextValidEntry();

}


// skips "." and "..", as well as any invalid item (usually file that has been removed after opendir() or readdir() got called)
bool CB_LIB_CALL FileSearcher::goToNextValidEntry()
{
    if (!m_pImpl->m_bOpen) { return false; }

    for (;;)
    {
        m_pImpl->m_pEnt = readdir(m_pImpl->m_pDir);
        if (0 == m_pImpl->m_pEnt) { break; }

        const char* p (m_pImpl->m_pEnt->d_name);
        if (0 == strcmp(".", p) || 0 == strcmp("..", p)) { continue; }

#ifndef WIN32
        if (0 != lstat64((m_strDir + m_pImpl->m_pEnt->d_name).c_str(), &m_pImpl->m_crtStat)) { continue; } //ttt1 see if seeking the next is the best way to deal with this error
#else
        if (!QFileInfo(convStr(m_strDir + m_pImpl->m_pEnt->d_name)).exists()) { continue; } //ttt1 see if seeking the next is the best way to deal with this error
#endif
        return true;
    }

    close();
    return false;
}


bool CB_LIB_CALL FileSearcher::findNext()
{
    CB_CHECK1 (*this, InvalidOperation()/*"findNext() may only be called on an open searcher"*/);
    return goToNextValidEntry();
}


void CB_LIB_CALL FileSearcher::close()
{
    if (m_pImpl->m_bOpen)
    {
        closedir(m_pImpl->m_pDir);
        m_pImpl->m_bOpen = false;

        m_pImpl->m_pDir = 0;
        m_pImpl->m_pEnt = 0;
    }
}

string CB_LIB_CALL FileSearcher::getName() const
{
    CB_CHECK1 (*this, InvalidOperation()/*"getName() may only be called on an open searcher"*/);
    //return m_pImpl->m_pEnt->d_name;
    return m_strDir + m_pImpl->m_pEnt->d_name;
}


long long CB_LIB_CALL FileSearcher::getSize() const
{
    CB_CHECK1 (*this, InvalidOperation()/*"getSize() may only be called on an open searcher"*/);
#ifndef WIN32
    return (long long)m_pImpl->m_crtStat.st_size;
#else
    return QFileInfo(convStr(getName())).size();
#endif
}

#if 0
long long CB_LIB_CALL FileSearcher::getCreationTime() const
{
    CB_CHECK1 (*this, InvalidOperation()/*"getCreationTime() may only be called on an open searcher"*/);
#ifndef WIN32
    return m_pImpl->m_crtStat.st_mtime; //ttt probably wrong; see who uses it
#else
    return QFileInfo(convStr(getName())).lastModified().toTime_t(); //ttt3 32bit  //ttt wrong; see who uses it
#endif
}
#endif

long long CB_LIB_CALL FileSearcher::getChangeTime() const
{
    CB_CHECK1 (*this, InvalidOperation()/*"getChangeTime() may only be called on an open searcher"*/);
#ifndef WIN32
    return m_pImpl->m_crtStat.st_mtime;
#else
    return QFileInfo(convStr(getName())).lastModified().toTime_t(); //ttt3 32bit
#endif
}

/*int CB_LIB_CALL FileSearcher::getAttribs() const
{
    return m_pImpl->m_findData.dwFileAttributes;
}*/


//void CB_LIB_CALL getFileInfo(const char* szFileName, long long & nCreationTime, long long & nChangeTime, long long& nSize)
void CB_LIB_CALL getFileInfo(const char* szFileName, long long & nChangeTime, long long& nSize)
{
    struct stat s;

    if (0 != stat(szFileName, &s))
    {
        throw NameNotFound();
        //throw CannotGetData(strFileName, getOsError(), LI);
    }

    nChangeTime = s.st_mtime;

    nSize = s.st_size;
}

//void CB_LIB_CALL setFileDate(const char* szFileName, time_t nCreationTime, long long nChangeTime)
void CB_LIB_CALL setFileDate(const char* szFileName, long long nChangeTime)
{
    utimbuf t;
    t.actime = (time_t)nChangeTime;
    t.modtime = (time_t)nChangeTime;
    if (0 != utime(szFileName, &t))
    {
        //throw CannotSetDates(strFileName, getOsError(), LI);
        throw 1; //ttt1
    }
}



// throws IncorrectDirName if the name ends with a path separator
void CB_LIB_CALL checkDirName(const string& strDir)
{
    CB_CHECK1 (!endsWith(strDir, getPathSepAsStr()), IncorrectDirName());
    //ttt2 more checks
}



// returns true if there is a file with that name;
// returns false if the name doesn't exist or it's a directory; doesn't throw
bool CB_LIB_CALL fileExists(const std::string& strFile)
{
    struct stat s;
    if (stat(strFile.c_str(), &s) < 0)
    { // nothing exists
        return false;
    }

    if (S_IFREG == (s.st_mode & S_IFMT))
    {
        return true;
    }

    return false; // exists but it's something else (dir, symlink, ...)
}



// returns true if there is a directory with that name;
// returns false if the name doesn't exist or it's a file; //ttt2 throws IncorrectDirName
bool CB_LIB_CALL dirExists(const std::string& strDir)
{
    if (strDir.empty()) { return true; }

    checkDirName(strDir);

    struct stat s;
    if (stat(strDir.c_str(), &s) < 0)
    { // nothing exists
        return false;
    }

    if (S_IFDIR == (s.st_mode & S_IFMT))
    {
        return true;
    }

    return false; // exists but it's something else (reg file, symlink, ...)
}


void CB_LIB_CALL createDir(const char* szDirName)
{
    if (0 == *szDirName) { return; } // the root dir always exists

    checkDirName(szDirName);

    if (dirExists(szDirName))
    {
        return;
    }

    const char* p (strrchr(szDirName, getPathSep()));
    CB_CHECK1 (0 != p, CannotCreateDir());
    string strParent (szDirName, p - szDirName);
    createDir(strParent.c_str());
    mkdir(szDirName
#ifndef WIN32
           , S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH
#else
#endif
        );

    CB_CHECK1 (dirExists(szDirName), CannotCreateDir());
}


void CB_LIB_CALL createDirForFile(const std::string& strFile)
{
    string::size_type n (strFile.find_last_of(getPathSep()));
    CB_ASSERT (string::npos != n);
    createDir(strFile.substr(0, n).c_str());
}

// does nothing on Linux; replaces "D:" with "/D" on Windows, only when "D:" isn't at the beggining of the string;
string replaceDriveLetter(const string& strFile)
{
#ifndef WIN32
    return strFile;
#else
    string s (strFile);
    for (string::size_type n = 2;;  )
    {
        n = s.find(':', n);
        if (string::npos == n) { return s; }
        s[n] = s[n - 1];
        s[n - 1] = '/';
    }
#endif
}

// adds a path separator at the end if none is present; throws IncorrectDirName on invalid file names
string getSepTerminatedDir(const string& strDir)
{
    string strRes (strDir);
    if (!endsWith(strDir, getPathSepAsStr()))
    {
        strRes += getPathSep();
    }
    checkDirName(strRes.substr(0, strRes.size() - 1));
    return strRes;
}


// removes the path separator at the end if present; throws IncorrectDirName on invalid file names
string getNonSepTerminatedDir(const string& strDir)
{
    string strRes (strDir);
    if (endsWith(strDir, getPathSepAsStr()))
    {
        strRes.erase(strRes.size() - 1);
    }
    checkDirName(strRes);
    return strRes;
}


// renames a file;
// throws FoundDir, AlreadyExists, NameNotFound, CannotRenameFile, ?IncorrectDirName,
void CB_LIB_CALL renameFile(const std::string& strOldName, const std::string& strNewName)
{
    CB_CHECK1 (!dirExists(strNewName), FoundDir());
    CB_CHECK1 (!dirExists(strOldName), FoundDir()); // ttt2 separate OldFoundDir / NewFoundDir
    CB_CHECK1 (!fileExists(strNewName), AlreadyExists());
    CB_CHECK1 (fileExists(strOldName), NameNotFound());
    createDirForFile(strNewName); //ttt3 undo on error

    int n (rename(strOldName.c_str(), strNewName.c_str()));
    int nErr (errno);
    if (0 != n && EXDEV != nErr) { cerr << "I/O Error " << nErr << ": " << (strerror(nErr)) << endl; }

    if (0 != n && EXDEV == nErr)
    { // rename only works on a single drive, so work around this //ttt2 warn if this is called a lot
        try
        {
            copyFile2(strOldName, strNewName);
        }
        catch (const CannotCopyFile&)
        {
            throw CannotRenameFile();
        }
        deleteFile(strOldName);
        return;
    }

    CB_CHECK1 (0 == n, CannotRenameFile());
}


// creates a copy a file;
// doesn't preserve any attributes; // ttt2 add option
// throws WriteError or EndOfFile from Helpers //ttt2 switch to: throws FoundDir, AlreadyExists, NameNotFound, CannotCopyFile, ?IncorrectDirName,
void CB_LIB_CALL copyFile(const std::string& strSourceName, const std::string& strDestName /*, OverwriteOption eOverwriteOption*/)
{
    ifstream in (strSourceName.c_str(), ios::binary);
    ofstream out (strDestName.c_str(), ios::binary);
    streampos nSize (getSize(in));

    appendFilePart(in, out, 0, nSize);
    CB_CHECK1 (out, WriteError());
    streampos nOutSize (out.tellp());
    CB_CHECK1 (nOutSize == nSize, WriteError());
}


void CB_LIB_CALL copyFile2(const std::string& strSourceName, const std::string& strDestName /*, OverwriteOption eOverwriteOption*/)
{
    CB_CHECK1 (!dirExists(strDestName), FoundDir());
    CB_CHECK1 (!dirExists(strSourceName), FoundDir()); // ttt2 separate OldFoundDir / NewFoundDir
    CB_CHECK1 (!fileExists(strDestName), AlreadyExists());
    CB_CHECK1 (fileExists(strSourceName), NameNotFound());
    createDirForFile(strDestName); //ttt3 undo on error

    ifstream in (strSourceName.c_str(), ios::binary);
    ofstream out (strDestName.c_str(), ios::binary);
    streampos nSize (getSize(in));

    appendFilePart(in, out, 0, nSize);
    streampos nOutSize (out.tellp());
    if (!out || nOutSize != nSize)
    {
        out.close();
        deleteFile(strDestName);
        throw CannotCopyFile();
    }

    long long nChangeTime;
    long long x;
    getFileInfo(strSourceName.c_str(), nChangeTime, x);
    setFileDate(strDestName.c_str(), nChangeTime);
}




// deletes a file; throws FoundDir,
// CannotDeleteFile, ?IncorrectDirName; it is OK if the file didn't exist to begin with
void CB_LIB_CALL deleteFile(const std::string& strFile)
{
    CB_CHECK1 (!dirExists(strFile), FoundDir());
    if (!fileExists(strFile)) { return; }
    CB_CHECK1 (0 == unlink(strFile.c_str()), CannotDeleteFile());
}


string getTempFile()
{
    char a [L_tmpnam + 1];
    char* p (tmpnam(a)); //ttt3 while this is "not safe", it is not used in programs with any privileges; also, the functionality that is actually desired isn't implemented in C or C++ : mkstemp or tmpfile return a file descriptor / stream descriptor; we want streams and at any rate we may decide later that we want or don't want to delete the temporary file;
    CB_ASSERT (0 != p);
    //ttt1 there may be an issue with temporary file names if they are on a different partition than the proc files, and "rename" gets called a lot; perhaps we should just use the "proc" dir if it is not empty
    return a;
}


// the name of a parent directory; returns an empty string if no parent exists; may throw IncorrectDirName
string getParent(const string& strName)
{
    checkDirName(strName);

    string::size_type n (strName.find_last_of(getPathSep()));
    //CB_ASSERT(string::npos != n);
    if (string::npos == n) { return ""; }
    return strName.substr(0, n);
}


bool isInsideDir(const std::string& strName, const std::string& strDir) // if strName is in strDir or in one of its subdirectories; may throw IncorrectDirName for either param
{
    checkDirName(strName);
    checkDirName(strDir);
    return beginsWith(strName, strDir) && getPathSep() == strName[strDir.size()];
}


string getExistingDir(const std::string& strName) // if strName exists and is a dir, it is returned; otherwise it returns the closest ancestor that exists; accepts names ending with file separator
{
    string s (strName);
    if (endsWith(s, getPathSepAsStr()))
    {
        s.erase(s.size() - 1);
    }
    try
    {
        while (!s.empty() && !dirExists(s))
        {
            s = getParent(s);
        }
    }
    catch (const IncorrectDirName&)
    {
        s.clear();
    }
    return s;
}


//ttt2 perhaps use strerror_r() to print file errors


//} //namespace ciobi_utils


