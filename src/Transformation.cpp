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
#include  <QMessageBox>

#include  "Transformation.h"

#include  "Helpers.h"
#include  "OsFile.h"
#include  "Mp3Manip.h"
#include  "CommonData.h"


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

//ttt2 review all defaults
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



TransfConfig::Options::Options() :
        //m_nProcOrigChange(5), // move w/o renaming, if doesn't exist; discard if it exists;
        m_eProcOrigChange(PO_ERASE), // erase
        m_bProcOrigUseLabel(false),
        m_bProcOrigAlwayUseCounter(false),

        m_eUnprocOrigChange(UPO_DONT_CHG), // don't change
        m_bUnprocOrigUseLabel(false),
        m_bUnprocOrigAlwayUseCounter(false),

        m_eProcessedCreate(PR_CREATE_RENAME_IF_USED), // create and rename if the name is in use
        m_bProcessedUseLabel(true),
        m_bProcessedAlwayUseCounter(true),
        m_bProcessedUseSeparateDir(false),

        m_bTempCreate(false),

        m_bCompCreate(false),

        m_bKeepOrigTime(false)
{
}




// returns *this, with some fields changed to match a "backup" configuration; touches many field, but ignores others (m_bProcOrigUseLabel, m_bProcOrigAlwayUseCounter, m_bUnprocOrigUseLabel, m_bUnprocOrigAlwayUseCounter, m_bKeepOrigTime)
TransfConfig::Options TransfConfig::Options::asBackup() const
{
    Options opt (*this);

    opt.m_bTempCreate = false;

    opt.m_bCompCreate = false;

    opt.m_eProcOrigChange = PO_MOVE_OR_ERASE;
    //opt.m_bProcOrigUseLabel = false;
    //opt.m_bProcOrigAlwayUseCounter = false;

    opt.m_eUnprocOrigChange = UPO_DONT_CHG; // don't change
    //opt.m_bUnprocOrigUseLabel = false;
    //opt.m_bUnprocOrigAlwayUseCounter = false;

    opt.m_eProcessedCreate = PR_CREATE_RENAME_IF_USED; // create and rename if the name is in use
    opt.m_bProcessedUseLabel = true;
    opt.m_bProcessedAlwayUseCounter = true;
    opt.m_bProcessedUseSeparateDir = false;

    //opt.m_bKeepOrigTime = false;

    return opt;
}

// returns *this, with some fields changed to match a "non-backup" configuration; touches many field, but ignores others (m_bProcOrigUseLabel, m_bProcOrigAlwayUseCounter, m_bUnprocOrigUseLabel, m_bUnprocOrigAlwayUseCounter, m_bKeepOrigTime)
TransfConfig::Options TransfConfig::Options::asNonBackup() const
{
    Options opt (*this);

    opt.m_bTempCreate = false;

    opt.m_bCompCreate = false;

    opt.m_eProcOrigChange = PO_ERASE; // move w/o renaming, if doesn't exist; discard if it exists;
    //opt.m_bProcOrigUseLabel = false;
    //opt.m_bProcOrigAlwayUseCounter = false;

    opt.m_eUnprocOrigChange = UPO_DONT_CHG; // don't change
    //opt.m_bUnprocOrigUseLabel = false;
    //opt.m_bUnprocOrigAlwayUseCounter = false;

    opt.m_eProcessedCreate = PR_CREATE_RENAME_IF_USED; // create and rename if the name is in use
    opt.m_bProcessedUseLabel = true;
    opt.m_bProcessedAlwayUseCounter = true;
    opt.m_bProcessedUseSeparateDir = false;

    //opt.m_bKeepOrigTime = false;

    return opt;
}



