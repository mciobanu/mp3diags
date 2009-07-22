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


#include  <sstream>
#include  <iomanip>

#include  <QDir>

#include  "Transformation.h"

#include  "Helpers.h"
#include  "OsFile.h"
#include  "Mp3Manip.h"


using namespace std;
using namespace pearl;


/*static const string& getOsTempDir() 
{
    static string s; // ttt3 these static variables are not really thread safe, but in this case it doesn't matter, because they all get called from a single thread (the UI thread)
    if (s.empty())
    {
        s = convStr(QDir::tempPath());
        if (endsWith(s, getPathSepAsStr()))
        {
            s.erase(s.size() - 1, 1);
        }
    }
    return s;
}
*/

//ttt1 review all defaults
static string getDefaultSrc() { return ""; } //ttt2 see if src dir needs to be revived

static string getDefaultUnprocOrig() { return convStr(getTempDir()) + "/mp3diags/unprocOrig"; }
static string getDefaultProcessed() { return convStr(getTempDir()) + "/mp3diags/proc"; }
static string getDefaultProcOrig() { return convStr(getTempDir() + "/mp3diags/procOrig"); }
static string getDefaultTemp() { return convStr(getTempDir() + "/mp3diags/temp"); }
static string getDefaultComp() { return convStr(getTempDir() + "/mp3diags/comp"); }




/*static TransfConfig::OptionsWrp getDefaultOptions()
{
    TransfConfig::OptionsWrp x;
    x.m_nVal = 0;

    x.m_opt.m_bTempCreate = true;

    x.m_opt.m_bCompCreate = true;

    x.m_opt.m_nProcessedCreate = 1; // create and always rename, since it is created in the same dir as the source
    x.m_opt.m_bProcessedUseLabel = true;
    x.m_opt.m_bProcessedAlwayUseCounter = false;
    x.m_opt.m_bProcessedUseSeparateDir = false;

    x.m_opt.m_nUnprocOrigChange = 0;
    x.m_opt.m_bUnprocOrigUseLabel = false;
    x.m_opt.m_bUnprocOrigAlwayUseCounter = false;

    x.m_opt.m_nProcOrigChange = 0;
    x.m_opt.m_bProcOrigUseLabel = false;
    x.m_opt.m_bProcOrigAlwayUseCounter = false;

    return x;
}*/



static TransfConfig::OptionsWrp getDefaultOptions()
{
    TransfConfig::OptionsWrp x;
    x.m_nVal = 0;

    x.m_opt.m_bTempCreate = false;

    x.m_opt.m_bCompCreate = false;

    x.m_opt.m_nProcOrigChange = 5; // move w/o renaming, if doesn't exist; discard if it exists;
    x.m_opt.m_bProcOrigUseLabel = false;
    x.m_opt.m_bProcOrigAlwayUseCounter = false;

    x.m_opt.m_nUnprocOrigChange = 0; // don't change
    x.m_opt.m_bUnprocOrigUseLabel = false;
    x.m_opt.m_bUnprocOrigAlwayUseCounter = false;

    x.m_opt.m_nProcessedCreate = 2; // create and always rename, since it is created in the same dir as the source
    x.m_opt.m_bProcessedUseLabel = true;
    x.m_opt.m_bProcessedAlwayUseCounter = true;
    x.m_opt.m_bProcessedUseSeparateDir = false;

    x.m_opt.m_bKeepOrigTime = false;

    return x;
}



TransfConfig::TransfConfig(
        const std::string& strSrcDir,
        const std::string& strProcOrigDir,
        const std::string& strUnprocOrigDir,
        const std::string& strProcessedDir,
        const std::string& strTempDir,
        const std::string& strCompDir,
        int nOptions // may be -1 to indicate "default" values
        )
{
    m_strSrcDir = "*" == strSrcDir ? getDefaultSrc() : strSrcDir;
    m_strProcOrigDir = "*" == strProcOrigDir ? getDefaultProcOrig() : strProcOrigDir;
    m_strUnprocOrigDir = "*" == strUnprocOrigDir ? getDefaultUnprocOrig() : strUnprocOrigDir;
    m_strProcessedDir = "*" == strProcessedDir ? getDefaultProcessed() : strProcessedDir;
    m_strTempDir = "*" == strTempDir ? getDefaultTemp() : strTempDir;
    m_strCompDir = "*" == strCompDir ? getDefaultComp() : strCompDir;

    m_optionsWrp.m_nVal = -1 == nOptions ? getDefaultOptions().m_nVal : nOptions;

    checkDirName(m_strSrcDir);
    checkDirName(m_strProcOrigDir);  // some of these dirs are allowed to be empty, just like src when comes from the config dialog, empty meaning "/", so that should work for all, probably
    checkDirName(m_strUnprocOrigDir);
    checkDirName(m_strProcessedDir);
    checkDirName(m_strTempDir);
    checkDirName(m_strCompDir);
}


