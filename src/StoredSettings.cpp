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


#include  <QSettings>
#include  <QFileInfo>

#include  "StoredSettings.h"

#include  "Helpers.h"

using namespace std;

//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================





SessionSettings::SessionSettings(const string& strIniFile)
{
    m_pSettings = new QSettings(convStr(strIniFile), QSettings::IniFormat);
}


SessionSettings::~SessionSettings()
{
    delete m_pSettings;
}


void SessionSettings::saveDirs(const vector<string>& vstrIncludedDirs, const vector<string>& vstrExcludedDirs)
{
    saveVector("folders/included", vstrIncludedDirs);
    saveVector("folders/excluded", vstrExcludedDirs);
}

// returns false if there were inconsistencies in the settings
bool SessionSettings::loadDirs(vector<string>& vstrIncludedDirs, vector<string>& vstrExcludedDirs) const
{
    bool bRes1 (true), bRes2 (true);

    {
        vector<string> v;
        vstrIncludedDirs = loadVector("folders/included", bRes1);

        for (int i = 0, n = cSize(vstrIncludedDirs); i < n; ++i)
        {
            string s (vstrIncludedDirs[i]);
            if (QFileInfo(convStr(s)).isDir())
            {
                v.push_back(s);
            }
            else
            {
                bRes1 = false;
            }
        }

        v.swap(vstrIncludedDirs);
    }

    {
        vector<string> v;
        vstrExcludedDirs = loadVector("folders/excluded", bRes2);

        for (int i = 0, n = cSize(vstrExcludedDirs); i < n; ++i)
        {
            string s (vstrExcludedDirs[i]);
            if (QFileInfo(convStr(s)).isDir())
            {
                v.push_back(s);
            }
            else
            {
                bRes2 = false;
            }
        }

        v.swap(vstrExcludedDirs);
    }

    return bRes1 && bRes2;
}


void SessionSettings::saveScanAtStartup(bool b)
{
    m_pSettings->setValue("main/scanAtStartup", b);
}

bool SessionSettings::loadScanAtStartup() const
{
    return m_pSettings->value("main/scanAtStartup", true).toBool();
}


bool SessionSettings::sync()
{
    m_pSettings->sync();

    return QSettings::NoError == m_pSettings->status();
}



//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================



void SessionSettings::saveMainSettings(int nWidth, int nHeight, int nNotesGW0, int nNotesGW2, int nStrmsGW0, int nStrmsGW1, int nStrmsGW2, int nStrmsGW3, int nUnotesGW0, const QByteArray& stateMainSpl, const QByteArray& stateLwrSpl, int nIconSize, int nScanWidth)
{
    m_pSettings->setValue("main/width", nWidth);
    m_pSettings->setValue("main/height", nHeight);

    m_pSettings->setValue("main/notesGWidth0", nNotesGW0);
    m_pSettings->setValue("main/notesGWidth2", nNotesGW2);

    m_pSettings->setValue("main/streamsGWidth0", nStrmsGW0);
    m_pSettings->setValue("main/streamsGWidth1", nStrmsGW1);
    m_pSettings->setValue("main/streamsGWidth2", nStrmsGW2);
    m_pSettings->setValue("main/streamsGWidth3", nStrmsGW3);

    m_pSettings->setValue("main/uniqueNotesGWidth0", nUnotesGW0);

    /*QList<int> l (m_pMainSplitter->sizes());
    for (QList<int>::iterator it = l.begin(), end = l.end(); it != end; ++it)
    {
        qDebug("l %d", *it);
    }*/
    m_pSettings->setValue("main/splitterState", stateMainSpl);
    m_pSettings->setValue("main/lowerSplitterState", stateLwrSpl);

    m_pSettings->setValue("main/iconSize", nIconSize);

    m_pSettings->setValue("main/scanWidth", nScanWidth);
}

