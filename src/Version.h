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


extern const char* APP_VER;

// used, e.g. for location at SourceForge
extern const char* WEB_BRANCH;

// to be shown to the user in various forms (app title, About box, shell integration, ...)
extern const char* APP_NAME;

// used for file names, e.g executable on Linux or icons
extern const char* PACKAGE_NAME;

//ttt0 replace this with PACKAGE_NAME, but have some code to import older settings and then clear them
extern const char* SETTINGS_APP_NAME;

// for config only
extern const char* ORGANIZATION;
