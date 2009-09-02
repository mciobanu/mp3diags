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

//#include  <deque>
#include  <set>
#include  <algorithm>

#include  <QDir>

#include  "FileEnum.h"

#include  "OsFile.h"

using namespace std;


// All directory names in DirTreeEnumeratorImpl end with the path separator
struct DirTreeEnumerator::DirTreeEnumeratorImpl
{
    DirTreeEnumeratorImpl(const vector<string>& vstrIncludeDirs, const vector<string>& vstrExcludeDirs);

    void reset();
    void reset(const vector<string>& vstrIncludeDirs, const vector<string>& vstrExcludeDirs);

    set<string> m_sstrIncludeDirs, m_sstrExcludeDirs;

    set<string> m_sstrProcDirs; // selected dirs (or their descendants) that have no ancestors in m_sstrProcDirs (for the first step they should have no ancestors in m_sstrIncludeDirs)
    set<string> m_sstrWaitingDirs; // selected dirs (or their descendants) that have ancestors in m_sstrProcDirs;

    //void getClosestAncestor(string strDir, string& strAncestor, bool& bIsChecked);
    enum ClosestAncestorState { NO_ANCESTOR, INCLUDED, EXCLUDED };
    ClosestAncestorState getClosestAncestorState(const string& strDir) const;

    bool hasAncestorInProcDirs(const string& strDir) const; // "strict" ancestor, doesn't matter if the argument itself is in ProcDirs
    void addDirs(set<string>& s, const vector<string>& v)
    {
        for (int i = 0, n = cSize(v); i < n; ++i)
        {
            string str (v[i]);
            CB_ASSERT (string::npos == str.find("//")); // put to see if handling of "//" is needed; // ttt1 see also QFileInfo::absoluteFilePath(), for Windows meaning
            if (!endsWith(str, "/")) { str += "/"; }
            s.insert(str);
        }
    }

    vector<string> m_vstrCrtDirList; // contains files only
    string m_strCrtDir; // used because QFileInfo::absoluteFilePath() has a performance warning; not sure if it helps; see also QFileInfo::canonicalFilePath
    int m_nCrtDirNdx; // index in m_vstrCrtDirList

    string next(); // returns an empty string if there's nothing next

    bool isIncluded(const string& strFile);
};

DirTreeEnumerator::~DirTreeEnumerator()
{
    delete m_pImpl;
}


DirTreeEnumerator::DirTreeEnumerator(const vector<string>& vstrIncludeDirs, const vector<string>& vstrExcludeDirs) : m_pImpl(new DirTreeEnumeratorImpl(vstrIncludeDirs, vstrExcludeDirs)), m_noDefaults(0)
{
}

DirTreeEnumerator::DirTreeEnumerator() : m_pImpl(new DirTreeEnumeratorImpl(vector<string>(), vector<string>())), m_noDefaults(0)
{
}

void DirTreeEnumerator::reset(const vector<string>& vstrIncludeDirs, const vector<string>& vstrExcludeDirs)
{
    m_pImpl->reset(vstrIncludeDirs, vstrExcludeDirs);
}

void DirTreeEnumerator::reset()
{
    m_pImpl->reset();
}

string DirTreeEnumerator::next() // returns an empty string if there's nothing next
{
    return m_pImpl->next();
}

bool DirTreeEnumerator::isIncluded(const std::string& strFile)
{
    return m_pImpl->isIncluded(strFile);
}




DirTreeEnumerator::DirTreeEnumeratorImpl::DirTreeEnumeratorImpl(const vector<string>& vstrIncludeDirs, const vector<string>& vstrExcludeDirs)
{
    reset(vstrIncludeDirs, vstrExcludeDirs);
}

