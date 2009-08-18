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

//#include  <QSettings> //ttt
//#include  <iostream>

#include  "fstream_unicode.h"

#include  <sys/stat.h>


// might want to look at basic_file_stdio.cc or ext/stdio_filebuf.h

// http://www2.roguewave.com/support/docs/leif/sourcepro/html/stdlibref/basic-ifstream.html
// STLPort
// MSVC
// Dinkumware - not

// http://www.aoc.nrao.edu/~tjuerges/ALMA/STL/html/class____gnu__cxx_1_1stdio__filebuf.html




//#include  "Helpers.h" //ttt remove

//ttt1 perhaps add flush()


static int getAcc(std::ios_base::openmode __mode)
{
    int nAcc;
    if (std::ios_base::in & __mode)
    {
        if (std::ios_base::out & __mode)
        {
            nAcc = O_RDWR | O_CREAT;
        }
        else
        {
            nAcc = O_RDONLY;
        }
    }
    else if (std::ios_base::out & __mode)
    {
        nAcc = O_RDWR | O_CREAT;
    }
    else
    {
        throw 1; // ttt1
    }

    #ifdef O_LARGEFILE
    nAcc |= O_LARGEFILE;
    #endif

    if (std::ios_base::trunc & __mode)
    {
        nAcc |= O_TRUNC;
    }

    if (std::ios_base::app & __mode)
    {
        nAcc |= O_APPEND;
    }

    #ifdef O_BINARY
    if (std::ios_base::binary & __mode)
    {
        nAcc |= O_BINARY;
    }
    #endif

    return nAcc;
}


#ifndef WIN32

    template<>
    int unicodeOpenHlp(const char* szUtf8Name, std::ios_base::openmode __mode)
    {
        int nAcc (getAcc(__mode));
        int nFd (open(szUtf8Name, nAcc, S_IREAD | S_IWRITE));
        //qDebug("fd %d %s acc=%d", nFd, szUtf8Name, nAcc);
        return nFd;
    }

#if 0
    template<>
    int unicodeOpenHlp(const wchar_t* /*wszUtf16Name*/, std::ios_base::openmode /*__mode*/)
    {
        throw 1; //ttt2 add if needed
    }
#endif

#else


    #include  <windows.h>

    #include  <vector>
    #include  <string>

    using namespace std;
    using namespace __gnu_cxx;


    wstring wstrFromUtf8(const string& s)
    {
        vector<wchar_t> w (s.size() + 1);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], w.size());
        //inspect(&w[0], w.size()*2);
        return &w[0];
    }


    template<>
    int unicodeOpenHlp(const wchar_t* wszUtf16Name, std::ios_base::openmode __mode)
    {
        int nAcc (getAcc(__mode));

        int nFd (_wopen(wszUtf16Name, nAcc, S_IREAD | S_IWRITE));
        //int nFd (open(szUtf8Name, nAcc, S_IREAD | S_IWRITE));
        //qDebug("fd %d %s acc=%d", nFd, szUtf8Name, nAcc);
        return nFd;
    }

    template<>
    int unicodeOpenHlp(const char* szUtf8Name, std::ios_base::openmode __mode)
    {
        return unicodeOpenHlp(wstrFromUtf8(szUtf8Name).c_str(), __mode);
    }

#endif // #ifndef WIN32 / #else


template<>
int unicodeOpenHlp(char* szUtf8Name, std::ios_base::openmode __mode)
{
    return unicodeOpenHlp<const char*>(szUtf8Name, __mode);
}


#ifndef WIN32 // ttt2 remove conditional if needed

#else

template<>
int unicodeOpenHlp(wchar_t* wszUtf16Name, std::ios_base::openmode __mode)
{
    return unicodeOpenHlp<const wchar_t*>(wszUtf16Name, __mode);
}

#endif


template<>
int unicodeOpenHlp(int fd, std::ios_base::openmode /*__mode*/)
{
    return fd;
}




//ttt1 review O_SHORT_LIVED