TransfConfig::TransfConfig()
{
    m_optionsWrp.m_nVal = getDefaultOptions().m_nVal;
}




// last '/' goes to dir; last '.' goes to ext (if extension present)
void TransfConfig::splitOrigName(const string& strOrigSrcName, std::string& strRelDir, std::string& strBaseName, std::string& strExt) const
{
    CB_CHECK1 (beginsWith(strOrigSrcName, m_strSrcDir), IncorrectPath());

    strRelDir = strOrigSrcName.substr(m_strSrcDir.size());
#ifndef WIN32
    CB_CHECK1 (beginsWith(strRelDir, getPathSepAsStr()), IncorrectPath());
#else
    //CB_CHECK1 (beginsWith(strRelDir, getPathSepAsStr()), IncorrectPath()); //ttt1 see if it's OK to skip this test on Windows; it probably is, because it's more of an assert and the fact that it's not triggered on Linux should be enough
#endif


    string::size_type nLastSep (strRelDir.rfind(getPathSep()));
    string::size_type nLastPer (strRelDir.rfind('.'));

    if (nLastPer > nLastSep)
    { // the file has an extension
        strExt = strRelDir.substr(nLastPer);
        strRelDir.erase(nLastPer, string::npos);
    }

    strBaseName = strRelDir.substr(nLastSep + 1);
    strRelDir.erase(nLastSep + 1, string::npos);
}

/*
static int s_anPositions[] = { 0, 1, 2, 4, 5, 6, 7, 10, 11, 12, 14, 15, 16 };


int TransfConfig::getOptionValue(Option eOption) const
{
    return getOpt(s_anPositions[eOption], s_anPositions[eOption + 1] - s_anPositions[eOption]);
}*/


// adds a counter and/or a label, such that a file with the resulting name doesn't exist;
/*static*/ string TransfConfig::addLabelAndCounter(const string& s1, const string& s2, const string& strLabel, bool bAlwaysUseCounter, bool bAlwaysRename)
{
    string strRes;
    if (!bAlwaysRename)
    {
        strRes = replaceDriveLetter(s1 + s2);
        if (!fileExists(strRes))
        {
            createDirForFile(strRes);
            return strRes;
        }
    }

    string strLabel2 (strLabel);
    if (!strLabel2.empty()) { strLabel2.insert(0, "."); }

    if (!bAlwaysUseCounter && !strLabel2.empty())
    {
        strRes = replaceDriveLetter(s1 + strLabel2 + s2);
        if (!fileExists(strRes))
        {
            createDirForFile(strRes);
            return strRes;
        }
    }

    for (int i = 1; ; ++i)
    {
        {
            ostringstream out;
            out << "." << setfill('0') << setw(3) << i;
            strRes = replaceDriveLetter(s1 + strLabel2 + out.str() + s2);
        }

        if (!fileExists(strRes))
        {
            createDirForFile(strRes);
            return strRes;
        }
    }
}

// normally makes sure that the name returned doesn't exist, by applying any renaming specified in the params; there's an exception, though: is bAllowDup is true, duplicates are allowed
string TransfConfig::getRenamedName(const std::string& strOrigSrcName, const std::string& strNewRootDir, const std::string& strLabel, bool bAlwayUseCounter, bool bAlwaysRename, bool bAllowDup /*= false*/) const // bAllowDup is needed only for the option 5 of m_nProcOrigChange, when ORIG_MOVE_OR_ERASE gets returned
{
    /* steps
        1. decide if it should rename, based on bAlwaysRename (merely changing the folder isn't considered renaming)
        2. if it should rename:
            2.1 add the label (if it's empty, there's nothing to add)
            2.2 if bAlwayUseCounter is false and there's a label, try first with that label
                2.2.1 if the name doesn't exist, return
            2.3 increment the counter until a new name is found
        (note that addCounter() does all that's needed for 2
    */
    string strRelDir;
    string strBaseName;
    string strExt;
    splitOrigName(strOrigSrcName, strRelDir, strBaseName, strExt);

    string strRes;

    if (!bAlwaysRename)
    { // aside from the new folder, no renaming takes places if this new name isn't in use
        strRes = replaceDriveLetter(strNewRootDir + strRelDir + strBaseName + strExt);
        if (!fileExists(strRes) || bAllowDup)
        {
            createDirForFile(strRes);
            return strRes;
        }
    }

    /*if (!bAlwayUseCounter && !strLabel.empty())
    { // now try to use the label but not the counter
        strRes = strNewRootDir + strRelDir + strBaseName + strExt;
        if (!fileExists(strRes))
        {
            createDirForFile(strRes);
            return strRes;
        }
    }*/

    //out << strNewRootDir << strRelDir << strBaseName << (strLabel.empty() ? "" : ".") << strLabel;
    string s1 (strNewRootDir + strRelDir + strBaseName);
    //setfill('0') << setw(3) << i << strExt;

    return addLabelAndCounter(s1, strExt, strLabel, bAlwayUseCounter, bAlwaysRename);
}


