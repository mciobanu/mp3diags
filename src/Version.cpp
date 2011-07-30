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


#include  <string>

using namespace std;

namespace Version {


const char* getAppVer()
{
    return "- custom build";
}

const char* getSimpleAppVer()
{
    static string s (getAppVer());
    static const char* szRes (0);
    if (0 == szRes)
    {
        string::size_type k (s.rfind("."));
        if (k != string::npos)
        {
            s.erase(k);
        }
        szRes = s.c_str();
    }
    return szRes;
}

// used, e.g. for location at SourceForge
const char* getWebBranch()
{
    return "/unstable";
}

// to be shown to the user in various forms (app title, About box, shell integration, ...)
const char* getAppName()
{
    return "MP3 Diags Unstable";
}

// icon name, needed for shell integration in Linux
const char* getIconName()
{
    return "MP3Diags-unstable";
}

// used for location of the documentation
const char* getHelpPackageName()
{
    return "mp3diags-unstable";
}


const char* getSettingsAppName()
{
    return "Mp3Diags-unstable"; //ttt1 maybe replace this with "mp3diags-unstable", but have some code to import older settings and then clear them
}

// for config only
const char* getOrganization()
{
    return "Ciobi";
}

} // namespace Version