void SessionSettings::loadMainSettings(int& nWidth, int& nHeight, int& nNotesGW0, int& nNotesGW2, int& nStrmsGW0, int& nStrmsGW1, int& nStrmsGW2, int& nStrmsGW3, int& nUnotesGW0, QByteArray& stateMainSpl, QByteArray& stateLwrSpl, int& nIconSize, int& nScanWidth) const
{
    nWidth = m_pSettings->value("main/width").toInt();
    nHeight = m_pSettings->value("main/height").toInt();

    nNotesGW0 = m_pSettings->value("main/notesGWidth0").toInt();
    nNotesGW2= m_pSettings->value("main/notesGWidth2").toInt();

    nStrmsGW0 = m_pSettings->value("main/streamsGWidth0").toInt();
    nStrmsGW1 = m_pSettings->value("main/streamsGWidth1").toInt();
    nStrmsGW2 = m_pSettings->value("main/streamsGWidth2").toInt();
    nStrmsGW3 = m_pSettings->value("main/streamsGWidth3").toInt();

    nUnotesGW0 = m_pSettings->value("main/uniqueNotesGWidth0").toInt();

    stateMainSpl = m_pSettings->value("main/splitterState").toByteArray();
    stateLwrSpl = m_pSettings->value("main/lowerSplitterState").toByteArray();

    nIconSize = m_pSettings->value("main/iconSize", 0).toInt();

    nScanWidth = m_pSettings->value("main/scanWidth").toInt();
}



void SessionSettings::saveMusicBrainzSettings(int nWidth, int nHeight)
{
    m_pSettings->setValue("musicbrainz/width", nWidth);
    m_pSettings->setValue("musicbrainz/height", nHeight);
}

void SessionSettings::loadMusicBrainzSettings(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("musicbrainz/width").toInt();
    nHeight = m_pSettings->value("musicbrainz/height").toInt();
}



void SessionSettings::saveDebugSettings(int nWidth, int nHeight, bool bSortByShortNames)
{
    m_pSettings->setValue("debug/width", nWidth);
    m_pSettings->setValue("debug/height", nHeight);
    m_pSettings->setValue("debug/sortByShortNames", bSortByShortNames);
}

void SessionSettings::loadDebugSettings(int& nWidth, int& nHeight, bool& bSortByShortNames) const
{
    nWidth = m_pSettings->value("debug/width").toInt();
    nHeight = m_pSettings->value("debug/height").toInt();
    bSortByShortNames = m_pSettings->value("debug/sortByShortNames", false).toBool();
}



void SessionSettings::saveDirFilterSettings(int nWidth, int nHeight)
{
    m_pSettings->setValue("dirFilter/width", nWidth);
    m_pSettings->setValue("dirFilter/height", nHeight);
}

void SessionSettings::loadDirFilterSettings(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("dirFilter/width").toInt();
    nHeight = m_pSettings->value("dirFilter/height").toInt();
}



void SessionSettings::saveDiscogsSettings(int nWidth, int nHeight)
{
    m_pSettings->setValue("discogs/width", nWidth);
    m_pSettings->setValue("discogs/height", nHeight);
}

void SessionSettings::loadDiscogsSettings(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("discogs/width").toInt();
    nHeight = m_pSettings->value("discogs/height").toInt();
}



void SessionSettings::saveRenamerSettings(int nWidth, int nHeight, int nSaButton, int nVaButton, bool bKeepOrig)
{
    m_pSettings->setValue("fileRenamer/width", nWidth);
    m_pSettings->setValue("fileRenamer/height", nHeight);
    m_pSettings->setValue("fileRenamer/saButton", nSaButton);
    m_pSettings->setValue("fileRenamer/vaButton", nVaButton);
    m_pSettings->setValue("fileRenamer/keepOriginal", bKeepOrig);
}

void SessionSettings::loadRenamerSettings(int& nWidth, int& nHeight, int& nSaButton, int& nVaButton, bool& bKeepOrig) const
{
    nWidth = m_pSettings->value("fileRenamer/width").toInt();
    nHeight = m_pSettings->value("fileRenamer/height").toInt();
    nSaButton = m_pSettings->value("fileRenamer/saButton").toInt();
    nVaButton = m_pSettings->value("fileRenamer/vaButton").toInt();
    bKeepOrig = m_pSettings->value("fileRenamer/keepOriginal", false).toBool();
}