TransfConfig::OrigFile TransfConfig::getChangedOrigNameHlp(const string& strOrigSrcName, const string& strDestDir, int nChange, bool bUseLabel, bool bAlwayUseCounter, string& strNewName) const
{
    switch (nChange)
    {
    case 0: // don't change unprocessed orig file
        strNewName = strOrigSrcName; return ORIG_DONT_CHANGE;

    case 1: // erase unprocessed orig file
        strNewName = strOrigSrcName; return ORIG_ERASE;

    case 2: // move unprocessed orig file to strDestDir; always rename
        strNewName = getRenamedName(strOrigSrcName, strDestDir, bUseLabel ? "orig" : "", bAlwayUseCounter, ALWAYS_RENAME);
        return ORIG_MOVE;

    case 3: // move unprocessed orig file to strDestDir; rename only if name is in use
        strNewName = getRenamedName(strOrigSrcName, strDestDir, bUseLabel ? "orig" : "", bAlwayUseCounter, RENAME_IF_NEEDED);
        return ORIG_MOVE;

    case 4: // rename in the same dir
        strNewName = getRenamedName(strOrigSrcName, m_strSrcDir, bUseLabel ? "orig" : "", bAlwayUseCounter, ALWAYS_RENAME);
        return ORIG_MOVE;

    case 5: // move unprocessed orig file to strDestDir if it's not already there; erase if it exists; don't
        strNewName = getRenamedName(strOrigSrcName, strDestDir, "", USE_COUNTER_IF_NEEDED, RENAME_IF_NEEDED, ALLOW_DUPLICATES);
        return ORIG_MOVE_OR_ERASE;

    default:
        CB_ASSERT(false);
    }
}


TransfConfig::OrigFile TransfConfig::getProcOrigName(string strOrigSrcName, string& strNewName) const
{
    removeSuffix(strOrigSrcName);
    return getChangedOrigNameHlp(strOrigSrcName, m_strProcOrigDir, m_optionsWrp.m_opt.m_nProcOrigChange, m_optionsWrp.m_opt.m_bProcOrigUseLabel, m_optionsWrp.m_opt.m_bProcOrigAlwayUseCounter, strNewName);
}


TransfConfig::OrigFile TransfConfig::getUnprocOrigName(string strOrigSrcName, string& strNewName) const
{
    removeSuffix(strOrigSrcName);
    return getChangedOrigNameHlp(strOrigSrcName, m_strUnprocOrigDir, m_optionsWrp.m_opt.m_nUnprocOrigChange, m_optionsWrp.m_opt.m_bUnprocOrigUseLabel, m_optionsWrp.m_opt.m_bUnprocOrigAlwayUseCounter, strNewName);
}


