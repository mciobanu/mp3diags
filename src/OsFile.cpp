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
#include  "fstream_unicode.h"
#include  <cerrno>
#include  <algorithm>

#include  <QFileInfo>
#include  <QDateTime>
#include  <QDir>

#include  <utime.h>
/*
#include  <sys/stat.h>
//#include  <sys/types.h>
//#include  <unistd.h>
#include <dirent.h>
*/
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
    CB_LIB_CALL FileSearcherImpl() : m_nCrtEntry(0) {}

    int m_nCrtEntry;
    QFileInfoList m_vFileInfos;

};



CB_LIB_CALL FileSearcher::FileSearcher() : m_pImpl(new FileSearcherImpl)
{
}


CB_LIB_CALL FileSearcher::FileSearcher(const string& strDirName) : m_pImpl(new FileSearcherImpl)
{
    findFirst(strDirName);
}


//TSearchRec sr;
CB_LIB_CALL FileSearcher::~FileSearcher()
{
    //FindClose(sr);
    close();
}




CB_LIB_CALL FileSearcher::operator bool() const
{
    return m_pImpl->m_nCrtEntry < m_pImpl->m_vFileInfos.size();
}


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
    return QFileInfo(convStr(getName())).isSymLink();
}


// it's easier to use the constructor, but sometimes may be more convenient to leave the object in an outer loop
bool CB_LIB_CALL FileSearcher::findFirst(const string& strDirName)
{
    close();

    if (endsWith(strDirName, getPathSepAsStr()))
    {
        m_strDir = strDirName;
    }
    else
    {
        m_strDir = strDirName + getPathSepAsStr();
    }

    m_pImpl->m_vFileInfos = QDir(convStr(m_strDir)).entryInfoList(QDir::NoDotAndDotDot);
    m_pImpl->m_nCrtEntry = 0;
    return goToNextValidEntry();
}


// skips "." and "..", as well as any invalid item (usually file that has been removed after entryInfoList() got called)
bool CB_LIB_CALL FileSearcher::goToNextValidEntry()
{
    for (;;)
    {
        if (!*this) { return false; }

        QString s (m_pImpl->m_vFileInfos[m_pImpl->m_nCrtEntry].fileName());
        CB_ASSERT (s != "." && s != ".."); // it was QDir::NoDotAndDotDot
        if (!m_pImpl->m_vFileInfos[m_pImpl->m_nCrtEntry].exists())
        {
            ++m_pImpl->m_nCrtEntry;
            continue; //ttt1 see if seeking the next is the best way to deal with this error
        }

        return true;
    }

    close();
    return false;
}


bool CB_LIB_CALL FileSearcher::findNext()
{
    CB_CHECK1 (*this, InvalidOperation()/*"findNext() may only be called on an open searcher"*/);
    ++m_pImpl->m_nCrtEntry;
    return goToNextValidEntry();
}


void CB_LIB_CALL FileSearcher::close()
{
    m_pImpl->m_vFileInfos.clear();
    m_pImpl->m_nCrtEntry = 0;
}

string CB_LIB_CALL FileSearcher::getName() const
{
    CB_CHECK1 (*this, InvalidOperation()/*"getName() may only be called on an open searcher"*/);
    return convStr(m_pImpl->m_vFileInfos[m_pImpl->m_nCrtEntry].absoluteFilePath());
}


long long CB_LIB_CALL FileSearcher::getSize() const
{
    CB_CHECK1 (*this, InvalidOperation()/*"getSize() may only be called on an open searcher"*/);
    return m_pImpl->m_vFileInfos[m_pImpl->m_nCrtEntry].size();
}


long long CB_LIB_CALL FileSearcher::getChangeTime() const
{
    CB_CHECK1 (*this, InvalidOperation()/*"getChangeTime() may only be called on an open searcher"*/);
    return m_pImpl->m_vFileInfos[m_pImpl->m_nCrtEntry].lastModified().toTime_t(); //ttt3 32bit
}

/*int CB_LIB_CALL FileSearcher::getAttribs() const
{
    return m_pImpl->m_findData.dwFileAttributes;
}*/


//void CB_LIB_CALL getFileInfo(const char* szFileName, long long & nCreationTime, long long & nChangeTime, long long& nSize)
void CB_LIB_CALL getFileInfo(const string& strFileName, long long & nChangeTime, long long& nSize)
{
    QFileInfo fi (convStr(strFileName));

    if (!fi.exists())
    {
        throw NameNotFound();
        //throw CannotGetData(strFileName, getOsError(), LI);
    }

    nChangeTime = fi.lastModified().toTime_t(); //ttt3 32bit
    nSize = fi.size();
}


//void CB_LIB_CALL setFileDate(const char* szFileName, time_t nCreationTime, long long nChangeTime)
void CB_LIB_CALL setFileDate(const string& strFileName, long long nChangeTime)
{
#ifndef WIN32
    utimbuf t;
    t.actime = (time_t)nChangeTime;
    t.modtime = (time_t)nChangeTime;
    if (0 != utime(strFileName.c_str(), &t))
    {
        //throw CannotSetDates(strFileName, getOsError(), LI);
        throw 1; //ttt1
    }
#else
    _utimbuf t;
    t.actime = (time_t)nChangeTime;
    t.modtime = (time_t)nChangeTime;
    if (0 != _wutime(wstrFromUtf8(strFileName).c_str(), &t))
    {
        //throw CannotSetDates(strFileName, getOsError(), LI);
        throw 1; //ttt1
    }
#endif
}