void SessionSettings::saveNormalizeSettings(int nWidth, int nHeight)
{
    m_pSettings->setValue("normalizer/width", nWidth);
    m_pSettings->setValue("normalizer/height", nHeight);
}

void SessionSettings::loadNormalizeSettings(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("normalizer/width").toInt();
    nHeight = m_pSettings->value("normalizer/height").toInt();
}



void SessionSettings::saveNoteFilterSettings(int nWidth, int nHeight)
{
    m_pSettings->setValue("noteFilter/width", nWidth);
    m_pSettings->setValue("noteFilter/height", nHeight);
}

void SessionSettings::loadNoteFilterSettings(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("noteFilter/width").toInt();
    nHeight = m_pSettings->value("noteFilter/height").toInt();
}



void SessionSettings::saveRenamerPatternsSettings(int nWidth, int nHeight)
{
    m_pSettings->setValue("fileRenamer/patterns/width", nWidth);
    m_pSettings->setValue("fileRenamer/patterns/height", nHeight);
}

void SessionSettings::loadRenamerPatternsSettings(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("fileRenamer/patterns/width").toInt();
    nHeight = m_pSettings->value("fileRenamer/patterns/height").toInt();
}


void SessionSettings::saveTagEdtSettings(int nWidth, int nHeight, const QByteArray& splitterState)
{
    m_pSettings->setValue("tagEditor/width", nWidth);
    m_pSettings->setValue("tagEditor/height", nHeight);
    m_pSettings->setValue("tagEditor/splitterState", splitterState);
}

void SessionSettings::loadTagEdtSettings(int& nWidth, int& nHeight, QByteArray& splitterState) const
{
    nWidth = m_pSettings->value("tagEditor/width").toInt();
    nHeight = m_pSettings->value("tagEditor/height").toInt();

    splitterState = m_pSettings->value("tagEditor/splitterState").toByteArray();
}



void SessionSettings::saveTagEdtPatternsSettings(int nWidth, int nHeight)
{
    m_pSettings->setValue("tagEditor/patterns/width", nWidth);
    m_pSettings->setValue("tagEditor/patterns/height", nHeight);
}

void SessionSettings::loadTagEdtPatternsSettings(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("tagEditor/patterns/width").toInt();
    nHeight = m_pSettings->value("tagEditor/patterns/height").toInt();
}


void SessionSettings::saveVector(const string& strPath, const vector<string>& v)
{
    //  not sure how beginReadArray() & Co handle error cases, so it seems safer this way (considering that the users may want to alter the config file manually)
    char bfr [50];
    QString s (convStr(strPath));
    m_pSettings->remove(s);

    int n (cSize(v));
    m_pSettings->setValue(s + "/count", n);
    for (int i = 0; i < n; ++i)
    {
        sprintf(bfr, "/val%04d", i);
        m_pSettings->setValue(s + bfr, convStr(v[i]));
    }
}



// allows empty entries, but stops at the first missing entry, in which case sets bErr
vector<string> SessionSettings::loadVector(const string& strPath, bool& bErr) const
{
    vector<string> v;
    QString s (convStr(strPath));
    bErr = false;

    int n (m_pSettings->value(s + "/count", 0).toInt());
    char bfr [50];

    for (int i = 0; i < n; ++i)
    {
        sprintf(bfr, "/val%04d", i);
        string strName (convStr(m_pSettings->value(s + bfr, "\2").toString()));
        if ("\2" == strName)
        {
            bErr = true;
            break;
        }
        v.push_back(strName);
    }

    return v;
}




//================================================================================================================================================
//================================================================================================================================================
//================================================================================================================================================


void SessionSettings::saveConfigSize(int nWidth, int nHeight)
{
    m_pSettings->setValue("config/width", nWidth);
    m_pSettings->setValue("config/height", nHeight);
}

void SessionSettings::loadConfigSize(int& nWidth, int& nHeight) const
{
    nWidth = m_pSettings->value("config/width").toInt();
    nHeight = m_pSettings->value("config/height").toInt();
}