void TransfConfig::removeSuffix(string& s) const
{
    /*
    // !!! this seemed too aggressive;
    for (;;)
    {
        string::size_type nPos (s.rfind(".orig."));
        if (string::npos == nPos) { break; }
        s.erase(nPos, 5);
    }

    for (;;)
    {
        string::size_type nPos (s.rfind(".proc."));
        if (string::npos == nPos) { break; }
        s.erase(nPos, 5);
    }

    for (;;)
    {
        int n (cSize(s));
        if (!endsWith(s, ".mp3") || n <= 4 + 4) { break; }
        if ('.' != s[n - 8] || !isdigit(s[n - 7]) || !isdigit(s[n - 6]) || !isdigit(s[n - 5])) { break; }
        s.erase(n - 8, 4);
    }
    */

    string::size_type n (s.size());

    string::size_type nPos (s.rfind(".orig."));
    if (string::npos != nPos && nPos > 0 && nPos == n - 9)
    {
        s.erase(n - 9, 5);
        return;
    }

    if (string::npos != nPos && nPos > 0 && nPos == n - 13 && isdigit(s[n - 7]) && isdigit(s[n - 6]) && isdigit(s[n - 5]) && '.' == s[n - 4])
    {
        s.erase(n - 13, 9);
        return;
    }

    nPos = s.rfind(".proc.");
    if (string::npos != nPos && nPos > 0 && nPos == n - 9)
    {
        s.erase(n - 9, 5);
        return;
    }

    if (string::npos != nPos && nPos > 0 && nPos == n - 13 && isdigit(s[n - 7]) && isdigit(s[n - 6]) && isdigit(s[n - 5]) && '.' == s[n - 4])
    {
        s.erase(n - 13, 9);
        return;
    }

    //ttt2 this needs to be revisited if files with extensions other than ".mp3" are accepted
}



void TransfConfig::testRemoveSuffix() const
{
    string s;
    const char* p;

    p = "abc.orig.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".orig.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".orig.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "ww trty.orig.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "abc.orig.003.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".orig.003.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".orig.003.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "ww trty.orig.003.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "abc.orig.03.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".orig.03.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".orig.03.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "ww trty.orig.03.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    qDebug("-------------------------------------");

    p = "abc.proc.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".proc.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".proc.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "ww trty.proc.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "abc.proc.003.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".proc.003.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".proc.003.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "ww trty.proc.003.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "abc.proc.03.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".proc.03.mp3"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = ".proc.03.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    p = "ww trty.proc.03.mp"; s = p; removeSuffix(s); qDebug("pref: %s   =>   %s", p, s.c_str());
    qDebug("-------------------------------------");
}



TransfConfig::TransfFile TransfConfig::getProcessedName(string strOrigSrcName, std::string& strName) const
{
    removeSuffix(strOrigSrcName);
    switch (m_optionsWrp.m_opt.m_nProcessedCreate)
    {
    case 0: // don't create proc files
        strName.clear(); return TRANSF_DONT_CREATE;

    case 1: // create proc files and always rename
        strName = getRenamedName(strOrigSrcName, m_optionsWrp.m_opt.m_bProcessedUseSeparateDir ? m_strProcessedDir : m_strSrcDir, m_optionsWrp.m_opt.m_bProcessedUseLabel ? "proc" : "", m_optionsWrp.m_opt.m_bProcessedAlwayUseCounter, ALWAYS_RENAME);
        return TRANSF_CREATE;

    case 2: // create proc files and rename if the name is in use
        strName = getRenamedName(strOrigSrcName, m_optionsWrp.m_opt.m_bProcessedUseSeparateDir ? m_strProcessedDir : m_strSrcDir, m_optionsWrp.m_opt.m_bProcessedUseLabel ? "proc" : "", m_optionsWrp.m_opt.m_bProcessedAlwayUseCounter, RENAME_IF_NEEDED);
        return TRANSF_CREATE;

    default:
        CB_ASSERT(false);
    }
}


TransfConfig::TransfFile TransfConfig::getTempName(string strOrigSrcName, const std::string& strOpName, std::string& strName) const
{
    removeSuffix(strOrigSrcName);
    if (!m_optionsWrp.m_opt.m_bTempCreate)
    {
        strName = getTempFile();
        return TRANSF_DONT_CREATE;
    }

    string strRelDir;
    string strBaseName;
    string strExt;
    splitOrigName(strOrigSrcName, strRelDir, strBaseName, strExt);

    //string strRes;

    {
        ostringstream out;
        out << m_strTempDir << strRelDir << strBaseName << ".temp " << strOpName;
        strName = addLabelAndCounter(out.str(), strExt, "", ALWAYS_USE_COUNTER, ALWAYS_RENAME);
        return TRANSF_CREATE;
    }
}