// throws IncorrectDirName if the name ends with a path separator
void CB_LIB_CALL checkDirName(const string& strDirName)
{
    CB_CHECK1 (!endsWith(strDirName, getPathSepAsStr()), IncorrectDirName());
    //ttt2 more checks
}



// returns true if there is a file with that name;
// returns false if the name doesn't exist or it's a directory; doesn't throw
bool CB_LIB_CALL fileExists(const std::string& strFileName)
{
    QFileInfo fi (convStr(strFileName));
    return fi.isFile(); // ttt1 not sure if this should allow symlinks
}



// returns true if there is a directory with that name;
// returns false if the name doesn't exist or it's a file; //ttt2 throws IncorrectDirName
bool CB_LIB_CALL dirExists(const std::string& strDirName)
{
    if (strDirName.empty()) { return true; }

    checkDirName(strDirName);

    string strDir1 (strDirName);

#ifndef WIN32
#else
    if (2 == cSize(strDir1))
    {
        strDir1 += getPathSepAsStr();
    }
#endif

    QFileInfo fi (convStr(strDir1));
    return fi.isDir(); // ttt1 not sure if this should allow symlinks
}


void CB_LIB_CALL createDir(const string& strDirName)
{
    if (strDirName.empty()) { return; } // the root dir always exists //ttt0 test create backup dir in root on wnd

    checkDirName(strDirName);

    if (dirExists(strDirName))
    {
        return;
    }

    string::size_type n (strDirName.rfind(getPathSep()));
    CB_CHECK1 (string::npos != n, CannotCreateDir());
    string strParent (strDirName.substr(0, n));
    createDir(strParent);

    QFileInfo fi (convStr(strDirName));
    CB_CHECK1 (fi.dir().mkdir(fi.fileName()), CannotCreateDir());

    CB_CHECK1 (dirExists(strDirName), CannotCreateDir());
}


void CB_LIB_CALL createDirForFile(const std::string& strFileName)
{
    string::size_type n (strFileName.find_last_of(getPathSep())); //ttt0 review all find_last_of
    CB_ASSERT (string::npos != n);
    createDir(strFileName.substr(0, n));
}

// does nothing on Linux; replaces "D:" with "/D" on Windows, only when "D:" isn't at the beggining of the string;
string replaceDriveLetter(const string& strFileName)
{
#ifndef WIN32
    return strFileName;
#else
    string s (strFileName);
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
string getSepTerminatedDir(const string& strDirName)
{
    string strRes (strDirName);
    if (!endsWith(strDirName, getPathSepAsStr()))
    {
        strRes += getPathSep();
    }
    checkDirName(strRes.substr(0, strRes.size() - 1));
    return strRes;
}


// removes the path separator at the end if present; throws IncorrectDirName on invalid file names
string getNonSepTerminatedDir(const string& strDirName)
{
    string strRes (strDirName);
    if (endsWith(strDirName, getPathSepAsStr()))
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

/*    int n (rename(strOldName.c_str(), strNewName.c_str()));
    int nErr (errno);
    if (0 != n && EXDEV != nErr) { qDebug("I/O Error %d: %s", strerror(nErr)); }

    if (0 != n && EXDEV == nErr)*/

    if (!QDir().rename(convStr(strOldName), convStr(strNewName)))
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

    //CB_CHECK1 (0 == n, CannotRenameFile());
}


// creates a copy a file;
// doesn't preserve any attributes; // ttt2 add option
// throws WriteError or EndOfFile from Helpers //ttt2 switch to: throws FoundDir, AlreadyExists, NameNotFound, CannotCopyFile, ?IncorrectDirName,
void CB_LIB_CALL copyFile(const std::string& strSourceName, const std::string& strDestName /*, OverwriteOption eOverwriteOption*/)
{
    ifstream_utf8 in (strSourceName.c_str(), ios::binary);
    ofstream_utf8 out (strDestName.c_str(), ios::binary);
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

    ifstream_utf8 in (strSourceName.c_str(), ios::binary);
    ofstream_utf8 out (strDestName.c_str(), ios::binary);
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
void CB_LIB_CALL deleteFile(const std::string& strFileName)
{
    CB_CHECK1 (!dirExists(strFileName), FoundDir());
    if (!fileExists(strFileName)) { return; }
    QFileInfo fi (convStr(strFileName));

    CB_CHECK1 (fi.dir().remove(fi.fileName()), CannotDeleteFile());
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

    string::size_type n (strName.rfind(getPathSep()));
    //CB_ASSERT(string::npos != n);
    if (string::npos == n) { return ""; }
    return strName.substr(0, n);
}


bool isInsideDir(const std::string& strName, const std::string& strDirName) // if strName is in strDirName or in one of its subdirectories; may throw IncorrectDirName for either param
{
    checkDirName(strName);
    checkDirName(strDirName);
    return beginsWith(strName, strDirName) && getPathSep() == strName[strDirName.size()];
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














