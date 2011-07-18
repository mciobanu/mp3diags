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


#ifndef StoredSettingsH
#define StoredSettingsH

#include  <string>
#include  <vector>

//#include  <QByteArray>
#include  <QStringList>  // ttt2 what we really want is QByteArray; however, by including QByteArray directly, lots of warnings get displayed; perhaps some defines are needed but don't know which; so we just include QStringList to avoid the warnings (see also Helpers.h)


class QSettings;
class TransfConfig;
class CommonData;

class SessionSettings
{
    QSettings* m_pSettings;
public:
    SessionSettings(const std::string& strSessFile);
    ~SessionSettings();

    void saveTransfConfig(const TransfConfig& transfConfig);
    bool loadTransfConfig(TransfConfig& transfConfig) const; // returns false if there was some error while loading (so the user can be told about defaults being used and those defaults could get saved)

    void saveDirs(const std::vector<std::string>& vstrIncludedDirs, const std::vector<std::string>& vstrExcludedDirs);
    bool loadDirs(std::vector<std::string>& vstrIncludedDirs, std::vector<std::string>& vstrExcludedDirs) const; // returns false if there were inconsistencies in the settings

    void saveScanAtStartup(bool b);
    bool loadScanAtStartup() const;

    void saveNoteFilterSettings(int nWidth, int nHeight);
    void loadNoteFilterSettings(int& nWidth, int& nHeight) const;

    void saveDirFilterSettings(int nWidth, int nHeight);
    void loadDirFilterSettings(int& nWidth, int& nHeight) const;

    void saveDiscogsSettings(int nWidth, int nHeight, int nStyleOption);
    void loadDiscogsSettings(int& nWidth, int& nHeight, int& nStyleOption) const;

    void saveMusicBrainzSettings(int nWidth, int nHeight);
    void loadMusicBrainzSettings(int& nWidth, int& nHeight) const;

    void saveConfigSize(int nWidth, int nHeight);
    void loadConfigSize(int& nWidth, int& nHeight) const;

    void saveDebugSettings(int nWidth, int nHeight);
    void loadDebugSettings(int& nWidth, int& nHeight) const;

    void saveExportSettings(int nWidth, int nHeight, bool bSortByShortNames, const std::string& strFile, bool bUseVisible, const std::string& strM3uRoot, const std::string& strM3uLocale);
    void loadExportSettings(int& nWidth, int& nHeight, bool& bSortByShortNames, std::string& strFile, bool& bUseVisible, std::string& strM3uRoot, std::string& strM3uLocale) const;

    void saveRenamerSettings(int nWidth, int nHeight, int nSaButton, int nVaButton, bool bKeepOrig, bool bUnratedAsDuplicate);
    void loadRenamerSettings(int& nWidth, int& nHeight, int& nSaButton, int& nVaButton, bool& bKeepOrig, bool& bUnratedAsDuplicate) const;

    void saveMainSettings(int nWidth, int nHeight, int nNotesGW0, int nNotesGW2, int nStrmsGW0, int nStrmsGW1, int nStrmsGW2, int nStrmsGW3, int nUnotesGW0, const QByteArray& stateMainSpl, const QByteArray& stateLwrSpl, int nIconSize, int nScanWidth);
    void loadMainSettings(int& nWidth, int& nHeight, int& nNotesGW0, int& nNotesGW2, int& nStrmsGW0, int& nStrmsGW1, int& nStrmsGW2, int& nStrmsGW3, int& nUnotesGW0, QByteArray& stateMainSpl, QByteArray& stateLwrSpl, int& nIconSize, int& nScanWidth) const;

    void saveNormalizeSettings(int nWidth, int nHeight);
    void loadNormalizeSettings(int& nWidth, int& nHeight) const;

    void saveTagEdtPatternsSettings(int nWidth, int nHeight);
    void loadTagEdtPatternsSettings(int& nWidth, int& nHeight) const;

    void saveRenamerPatternsSettings(int nWidth, int nHeight);
    void loadRenamerPatternsSettings(int& nWidth, int& nHeight) const;

    void saveTagEdtSettings(int nWidth, int nHeight, int nArtistCase, int nTitleCase); // Case params are really enum
    void loadTagEdtSettings(int& nWidth, int& nHeight, int& nArtistsCase, int& nOthersCase) const;

    void saveVector(const std::string& strPath, const std::vector<std::string>& v);
    std::vector<std::string> loadVector(const std::string& strPath, bool& bErr) const; // allows empty entries, but stops at the first missing entry, in which case sets bErr

    void saveMiscConfigSettings(const CommonData*);
    void loadMiscConfigSettings(CommonData*) const;

    void saveDbDirty(bool bDirty);
    void loadDbDirty(bool& bDirty);

    void saveCrashedAtStartup(bool bCrashedAtStartup);
    void loadCrashedAtStartup(bool& bCrashedAtStartup);

    void saveVersion(const std::string& strVersion);
    void loadVersion(std::string& strVersion);

    //ttt2 ??? see about ThreadRunner size; perhaps set width to its parent

    bool sync();
};




class GlobalSettings
{
    QSettings* m_pSettings;
    //static QSettings* getGlobalSettings();
public:
    GlobalSettings();
    ~GlobalSettings();

    enum { IGNORE_EXTERNAL_CHANGES = 0, LOAD_EXTERNAL_CHANGES = 1 };
    void saveSessions(const std::vector<std::string>& vstrSess, const std::string& strLastSess, bool bOpenLast, const std::string& strTempSessTempl, const std::string& strDirSessTempl, bool bLoadExternalChanges);
    void loadSessions(std::vector<std::string>& vstrSess, std::string& strLastSess, bool& bOpenLast, std::string& strTempSessTempl, std::string& strDirSessTempl) const;

    void saveSessionsDlgSize(int nWidth, int nHeight);
    void loadSessionsDlgSize(int& nWidth, int& nHeight) const;

    void saveSessionEdtSize(int nWidth, int nHeight);
    void loadSessionEdtSize(int& nWidth, int& nHeight) const;
};

#endif