void DirTreeEnumerator::DirTreeEnumeratorImpl::reset(const vector<string>& vstrIncludeDirs, const vector<string>& vstrExcludeDirs)
{
    m_sstrIncludeDirs.clear();
    m_sstrExcludeDirs.clear();

    addDirs(m_sstrIncludeDirs, vstrIncludeDirs);
    addDirs(m_sstrExcludeDirs, vstrExcludeDirs);

    vector<string> v;
    set_intersection(m_sstrIncludeDirs.begin(), m_sstrIncludeDirs.end(), m_sstrExcludeDirs.begin(), m_sstrExcludeDirs.end(), back_inserter(v));
    CB_CHECK1 (vstrIncludeDirs.size() + vstrExcludeDirs.size() == m_sstrIncludeDirs.size() + m_sstrExcludeDirs.size() && v.empty(), DirTreeEnumerator::InvalidDirs());

    for (set<string>::iterator it = m_sstrIncludeDirs.begin(), end = m_sstrIncludeDirs.end(); it != end; ++it)
    {
        ClosestAncestorState e (getClosestAncestorState(*it));
        CB_CHECK1 (NO_ANCESTOR == e || EXCLUDED == e, DirTreeEnumerator::InvalidDirs());
    }//ttt0 ??? some means to log only uncaught exceptions //ttt0 see what to do with CB_CHECK1: on Wnd it logs errors to a file but the user isn't told about them; have a check in main() and show error

    for (set<string>::iterator it = m_sstrExcludeDirs.begin(), end = m_sstrExcludeDirs.end(); it != end; ++it)
    {
        ClosestAncestorState e (getClosestAncestorState(*it));
        CB_CHECK1 (INCLUDED == e, DirTreeEnumerator::InvalidDirs());
    }

    reset();
}

void DirTreeEnumerator::DirTreeEnumeratorImpl::reset()
{
    m_nCrtDirNdx = -1;
}



DirTreeEnumerator::DirTreeEnumeratorImpl::ClosestAncestorState DirTreeEnumerator::DirTreeEnumeratorImpl::getClosestAncestorState(const string& strDir) const
{
    string s (strDir);
    //bIsChecked = false; strAncestor.clear();
    if (endsWith(s, "/")) { s.erase(s.size() - 1); } // it's messier to work with an s that ends with '/', because then s=="/" is a special case
    for (;;)
    {
        string::size_type n (s.rfind("/"));
        if (string::npos == n) { return NO_ANCESTOR; }
        s.erase(n + 1);
        bool bI (m_sstrIncludeDirs.count(s) > 0);
        bool bE (m_sstrExcludeDirs.count(s) > 0);
        CB_ASSERT (!(bE && bI));
        if (bI) { return INCLUDED; }
        if (bE) { return EXCLUDED; }
        s.erase(s.size() - 1);
    }
}


bool DirTreeEnumerator::DirTreeEnumeratorImpl::hasAncestorInProcDirs(const string& strDir) const // "strict" ancestor, doesn't matter if the argument itself is in ProcDirs
{
    string s (strDir);
    if (endsWith(s, "/")) { s.erase(s.size() - 1); } // it's messier to work with an s that ends with '/', because then s=="/" is a special case
    for (;;)
    {
        string::size_type n (s.rfind("/"));
        if (string::npos == n) { return false; }
        s.erase(n + 1);
        if (m_sstrProcDirs.count(s) > 0) { return true; }
        s.erase(s.size() - 1);
    }
}


bool DirTreeEnumerator::DirTreeEnumeratorImpl::isIncluded(const string& strFile)
{
    //return INCLUDED == getClosestAncestorState(getParent(strFile));
    return INCLUDED == getClosestAncestorState(strFile);
}


/*

Algorithm:
 - split m_sstrIncludeDirs in 2, putting those that have no ancestors in m_sstrProcDirs and the others in m_sstrWaitingDirs;
 - when next() is called:
    - go to the next file in the "current dir", putting all the found directories in m_sstrIncludeDirs (well, unless they are in m_sstrExcludeDirs);
    - if there are files left in the "current dir", return the next file;
    - if there are no files left in the "current dir", take the first entry from m_sstrIncludeDirs and turn it into the current dir, then move to m_sstrIncludeDirs all entries from m_sstrWaitingDirs that don't have an ancestor in m_sstrIncludeDirs
    - if m_sstrIncludeDirs is empty, just return "";

*/

