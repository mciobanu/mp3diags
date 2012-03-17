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

#include  <QApplication>
#include  <QFileInfo>
#include  <QDir>
#include  <QMessageBox>

#include  "Translation.h"

#include  "Helpers.h"
#include  "Version.h"

using namespace std;

using namespace Version;


/*

/usr/bin/MP3Diags-unstable
/usr/share/mp3diags-unstable/translations


/usr/local/bin/MP3Diags-unstable
/usr/local/share/mp3diags-unstable/translations

*/


TranslatorHandler::TranslatorHandler()
{
    m_vstrTranslations.push_back("mp3diags_en_US.qm");
    m_vstrLongTranslations.push_back("mp3diags_en_US.qm");
    addTranslations(convStr(QCoreApplication::instance()->applicationDirPath()));
    if (cSize(m_vstrTranslations) == 1)
    {
        addTranslations(convStr(QCoreApplication::instance()->applicationDirPath() + "/../share/" + getTranslationPackageName() + "/translations"));
    }
    //ttt0 perhaps also look in "my documents", so users can easily add translations

#if defined(WIN32) || defined(__OS2__)
    m_qstrSystemTranslDir = QCoreApplication::instance()->applicationDirPath();
#else
    m_qstrSystemTranslDir = "/usr/share/qt4/translations"; //ttt1 maybe improve
#endif
}

void TranslatorHandler::addTranslations(const std::string& strDir)
{
    qDebug("trying %s", strDir.c_str());
    QFileInfoList fileInfos (QDir(convStr(strDir), "mp3diags_*.qm").entryInfoList(QDir::NoDotAndDotDot | QDir::Files));
    for (int i = 0; i < fileInfos.size(); ++i)
    {
        m_vstrLongTranslations.push_back(convStr(fileInfos[i].canonicalFilePath()));
        m_vstrTranslations.push_back(convStr(fileInfos[i].fileName()));
        qDebug("added %s", m_vstrLongTranslations.back().c_str());
    }
}


string TranslatorHandler::getLocale(string strTranslation)
{
    strTranslation.erase(strTranslation.size() - 3, 3);
    strTranslation.erase(0, 8);
    return strTranslation;
}


void TranslatorHandler::setTranslation(const string& strTranslation)
{
    for (int i = 0; i < cSize(m_vstrTranslations); ++i)
    {
        if (m_vstrTranslations[i] == strTranslation)
        {
            m_appTranslator.load(convStr(m_vstrLongTranslations[i]));
            break;
        }
    }

    try
    {
        QCoreApplication::instance()->removeTranslator(&m_appTranslator);
        QCoreApplication::instance()->installTranslator(&m_appTranslator);

        // http://www.qtcentre.org/threads/20889-Translation-of-app-and-Qt-Dialogs
        //m_systemTranslator.load("qt_cs.qm", m_qstrSystemTranslDir);
        try
        {
            m_systemTranslator.load("qt" + convStr(getLocale(strTranslation)) + ".qm", m_qstrSystemTranslDir); // ttt0 see what is available in 4.7 and recheck that "de_DE" is loaded if there's no "de" or "de_CH"
            QCoreApplication::instance()->removeTranslator(&m_systemTranslator);
            QCoreApplication::instance()->installTranslator(&m_systemTranslator);
        }
        catch (...)
        { // !!! nothing; usually there is no system translator for this locale, so the file and color dialogs will be shown in English
        }
    }
    catch (...)
    {
        if (strTranslation != "")
        {
            QMessageBox::critical(0, "Error", "An error was encountered while setting up the translation. The program will continue, but the language may be incorrect.");
        }
    }
}


/*static*/ TranslatorHandler& TranslatorHandler::getGlobalTranslator()
{
    static TranslatorHandler hndl;
    return hndl;
}

/*
LocaleInfo::LocaleInfo(std::string strFileName) : m_strCountry("err"), m_strLanguage("err")
{
    // QLocale::nativeLanguageName and QLocale::nativeCountryName - in Qt 4.8
    if (endsWith(strFileName), "_en_US.qm")
    {
        m_strCountry = "USA";
        m_strLanguage = "English";
    }
    else if (endsWith(strFileName), "_cs.qm")
    {
        //m_strCountry = "Czech Republic";
        //m_strLanguage = "Czech";
        m_strCountry = "Czech Republic";
        m_strLanguage = "Czech";
    }
    else
    {
        if (endsWith(strFileName), ".qm")
        {
            strFileName.erase(strFileName.size() - 3);
            string::size_type k (strFileName.find_last_of("_"));
            if (k )
        }

    }
}
*/

/*static*/ string TranslatorHandler::getLanguageInfo(string strFileName)
{
    string::size_type k (strFileName.find_last_of(getPathSep()));
    if (k != string::npos)
    {
        strFileName.erase(0, k + 1);
    }

    if (strFileName == "mp3diags_cs.qm")
    {
        //return "Czech - Czech Republic";
        //return "Česko - Česká republika";
        return "Česko";
    }

    if (strFileName == "mp3diags_en_US.qm")
    {
        return "English - United States";
    }

    if (endsWith(strFileName, ".qm"))
    {
        strFileName.erase(strFileName.size() - 3);
        if (beginsWith(strFileName, "mp3diags_"))
        {
            strFileName.erase(0, strlen("mp3diags_"));
            //ttt0 look for "xx_XX" or "xx" and define some more
            return strFileName;
        }
    }

    return "Error - " + strFileName;
}

// to rebuild the translations use RebuildUkTransl.sh, which also updates all the .cs files, rather than MakeTranslations.sh, which assumes the .cs files are OK

