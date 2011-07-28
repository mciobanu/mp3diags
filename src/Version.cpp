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


const char* APP_VER ("- custom build");

// used, e.g. for location at SourceForge
const char* WEB_BRANCH ("/unstable");

// to be shown to the user in various forms (app title, About box, shell integration, ...)
const char* APP_NAME ("MP3 Diags Unstable");

// used for file names, e.g executable on Linux or icons
//const char* PACKAGE_NAME ("mp3diags-unstable");
const char* PACKAGE_NAME ("MP3Diags"); //ttt0 use above version after testing

//ttt0 replace this with PACKAGE_NAME, but have some code to import older settings and then clear them
const char* SETTINGS_APP_NAME ("Mp3Diags-unstable");

// for config only
const char* ORGANIZATION ("Ciobi");