string DirTreeEnumerator::DirTreeEnumeratorImpl::next() // returns an empty string if there's nothing next
{
    if (-1 == m_nCrtDirNdx)
    {
        m_nCrtDirNdx = 0;
        m_vstrCrtDirList.clear();

        set<string> sstrNewProc;
        m_sstrWaitingDirs.clear();
        m_sstrProcDirs = m_sstrIncludeDirs; // !!! needed for hasAncestorInProcDirs()

        for (set<string>::iterator it = m_sstrIncludeDirs.begin(), end = m_sstrIncludeDirs.end(); it != end; ++it)
        {
            if (!hasAncestorInProcDirs(*it))
            {
                sstrNewProc.insert(*it);
            }
            else
            {
                m_sstrWaitingDirs.insert(*it);
            }
        }

        m_sstrProcDirs.swap(sstrNewProc);
    }

    for (;;)
    {
        if (m_nCrtDirNdx < cSize(m_vstrCrtDirList))
        {
            return m_vstrCrtDirList[m_nCrtDirNdx++];
        }

        if (m_sstrProcDirs.empty())
        {
            if (m_sstrWaitingDirs.empty()) { return ""; }
            set<string> sstrNewProc, sstrNewWt;
            m_sstrProcDirs = m_sstrWaitingDirs; // !!! needed for hasAncestorInProcDirs()

            for (set<string>::iterator it = m_sstrWaitingDirs.begin(), end = m_sstrWaitingDirs.end(); it != end; ++it)
            {
                if (!hasAncestorInProcDirs(*it))
                {
                    sstrNewProc.insert(*it);
                }
                else
                {
                    sstrNewWt.insert(*it);
                }
            }

            sstrNewWt.swap(m_sstrWaitingDirs);
            sstrNewProc.swap(m_sstrProcDirs);
        }

        m_vstrCrtDirList.clear();
        m_nCrtDirNdx = 0;
        m_strCrtDir = *m_sstrProcDirs.begin();
        m_sstrProcDirs.erase(m_sstrProcDirs.begin());

        for (set<string>::iterator it = m_sstrWaitingDirs.begin(), end = m_sstrWaitingDirs.end(); it != end; ++it)
        {
            if (!hasAncestorInProcDirs(*it))
            {
                m_sstrWaitingDirs.insert(*it);
            }
        }

        QDir dir (convStr(m_strCrtDir));

        QFileInfoList lst (dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot));
        for (QFileInfoList::iterator it = lst.begin(), end = lst.end(); it != end; ++it)
        {
            const QFileInfo& inf (*it);
            string s (convStr(inf.fileName()));

            s = m_strCrtDir + s;
            // !!! note that inf.isSymLink() may return true in addition to isFile() or isDir() returning true, so no "special case" processing is needed unless we want to exclude symlinks
            if (inf.isFile())
            {
                m_vstrCrtDirList.push_back(s);
            }
            else if (inf.isDir())
            {
                if (!endsWith(s, "/")) { s += "/"; }
                if (0 == m_sstrExcludeDirs.count(s))
                {
                    m_sstrProcDirs.insert(s);
                }
            }
        }
    }
}

/*
struct AAAlp
{
    AAAlp()
    {
        vector<string> vIncl, vExcl;
        //vIncl.push_back("/r/temp/1/tmp2/a rep/a");
        vIncl.push_back("/r/temp/1/tmp2/a rep/");
        vExcl.push_back("/r/temp/1/tmp2/a rep/b");
        DirTreeEnumerator en (vIncl, vExcl);
        for (;;)
        {
            string s (en.next());
            if (s.empty()) { throw 1; }
            qDebug("%s", s.c_str());
        }
    }
};

AAAlp lprecerce;
*/


//========================================================================================================================
//========================================================================================================================
//========================================================================================================================


ListEnumerator::ListEnumerator(const string& strDir) : m_noDefaults(0), m_nCrt(0)
{
    QDir dir (convStr(strDir));
    string s1 (strDir);
    if (!endsWith(s1, "/"))
    {
        s1 += "/";
    }

    QFileInfoList lst (dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot));
    for (QFileInfoList::iterator it = lst.begin(), end = lst.end(); it != end; ++it)
    {
        const QFileInfo& inf (*it);
        string s (convStr(inf.fileName()));

        s = s1 + s;
        // !!! note that inf.isSymLink() may return true in addition to isFile() or isDir() returning true, so no "special case" processing is needed unless we want to exclude symlinks
        if (inf.isFile())
        {
            m_vstrFiles.push_back(s);
        }
    }
}


ListEnumerator::ListEnumerator(const std::vector<std::string>& vstrFiles) : m_noDefaults(0), m_vstrFiles(vstrFiles), m_nCrt(0)
{
}


// returns an empty string if there's nothing next
/*override*/ std::string ListEnumerator::next()
{
    if (m_nCrt >= cSize(m_vstrFiles)) { return ""; }

    return m_vstrFiles[m_nCrt++];
}




//========================================================================================================================
//========================================================================================================================
//========================================================================================================================