// like getDefaultOptions() but does backups rather than erase the original file (if a backup exists, the file is deleted, though)
/*static* / TransfConfig::Options TransfConfig::getBackupDefaultOptions()
{
    Options opt (getDefaultOptions());
    opt.m_nProcOrigChange = 5; // move w/o renaming, if doesn't exist; discard if it exists;
    return opt;
}
*/


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
    m_bInitError = false;
    m_strSrcDir = "*" == strSrcDir ? getDefaultSrc() : strSrcDir;
    m_strProcOrigDir = "*" == strProcOrigDir ? getDefaultProcOrig() : strProcOrigDir;
    m_strUnprocOrigDir = "*" == strUnprocOrigDir ? getDefaultUnprocOrig() : strUnprocOrigDir;
    m_strProcessedDir = "*" == strProcessedDir ? getDefaultProcessed() : strProcessedDir;
    m_strTempDir = "*" == strTempDir ? getDefaultTemp() : strTempDir;
    m_strCompDir = "*" == strCompDir ? getDefaultComp() : strCompDir;

    if (nOptions >= 0x00040000)
    { // only the first 18 bits are supposed to be used; if more seem to be used, it is because of a manually entered wrong value or because a change in the bitfield representation;
        QMessageBox::critical(getMainForm(), "Error", "Invalid value found for file settings. Reverting to default settings.");
        nOptions = -1;
        m_bInitError = true;
    }

    if (-1 != nOptions)
    {
        m_options.setVal(nOptions);
    }

    checkDirName(m_strSrcDir);
    checkDirName(m_strProcOrigDir);  // some of these dirs are allowed to be empty, just like src when comes from the config dialog, empty meaning "/", so that should work for all, probably
    checkDirName(m_strUnprocOrigDir);
    checkDirName(m_strProcessedDir);
    checkDirName(m_strTempDir);
    checkDirName(m_strCompDir);
}


TransfConfig::TransfConfig() : m_bInitError(false)
{
    m_strSrcDir = getDefaultSrc();
    m_strProcOrigDir = getDefaultProcOrig();
    m_strUnprocOrigDir = getDefaultUnprocOrig();
    m_strProcessedDir = getDefaultProcessed();
    m_strTempDir = getDefaultTemp();
    m_strCompDir = getDefaultComp();
}




