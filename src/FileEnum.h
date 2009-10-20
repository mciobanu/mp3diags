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

#ifndef FileEnumH
#define FileEnumH


#include  <string>
#include  <vector>

#include  "Helpers.h"

struct FileEnumerator
{
    virtual ~FileEnumerator() {}
    virtual std::string next() = 0; // returns an empty string if there's nothing next
    virtual void reset() = 0;
};


/*
Returns a list with all the files in the "include directories", discarding the ones in the "exclude directories".

If the directories have redundant information, an InvalidDirs is thrown. Here's how they should be:
    - a directory may be "included" iff it has no ancestors OR its closest ancestor is "excluded";
    - a directory may be "excluded" iff it has ancestors AND its closest ancestor is "included";
    - duplicates are not allowed;

Assumes there are no "." or ".." in the paths, but doesn't check. // ttt2 perhaps check

What is really on the disk is not important, in the sense that names of directories that don't exist or of files are ignored.

Ending of directory names with the path separator is optional and it doesn't have to be consistent.

*/

class DirTreeEnumerator : public FileEnumerator
{
    struct DirTreeEnumeratorImpl;
    DirTreeEnumeratorImpl* m_pImpl;
    NoDefaults m_noDefaults;
public:
    DirTreeEnumerator(const std::vector<std::string>& vstrIncludeDirs, const std::vector<std::string>& vstrExcludeDirs);
    DirTreeEnumerator();
    /*override*/ ~DirTreeEnumerator();
    /*override*/ void reset();
    void reset(const std::vector<std::string>& vstrIncludeDirs, const std::vector<std::string>& vstrExcludeDirs);
    /*override*/ std::string next(); // returns an empty string if there's nothing next
    bool isIncluded(const std::string& strFile);

    struct InvalidDirs {};
};


#if 0
class DirEnumerator : public FileEnumerator
{
    struct DirEnumeratorImpl;
    DirEnumeratorImpl* m_pImpl;
    NoDefaults m_noDefaults;
    int m_nCrt;
public:
    DirEnumerator(const std::string& strDir);
    ~DirEnumerator();
    /*override*/ void reset() { m_nCrt = 0; }
    /*override*/ std::string next(); // returns an empty string if there's nothing next
};
#endif


class ListEnumerator : public FileEnumerator
{
    //struct ListEnumeratorImpl;
    //ListEnumeratorImpl* m_pImpl;
    NoDefaults m_noDefaults;
    std::vector<std::string> m_vstrFiles;
    int m_nCrt;
public:
    ListEnumerator(const std::vector<std::string>& vstrFiles);
    ListEnumerator(const std::string& strDir);
    /*override*/ ~ListEnumerator() {}
    /*override*/ void reset() { m_nCrt = 0; }
    /*override*/ std::string next(); // returns an empty string if there's nothing next
};


#endif // #ifndef FileEnumH