TransfConfig::TransfFile TransfConfig::getCompNames(string strOrigSrcName, const std::string& strOpName, std::string& strBefore, std::string& strAfter) const
{
    removeSuffix(strOrigSrcName);

    if (!m_optionsWrp.m_opt.m_bCompCreate)
    {
        strBefore = getTempFile();
        strAfter = getTempFile();
        return TRANSF_DONT_CREATE;
    }

    string strRelDir;
    string strBaseName;
    string strExt;
    splitOrigName(strOrigSrcName, strRelDir, strBaseName, strExt);

    /*{
        ostringstream out;
        //out << m_strCompDir << strRelDir << strBaseName << ".1(before " << strOpName << ")" << strExt;
        strBefore = out.str();
    }

    {
        ostringstream out;
        out << m_strCompDir << strRelDir << strBaseName << ".2(after " << strOpName << ")" << strExt;
        strAfter = out.str();
    }

    if (!fileExists(strBefore) && !fileExists(strAfter))
    {
        createDirForFile(strAfter);
        return TRANSF_CREATE;
    }*/


    /*for (int i = 1; ; ++i)
    {
        {
            ostringstream out;
            //out << m_strCompDir << strRelDir << strBaseName << ".1(before " << strOpName << ")." << setfill('0') << setw(3) << i << strExt;
            out << m_strCompDir << strRelDir << strBaseName << "." << setfill('0') << setw(3) << i << "a (before " << strOpName << ")" << strExt;
            strBefore = out.str();
        }

        {
            ostringstream out;
            //out << m_strCompDir << strRelDir << strBaseName << ".2(after " << strOpName << ")." << setfill('0') << setw(3) << i << strExt;
            out << m_strCompDir << strRelDir << strBaseName << "." << setfill('0') << setw(3) << i << "b (after " << strOpName << ")" << strExt;
            strAfter = out.str();
        }

        if (!fileExists(strBefore) && !fileExists(strAfter))
        {
            createDirForFile(strAfter);
            return TRANSF_CREATE;
        }
    }*/

    int i (1);
    //File Searcher fs;
    for (;; ++i)
    {
        ostringstream out;
        string strDir (m_strCompDir + strRelDir);
        out << strBaseName << "." << setfill('0') << setw(3) << i << "*";
        string strFile (out.str());
        QDir d (convStr(strDir));
        QStringList nameFlt;
        nameFlt << convStr(strFile);
        if (d.entryList(nameFlt).isEmpty())
        {
            break;
        }
    }

    {
        ostringstream out;
        //out << m_strCompDir << strRelDir << strBaseName << ".1(before " << strOpName << ")." << setfill('0') << setw(3) << i << strExt;
        out << m_strCompDir << strRelDir << strBaseName << "." << setfill('0') << setw(3) << i << "a (before " << strOpName << ")" << strExt;
        strBefore = out.str();
    }

    {
        ostringstream out;
        //out << m_strCompDir << strRelDir << strBaseName << ".2(after " << strOpName << ")." << setfill('0') << setw(3) << i << strExt;
        out << m_strCompDir << strRelDir << strBaseName << "." << setfill('0') << setw(3) << i << "b (after " << strOpName << ")" << strExt;
        strAfter = replaceDriveLetter(out.str());
    }

    createDirForFile(strAfter);
    return TRANSF_CREATE;
}




TransfConfig::OrigFile TransfConfig::getProcOrigAction() const
{
    switch (m_optionsWrp.m_opt.m_nProcOrigChange)
    {
    case 0: // don't change unprocessed orig file
        return ORIG_DONT_CHANGE;

    case 1: // erase unprocessed orig file
        return ORIG_ERASE;

    case 2: // move unprocessed orig file to strDestDir; always rename
    case 3: // move unprocessed orig file to strDestDir; rename only if name is in use
    case 4: // rename in the same dir
        return ORIG_MOVE;

    case 5: // move if dest doesn't exist; erase if it does;
        return ORIG_MOVE_OR_ERASE;

    default:
        CB_ASSERT(false);
    }
}

TransfConfig::OrigFile TransfConfig::getUnprocOrigAction() const
{
    switch (m_optionsWrp.m_opt.m_nUnprocOrigChange)
    {
    case 0: // don't change unprocessed orig file
        return ORIG_DONT_CHANGE;

    case 1: // erase unprocessed orig file
        return ORIG_ERASE;

    case 2: // move unprocessed orig file to strDestDir; always rename
    case 3: // move unprocessed orig file to strDestDir; rename only if name is in use
    case 4: // rename in the same dir
        return ORIG_MOVE;

    default:
        CB_ASSERT(false);
    }
}


TransfConfig::TransfFile TransfConfig::getProcessedAction() const
{
    switch (m_optionsWrp.m_opt.m_nProcessedCreate)
    {
    case 0: // don't create proc files
        return TRANSF_DONT_CREATE;

    case 1: // create proc files and always rename
    case 2: // create proc files and rename if the name is in use
        return TRANSF_CREATE;

    default:
        CB_ASSERT(false);
    }
}

