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
#include  <QTemporaryFile>

#ifndef _MSC_VER
    #include  <utime.h>
#else
    #include  <sys/utime.h>
#endif

#ifndef WIN32
#else
    #include  <windows.h>
#endif

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

    m_pImpl->m_vFileInfos = QDir(convStr(m_strDir)).entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs);
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
            continue; //ttt2 see if seeking the next is the best way to deal with this error
        }

        return true;
    }

    close();
    return false;
}


bool CB_LIB_CALL FileSearcher::findNext()
{
    CB_CHECK (*this, InvalidOperation/*"findNext() may only be called on an open searcher"*/);
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
    CB_CHECK (*this, InvalidOperation/*"getName() may only be called on an open searcher"*/);
    return convStr(m_pImpl->m_vFileInfos[m_pImpl->m_nCrtEntry].absoluteFilePath());
}


long long CB_LIB_CALL FileSearcher::getSize() const
{
    CB_CHECK (*this, InvalidOperation/*"getSize() may only be called on an open searcher"*/);
    return m_pImpl->m_vFileInfos[m_pImpl->m_nCrtEntry].size();
}


long long CB_LIB_CALL FileSearcher::getChangeTime() const
{
    CB_CHECK (*this, InvalidOperation/*"getChangeTime() may only be called on an open searcher"*/);
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
        CB_THROW(NameNotFound);
        //throw CannotGetData(strFileName, getOsError(), LI);
    }

    nChangeTime = fi.lastModified().toTime_t(); //ttt3 32bit
    nSize = fi.size();
}


#ifndef WIN32
#else
static wstring wstrFromUtf8(const string& s)
{
    vector<wchar_t> w (s.size() + 1);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], w.size());
    //inspect(&w[0], w.size()*2);
    return &w[0];
}
#endif

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
        CB_THROW(CbRuntimeError); //ttt2
    }
#else
    _utimbuf t;
    t.actime = (time_t)nChangeTime;
    t.modtime = (time_t)nChangeTime;
    if (0 != _wutime(wstrFromUtf8(strFileName).c_str(), &t))
    {
        //throw CannotSetDates(strFileName, getOsError(), LI);
        CB_THROW(CbRuntimeError); //ttt2
    }
#endif
}


// throws IncorrectDirName if the name ends with a path separator
void CB_LIB_CALL checkDirName(const string& strDirName)
{
    CB_CHECK (!endsWith(strDirName, getPathSepAsStr()), IncorrectDirName);
    //ttt2 more checks
}



// returns true if there is a file with that name;
// returns false if the name doesn't exist or it's a directory; doesn't throw
bool CB_LIB_CALL fileExists(const std::string& strFileName)
{
    QFileInfo fi (convStr(strFileName));
    return fi.isFile(); // ttt2 not sure if this should allow symlinks
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
    return fi.isDir(); // ttt2 not sure if this should allow symlinks
}


void CB_LIB_CALL createDir(const string& strDirName)
{
    if (strDirName.empty()) { return; } // the root dir always exists

    checkDirName(strDirName);

    if (dirExists(strDirName))
    {
        return;
    }

    string::size_type n (strDirName.rfind(getPathSep()));
    CB_CHECK1 (string::npos != n, CannotCreateDir, strDirName);
    string strParent (strDirName.substr(0, n));
    createDir(strParent);

    QFileInfo fi (convStr(strDirName));
    CB_CHECK1 (fi.dir().mkdir(fi.fileName()), CannotCreateDir, strDirName);

    CB_CHECK1 (dirExists(strDirName), CannotCreateDir, strDirName);
}


