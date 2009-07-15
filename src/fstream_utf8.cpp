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

//#include  <QSettings>
//#include  <iostream>

#include  "fstream_utf8.h"


//#include  "Helpers.h" //ttt remove

#ifndef WIN32
/*int convertUtf8(const char* szUtf8Name, std::ios_base::openmode __mode)
{
    int nAcc;
    if (ios_base::in & __mode)
    {
        if (ios_base::out & __mode)
        {
            nAcc = O_RDWR | O_CREAT | O_LARGEFILE;
        }
        else
        {
            nAcc = O_RDONLY;
        }
    }
    else if (ios_base::out & __mode)
    {
        nAcc = O_RDWR | O_CREAT | O_LARGEFILE;
    }
    else
    {
        throw 1; // ttt0
    }

    if (ios_base::trunc & __mode)
    {
        nAcc = nAcc | O_TRUNC;
    }

    int nFd (open(szUtf8Name, nAcc, 0666));
    qDebug("fd %d %s acc=%d", nFd, szUtf8Name, nAcc);
    return nFd;
}*/


#else

#include  <windows.h>
#include  <sys/stat.h>
#include  <vector>

using namespace std;
using namespace __gnu_cxx;


//ttt0 perhaps add flush()


// might want to look at basic_file_stdio.cc or ext/stdio_filebuf.h

// http://www2.roguewave.com/support/docs/leif/sourcepro/html/stdlibref/basic-ifstream.html
// STLPort
// MSVC
// Dinkumware - not

// http://www.aoc.nrao.edu/~tjuerges/ALMA/STL/html/class____gnu__cxx_1_1stdio__filebuf.html


int convertUtf8(const char* szUtf8Name, std::ios_base::openmode __mode)
{
    int nAcc;
    if (ios_base::in & __mode)
    {
        if (ios_base::out & __mode)
        {
            nAcc = O_RDWR | O_CREAT;
        }
        else
        {
            nAcc = O_RDONLY;
        }
    }
    else if (ios_base::out & __mode)
    {
        nAcc = O_RDWR | O_CREAT;
    }
    else
    {
        throw 1; // ttt0
    }

    #ifdef O_LARGEFILE
    nAcc = nAcc | O_LARGEFILE; //ttt2
    #endif

    if (ios_base::trunc & __mode)
    {
        nAcc = nAcc | O_TRUNC;
    }

    if (ios_base::binary & __mode)
    {
        nAcc = nAcc | O_BINARY;
    }

    int nFd (_wopen(wstrFromUtf8(szUtf8Name).c_str(), nAcc, S_IREAD | S_IWRITE));
    //int nFd (open(szUtf8Name, nAcc, S_IREAD | S_IWRITE));
    //qDebug("fd %d %s acc=%d", nFd, szUtf8Name, nAcc);
    return nFd;
}
//ttt0 review O_SHORT_LIVED

wstring wstrFromUtf8(const string& s)
{
    vector<wchar_t> w (s.size() + 1);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], w.size());
    //inspect(&w[0], w.size()*2);
    return &w[0];
}


#endif // #ifndef WIN32 / #else