TransfConfig::TransfFile TransfConfig::getTempAction() const
{
    return m_optionsWrp.m_opt.m_bTempCreate ? TRANSF_CREATE : TRANSF_DONT_CREATE;
}

TransfConfig::TransfFile TransfConfig::getCompAction() const
{
    return m_optionsWrp.m_opt.m_bCompCreate ? TRANSF_CREATE : TRANSF_DONT_CREATE;
}




#if 0
// to what an original file should be renamed, if it was "processed";
// if m_strSrcRootDir and m_strProcRootDir are different, it returns ""; (the original file doesn't get touched);
// if m_strSrcRootDir and m_strProcRootDir are equal, it returns a name based on the original, with an ".orig.<NNN>" inserted before the extension, where <NNN> is a number chosen such that a file with this new name doesn't exist;
string TransfConfig::getBackupName(const std::string& strOrigSrcName) const
{
    if (m_strSrcRootDir != m_strProcRootDir) { return ""; }

    string strRelName;
    string strExt;
    splitOrigName(strOrigSrcName, strRelName, strExt);

    string strRes;
    for (int i = 1; ; ++i)
    {
        {
            ostringstream out;
            out << m_strSrcRootDir << strRelName << ".orig." << setfill('0') << setw(3) << i << strExt;
            strRes = out.str();
        }

        if (!fileExists(strRes))
        {
            createDirForFile(strRes);
            return strRes;
        }
    }
}



// to what the last "processed" file should be renamed;
// if m_strSrcRootDir and m_strProcRootDir are different, it returns strOrigSrcName, moved to m_strProcRootDir;
// if m_strSrcRootDir and m_strProcRootDir are equal, it returns strOrigSrcName;
// if the name that it would return already exists, it inserts a ".proc.<NNN>" such that a file with this new name doesn't exist; (so if m_strSrcRootDir and m_strProcRootDir are equal, the original file should be renamed before calling this)
std::string TransfConfig::getProcName(const std::string& strOrigSrcName) const
{
    string strRelName;
    string strExt;
    splitOrigName(strOrigSrcName, strRelName, strExt);

    if (m_strSrcRootDir == m_strProcRootDir)
    {
        if (!fileExists(strOrigSrcName))
        {
            return strOrigSrcName;
        }

        // not quite OK; by the time this gets called, strOrigSrcName should have been renamed; well ...
    }
    else
    {
        string s (m_strProcRootDir + strRelName + strExt);
        if (!fileExists(s))
        {
            createDirForFile(s);
            return s;
        }
    }

    string strRes;

    for (int i = 1; ; ++i)
    {
        {
            ostringstream out;
            out << m_strProcRootDir << strRelName << ".proc." << setfill('0') << setw(3) << i << strExt;
            strRes = out.str();
        }

        if (!fileExists(strRes))
        {
            createDirForFile(strRes);
            return strRes;
        }
    }
}
#endif


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



/*override*/ Transformation::Result IdentityTransformation::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    if (h.getName() != strOrigSrcName) { return NOT_CHANGED; }
    transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
    copyFile(h.getName(), strTempName);

    return CHANGED_NO_RECALL;
}

//ttt1 test transformation (and perhaps the other threads) with this: wheat happens if something takes very long and the user presses both pause and resume before the UI checks




/*

ttt1 warnings + info


ID3 V2.3
ID3 V2.4
ID3 V1
APE
XING HDR
LAME HDR



no mp3gain

more than 50000 in unknown streams
more than 5% in unknown streams

Xing TOC incorrect (probably hard to do)

ttt2 When removing frames and Xing/Lame is present, adjust header for count and TOC (or remove TOC)

*/





//ttt2 indiv transformations should not generate duplicates of "original" and "temp" files; this should work only when the orig file is changed (there's no point if the modified file is elsewhere, because changes aren't visible) in the UI

//ttt2 perhaps separate tab with actions: "check quality", ... ; or perhaps tab with settings, so the quality is checked the first time

//ttt2 there are both "fixes" (pad truncated ...) and "actions" ("remove lyrics", "add picture", "check ID3"); perhaps distinguish between them
//ttt1 "check tags" command - goes to musicbrainz and checks all known tags; 2009.03.19 - this doesn't seem to make much sense, given that there are many albums with the same name, ... ; perhaps we should search by track duration too;



//ttt1 transform: convert to id3v2: assumes whatever reader order is currently in tag editor and writes to id3v2 if there are differences (including rating); maybe option to convert ID3V2.4.0 reagrdless; doing it manually isn't such a big pain, though;