// last '/' goes to dir; last '.' goes to ext (if extension present)
void TransfConfig::splitOrigName(const string& strOrigSrcName, std::string& strRelDir, std::string& strBaseName, std::string& strExt) const
{
//TRACER1A("splitOrigName ", 1);
//Tracer t1 (strOrigName);
    //TRACER1A("splitOrigName ", 1);
    //TRACER1(strOrigSrcName.c_str(), 2);
    CB_CHECK1 (beginsWith(strOrigSrcName, m_strSrcDir), IncorrectPath());
    //TRACER1A("splitOrigName ", 3);

    strRelDir = strOrigSrcName.substr(m_strSrcDir.size());
#if defined(WIN32)
    //CB_CHECK1 (beginsWith(strRelDir, getPathSepAsStr()), IncorrectPath()); //ttt2 see if it's OK to skip this test on Windows; it probably is, because it's more of an assert and the fact that it's not triggered on Linux should be enough
#elif defined(__OS2__)
    //nothing
#else
    CB_CHECK1 (beginsWith(strRelDir, getPathSepAsStr()), IncorrectPath());
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
    //TRACER1A("splitOrigName ", 6);
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
    //TRACER1A("addLabelAndCounter ", 1);
    string strRes;
    if (!bAlwaysRename)
    {
        //TRACER1A("addLabelAndCounter ", 2);
        strRes = replaceDriveLetter(s1 + s2);
        //TRACER1A("addLabelAndCounter ", 3);
        if (!fileExists(strRes))
        {
            //TRACER1A("addLabelAndCounter ", 4);
            createDirForFile(strRes);
            //TRACER1A("addLabelAndCounter ", 5);
            return strRes;
        }
    }

    string strLabel2 (strLabel);
    if (!strLabel2.empty()) { strLabel2.insert(0, "."); }

    //TRACER1A("addLabelAndCounter ", 6);
    if (!bAlwaysUseCounter && !strLabel2.empty())
    {
        //TRACER1A("addLabelAndCounter ", 7);
        strRes = replaceDriveLetter(s1 + strLabel2 + s2);
        //TRACER1A("addLabelAndCounter ", 8);
        if (!fileExists(strRes))
        {
            //TRACER1A("addLabelAndCounter ", 9);
            createDirForFile(strRes);
            //TRACER1A("addLabelAndCounter ", 10);
            return strRes;
        }
    }
    //TRACER1A("addLabelAndCounter ", 11);

    for (int i = 1; ; ++i)
    {
        {
            ostringstream out;
            out << "." << setfill('0') << setw(3) << i;
            //TRACER1A("addLabelAndCounter ", 12);
            strRes = replaceDriveLetter(s1 + strLabel2 + out.str() + s2);
            //TRACER1A("addLabelAndCounter ", 13);
        }

        //TRACER1A("addLabelAndCounter ", 14);
        if (!fileExists(strRes))
        {
            //TRACER1A("addLabelAndCounter ", 15);
            createDirForFile(strRes);
            //TRACER1A("addLabelAndCounter ", 16);
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
    //TRACER1A("getRenamedName ", 1);
    splitOrigName(strOrigSrcName, strRelDir, strBaseName, strExt);

    string strRes;
    //TRACER1A("getRenamedName ", 2);

    if (!bAlwaysRename)
    { // aside from the new folder, no renaming takes places if this new name isn't in use
        //TRACER1A("getRenamedName ", 3);
        strRes = replaceDriveLetter(strNewRootDir + strRelDir + strBaseName + strExt);
        if (!fileExists(strRes) || bAllowDup)
        {
            //TRACER1A("getRenamedName ", 4);
            createDirForFile(strRes);
            //TRACER1A("getRenamedName ", 5);
            return strRes;
        }
    }
    //TRACER1A("getRenamedName ", 6);

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
    switch (nChange) // !!! nChange must match both ProcOrig and UnprocOrig, since getChangedOrigNameHlp() gets called in both cases
    {
    case Options::PO_DONT_CHG: // don't change unprocessed orig file
        strNewName = strOrigSrcName; return ORIG_DONT_CHANGE;

    case Options::PO_ERASE: // erase unprocessed orig file
        strNewName = strOrigSrcName; return ORIG_ERASE;

    case Options::PO_MOVE_ALWAYS_RENAME: // move unprocessed orig file to strDestDir; always rename
        strNewName = getRenamedName(strOrigSrcName, strDestDir, bUseLabel ? "orig" : "", bAlwayUseCounter, ALWAYS_RENAME);
        return ORIG_MOVE;

    case Options::PO_MOVE_RENAME_IF_USED: // move unprocessed orig file to strDestDir; rename only if name is in use
        strNewName = getRenamedName(strOrigSrcName, strDestDir, bUseLabel ? "orig" : "", bAlwayUseCounter, RENAME_IF_NEEDED);
        return ORIG_MOVE;

    case Options::PO_RENAME_SAME_DIR: // rename in the same dir
        strNewName = getRenamedName(strOrigSrcName, m_strSrcDir, bUseLabel ? "orig" : "", bAlwayUseCounter, ALWAYS_RENAME);
        return ORIG_MOVE;

    case Options::PO_MOVE_OR_ERASE: // move unprocessed orig file to strDestDir if it's not already there; erase if it exists;
        strNewName = getRenamedName(strOrigSrcName, strDestDir, "", USE_COUNTER_IF_NEEDED, RENAME_IF_NEEDED, ALLOW_DUPLICATES);
        return ORIG_MOVE_OR_ERASE;

    default:
        CB_ASSERT(false);
    }
}


TransfConfig::OrigFile TransfConfig::getProcOrigName(string strOrigSrcName, string& strNewName) const
{
    removeSuffix(strOrigSrcName);
    return getChangedOrigNameHlp(strOrigSrcName, m_strProcOrigDir, m_options.m_eProcOrigChange, m_options.m_bProcOrigUseLabel, m_options.m_bProcOrigAlwayUseCounter, strNewName);
}


TransfConfig::OrigFile TransfConfig::getUnprocOrigName(string strOrigSrcName, string& strNewName) const
{
    removeSuffix(strOrigSrcName);
    return getChangedOrigNameHlp(strOrigSrcName, m_strUnprocOrigDir, m_options.m_eUnprocOrigChange, m_options.m_bUnprocOrigUseLabel, m_options.m_bUnprocOrigAlwayUseCounter, strNewName);
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
//TRACER1A("getProcessedName ", 1);
//Tracer t1 (strOrigSrcName);
    removeSuffix(strOrigSrcName);
    //TRACER1A("getProcessedName ", 2);
    //Tracer t2 (strOrigSrcName);
    switch (m_options.m_eProcessedCreate)
    {
    case Options::PR_DONT_CREATE: // don't create proc files
        {
            //TRACER1A("getProcessedName ", 3);
            strName.clear(); return TRANSF_DONT_CREATE;
        }

    case Options::PR_CREATE_ALWAYS_RENAME: // create proc files and always rename
        {
            //TRACER1A("getProcessedName ", 4);
            strName = getRenamedName(strOrigSrcName, m_options.m_bProcessedUseSeparateDir ? m_strProcessedDir : m_strSrcDir, m_options.m_bProcessedUseLabel ? "proc" : "", m_options.m_bProcessedAlwayUseCounter, ALWAYS_RENAME);
            //Tracer t1 (strName);
            return TRANSF_CREATE;
        }

    case Options::PR_CREATE_RENAME_IF_USED: // create proc files and rename if the name is in use
        {
            //TRACER1A("getProcessedName ", 5);
            strName = getRenamedName(strOrigSrcName, m_options.m_bProcessedUseSeparateDir ? m_strProcessedDir : m_strSrcDir, m_options.m_bProcessedUseLabel ? "proc" : "", m_options.m_bProcessedAlwayUseCounter, RENAME_IF_NEEDED);
            //Tracer t1 (strName);
            return TRANSF_CREATE;
        }

    default:
        CB_ASSERT(false);
    }
}


TransfConfig::TransfFile TransfConfig::getTempName(string strOrigSrcName, const std::string& strOpName, std::string& strName) const
{
    removeSuffix(strOrigSrcName);
    if (!m_options.m_bTempCreate)
    {
        strName = getTempFile(strOrigSrcName);
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

    if (!m_options.m_bCompCreate)
    {
        strBefore = getTempFile(strOrigSrcName);
        strAfter = getTempFile(strOrigSrcName);
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
    switch (m_options.m_eProcOrigChange)
    {
    case Options::PO_DONT_CHG: // don't change unprocessed orig file
        return ORIG_DONT_CHANGE;

    case Options::PO_ERASE: // erase unprocessed orig file
        return ORIG_ERASE;

    case Options::PO_MOVE_ALWAYS_RENAME: // move unprocessed orig file to strDestDir; always rename
    case Options::PO_MOVE_RENAME_IF_USED: // move unprocessed orig file to strDestDir; rename only if name is in use
    case Options::PO_RENAME_SAME_DIR: // rename in the same dir
        return ORIG_MOVE;

    case Options::PO_MOVE_OR_ERASE: // move if dest doesn't exist; erase if it does;
        return ORIG_MOVE_OR_ERASE;

    default:
        CB_ASSERT(false);
    }
}


TransfConfig::OrigFile TransfConfig::getUnprocOrigAction() const
{
    switch (m_options.m_eUnprocOrigChange)
    {
    case Options::UPO_DONT_CHG: // don't change unprocessed orig file
        return ORIG_DONT_CHANGE;

    case Options::UPO_ERASE: // erase unprocessed orig file
        return ORIG_ERASE;

    case Options::UPO_MOVE_ALWAYS_RENAME: // move unprocessed orig file to strDestDir; always rename
    case Options::UPO_MOVE_RENAME_IF_USED: // move unprocessed orig file to strDestDir; rename only if name is in use
    case Options::UPO_RENAME_SAME_DIR: // rename in the same dir
        return ORIG_MOVE;

    default:
        CB_ASSERT(false);
    }
}


TransfConfig::TransfFile TransfConfig::getProcessedAction() const
{
    switch (m_options.m_eProcessedCreate)
    {
    case Options::PR_DONT_CREATE: // don't create proc files
        return TRANSF_DONT_CREATE;

    case Options::PR_CREATE_ALWAYS_RENAME: // create proc files and always rename
    case Options::PR_CREATE_RENAME_IF_USED: // create proc files and rename if the name is in use
        return TRANSF_CREATE;

    default:
        CB_ASSERT(false);
    }
}

TransfConfig::TransfFile TransfConfig::getTempAction() const
{
    return m_options.m_bTempCreate ? TRANSF_CREATE : TRANSF_DONT_CREATE;
}

TransfConfig::TransfFile TransfConfig::getCompAction() const
{
    return m_options.m_bCompCreate ? TRANSF_CREATE : TRANSF_DONT_CREATE;
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

//ttt2 test transformation (and perhaps the other threads) with this: wheat happens if something takes very long and the user presses both pause and resume before the UI checks




/*

ttt2 warnings + info


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
//ttt2 "check tags" command - goes to musicbrainz and checks all known tags; 2009.03.19 - this doesn't seem to make much sense, given that there are many albums with the same name, ... ; perhaps we should search by track duration too;



//ttt2 transform: convert to id3v2: assumes whatever reader order is currently in tag editor and writes to id3v2 if there are differences (including rating); maybe option to convert ID3V2.4.0 reagrdless; doing it manually isn't such a big pain, though;


//ttt1 see why some files are read-only on W7