void CB_LIB_CALL createDirForFile(const std::string& strFileName)
{
    string::size_type n (strFileName.rfind(getPathSep()));
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
    CB_CHECK (!dirExists(strNewName), FoundDir);
    CB_CHECK (!dirExists(strOldName), FoundDir); // ttt2 separate OldFoundDir / NewFoundDir
    CB_CHECK (!fileExists(strNewName), AlreadyExists);
    CB_CHECK (fileExists(strOldName), NameNotFound);
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
            CB_THROW(CannotRenameFile);
        }
        catch (...) //ttt2 not quite right //ttt2 perhaps also NameNotFound, AlreadyExists, ...
        {
            CB_THROW(CannotRenameFile);
        }

        deleteFile(strOldName);
        return;
    }

    //CB_OLD_CHECK1 (0 == n, CannotRenameFile());
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
    CB_CHECK (out, WriteError);
    streampos nOutSize (out.tellp());
    CB_CHECK (nOutSize == nSize, WriteError);
}


void CB_LIB_CALL copyFile2(const std::string& strSourceName, const std::string& strDestName /*, OverwriteOption eOverwriteOption*/)
{
    CB_CHECK (!dirExists(strDestName), FoundDir);
    CB_CHECK (!dirExists(strSourceName), FoundDir); // ttt2 separate OldFoundDir / NewFoundDir
    CB_CHECK (!fileExists(strDestName), AlreadyExists);
    CB_CHECK (fileExists(strSourceName), NameNotFound);
    createDirForFile(strDestName); //ttt3 undo on error

    ifstream_utf8 in (strSourceName.c_str(), ios::binary);
    ofstream_utf8 out (strDestName.c_str(), ios::binary);
    CB_CHECK (in, CannotCopyFile);
    CB_CHECK (out, CannotCopyFile);
    streampos nSize (getSize(in));

    try
    {
        appendFilePart(in, out, 0, nSize);
    }
    catch (const WriteError&)
    {
        CB_THROW(CannotCopyFile);
    }

    streampos nOutSize (out.tellp());
    if (!out || nOutSize != nSize)
    {
        out.close();
        deleteFile(strDestName);
        CB_THROW(CannotCopyFile);
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
    CB_CHECK (!dirExists(strFileName), FoundDir);
    if (!fileExists(strFileName)) { return; }
    QFileInfo fi (convStr(strFileName));

    CB_CHECK (fi.dir().remove(fi.fileName()), CannotDeleteFile);
}


// just a name that doesn't exist; the file won't be deleted automatically; normally the name is obtained by appending something to strMasterFileName, but a more generic temp is used if the name is too long on wnd
string getTempFile(const std::string& strMasterFileName)
{
    QTemporaryFile tmp (convStr(strMasterFileName));

    QString qs;
    if (tmp.open()
#ifndef WIN32
#else
        && convStr(tmp.fileName()).size() < MAX_PATH //ttt3 not sure if MAX_PATH includes the terminator or not, so assume it does
#endif
        )
    {
        qs = tmp.fileName();
    }
    else
    {
        QTemporaryFile tmp1;
        CB_ASSERT (tmp1.open()); //ttt2 if it gets to this on W7 (and perhaps others) the file attributes are wrong, probably allowing only the current user to see it; ttt2 perhaps only use the dir, instead of the full file name in such case; //ttt2 doc
        qs = tmp1.fileName();
    }

    string s (convStr(qs));
    //qDebug("patt: '%s', tmp: '%s'", strMasterFileName.c_str(), s.c_str());
    return s;
    //ttt2 make sure it works OK if strMasterFileName is empty
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


// erases files matching strRoot followed by anything; returns the name of the first file that couldn't be erased
string eraseFiles(const string& strRoot)
{
    string::size_type n (strRoot.rfind("/"));
    string strDir (strRoot.substr(0, n));
    string strFile (strRoot.substr(n + 1));
    QStringList filter;
    filter << convStr(strFile + "*");
    QDir dir (convStr(strDir));
    QFileInfoList vFileInfos (dir.entryInfoList(filter, QDir::Files));
    for (int i = 0; i < vFileInfos.size(); ++i)
    {
        cout << convStr(vFileInfos[i].absoluteFilePath()) << "   " << convStr(vFileInfos[i].fileName()) << endl;
        if (!dir.remove(vFileInfos[i].fileName()))
        {
            return convStr(vFileInfos[i].absoluteFilePath());
        }
    }

    return "";
}


//ttt2 perhaps use strerror_r() to print file errors


//} //namespace ciobi_utils














