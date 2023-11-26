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


#ifndef HelpersH
#define HelpersH


#include  <ios>
#include  <iostream>
#include  <string>
#include  <stdexcept>
#include  <cstdlib> // for exit()
#include  <set>
#include  <vector>

//#include  <QString>
#include  <QStringList>  // ttt2 what we really want is QString; however, by including QString directly, lots of warnings get displayed; perhaps some defines are needed but don't know which; so we just include QStringList to avoid the warnings

class QTableView;

#include  "CbException.h"

////#include  <CbLibCall.h>




#define CB_LIB_CALL

void assertBreakpoint();


/*

Protocol for reading from files:
    - the file is always left in good state, no eof or fail
    - initializing objects with from a file:
        - the starting position is not passed, but it is the current position
        - if an error occurs and the constructor fails, the read pointer is restored (and the error flags are cleared); StreamStateRestorer can be used to do this
        - on success, the read pointer is left after the last byte that was used; (that may be EOF, but the eof flag should still be clear, because no attempt is made to actually read it after moving the pointer)

    - relying on a stream's flags should be avoided; (the flags are cleared most of the time anyway); the test should normally be to check if a desired number of bytes could be read;

*/

// !!! readsome seems to do the job but it only returns whatever is in buffer, without waiting for new data to be brought in. Therefore it is not usable in this case.
// !!! doesn't leave the file with fail or eof set; the caller should check the returned value;
inline std::streamsize read(std::istream& in, char* bfr, std::streamsize nCount)
{
    in.read(bfr, nCount);
    std::streamsize nRes (in.gcount());
    in.clear();

    return nRes;
}



/*
Usage: declare a StreamStateRestorer. Before it goes out of scope, call setOk() if everything went OK. On the destructor it checks if setOk() was called. If it wasn't, the read position is set to whatever was on the constructor. All flags of the stream are cleared in either case.
*/
class StreamStateRestorer
{
    std::istream& m_in;
    std::streampos m_pos;
    bool m_bOk;
public:
    StreamStateRestorer(std::istream& in);
    ~StreamStateRestorer();
    void setOk() { m_bOk = true; }
};



// on its destructor restores the value of the variable passed on the constructor unless setOk() gets called
template <typename T>
class ValueRestorer
{
    T m_val;
    T& m_ref;
    bool m_bRestore;
public:
    ValueRestorer(T& ref) : m_val(ref), m_ref(ref), m_bRestore(true) {}
    ~ValueRestorer()
    {
        if (m_bRestore)
        {
            m_ref = m_val;
        }
    }

    void setOk(bool b = true) { m_bRestore = !b; }
};


template<typename T>
void CB_LIB_CALL releasePtr(T*& p)
{
    delete p;
    p = 0;
}




#define TRACE(A) { std::ostringstream sTrM; sTrM << A; ::trace(sTrM.str()); }
void trace(const std::string& s);

void logAssert(const char* szFile, int nLine, const char* szCond);
void logAssert(const char* szFile, int nLine, const char* szCond, const std::string& strAddtlInfo);


namespace pearl {

template<class T> void CB_LIB_CALL clearPtrContainer(T& c) // calls delete on all the elements of a container of pointers, then sets its size to 0; shouldn't be used on sets
{
    for (typename T::iterator it = c.begin(), end = c.end(); it != end; ++it)
    {
        delete *it;
    }
    c.clear();
}

template<typename T> void CB_LIB_CALL clearPtrContainer(std::set<T*>& c) // specialization for sets
{
    while (!c.empty())
    {
        T* p;
        p = *c.begin(); //ddd see if it's better to remove from end / middle instead of front

        c.erase(c.begin());
        delete p;
    }
}


// to make sure that an array is deallocated even when exceptions are thrown; similar to auto_ptr
template<typename T>
class ArrayPtrRelease
{
    T* m_pArray;

    ArrayPtrRelease();
    ArrayPtrRelease(const ArrayPtrRelease&);
    ArrayPtrRelease& operator=(const ArrayPtrRelease&);

public:
    ArrayPtrRelease(T* pArray) : m_pArray(pArray) {}
    ~ArrayPtrRelease() { delete[] m_pArray; }

    T* get() { return m_pArray; }
    const T* get() const { return m_pArray; }
};

} // namespace pearl


template<class T> int CB_LIB_CALL cSize(const T& c) // returns the size of a container as an int
{
    return (int)c.size();
}



DEFINE_CB_EXCP(EndOfFile);
DEFINE_CB_EXCP(WriteError);

// throws WriteError or EndOfFile
void appendFilePart(std::istream& in, std::ostream& out, std::streampos pos, std::streamoff nSize);


// prints to cout the content of a memory location, as ASCII and hex;
// GDB has a tendency to not see char arrays and other local variables; actually GCC seems to be the culprit (bug 34767);
void inspect(const void* p, int nSize);

template <typename T>
void inspect(const T& x)
{
    if (sizeof(x) == 1001) { return; }
}


// prints the elements of a container
template <typename T>
void printContainer(const T& s, std::ostream& out, const std::string& strSep = " ")
{
    for (typename T::const_iterator it = s.begin(); it != s.end(); ++it)
    {
        out << *it << strSep;
    }
    out << std::endl;
}


std::string asHex(const char* p, int nSize);


inline bool CB_LIB_CALL beginsWith(const std::string& strMain, const std::string& strSubstr)
{
    if (strSubstr.size() > strMain.size()) return false;
    return strMain.substr(0, strSubstr.size()) == strSubstr;
}


inline bool CB_LIB_CALL endsWith(const std::string& strMain, const std::string& strSubstr)
{
    if (strSubstr.size() > strMain.size()) return false; // !!! otherwise the next line might incorrectly return true if
                                                         // strMain.size() = strSubstr.size() - 1

    return strMain.rfind(strSubstr) == strMain.size() - strSubstr.size();
}


bool CB_LIB_CALL rtrim(std::string& s); // removes whitespaces at the end of the string
bool CB_LIB_CALL ltrim(std::string& s); // removes whitespaces at the beginning of the string
bool CB_LIB_CALL trim(std::string& s);


int get32BitBigEndian(const char*);
void put32BitBigEndian(int n, char*);


// The reference that is passed in the constructor (the "guard") should be accessible to all parties interested; it should be initialized to "false" first. When a piece of code wants access to resources protected by the guard, it should declare an NonblockingGuard variable, initialize it with the guard and then check if it went OK, by calling "operator()"; if "operator()" returns true, it means the guard was acquired, so it can proceed.
// The major point is that it's a non-blocking call.
// The intended use is with single-threaded applications (or rather to not share a guard among threads). This is useful to handle the situation where function f() may call g() and g() may call f(), but with the restriction that f() may call g() only if f() wasn't already called by g(). As this isn't designed for multi-threading, it doesn't need any system-specific code or porting.
class NonblockingGuard
{
    bool& m_bLock;
    bool m_bInitialState;

public:
    CB_LIB_CALL NonblockingGuard(bool& bLock) : m_bLock(bLock), m_bInitialState(m_bLock)
    {
        m_bLock = true; // doesn't matter if it was true
    }

    CB_LIB_CALL ~NonblockingGuard()
    {
        m_bLock = m_bInitialState;
    }

    operator bool () const { return !m_bInitialState; }
};


// takes a Latin1 string and converts it to UTF8
std::string utf8FromLatin1(const std::string&);

// the total memory currently used by the current process, in kB
long getMemUsage();

std::string decodeMpegFrame(unsigned int n, const char* szSep, bool* pbIsValid = 0); // on error doesn't throw, but returns an error string; szSep is used as separator for the output string
std::string decodeMpegFrame(const char* bfr, const char* szSep, bool* pbIsValid = 0);
std::string decodeMpegFrameAsXml(const char* bfr, bool* pbIsValid = 0);


inline const char* boolAsYesNo(bool b) { return b ? QT_TRANSLATE_NOOP("GlobalTranslHlp", "yes") : QT_TRANSLATE_NOOP("GlobalTranslHlp", "no"); }

char getPathSep();
const std::string& getPathSepAsStr();

std::streampos getSize(std::istream& in);

// throws WriteError or EndOfFile
void writeZeros(std::ostream& out, int nCnt);


// needed because gdb has trouble setting breakpoints in template code
inline void templateBreakpoint()
{
    qDebug("breakpoint");
}

// by including this as a member the compiler generated constructor / copy op. are disabled
class NoDefaults
{
    CB_LIB_CALL NoDefaults();
    CB_LIB_CALL NoDefaults(const NoDefaults&);
    NoDefaults& CB_LIB_CALL operator=(NoDefaults&); //ttt2 see if there are other generated operations and include them all
public:
    CB_LIB_CALL NoDefaults(int) {}
};


class QWidget;
void listWidget(QWidget* p, int nIndent = 0); //ttt2 move this elsewhere

std::string escapeHttp(const std::string& s); // replaces invalid HTTP characters like ' ' or '"' with their hex code (%20 or %22)

inline QString convStr(const std::string& s) { return QString::fromUtf8(s.c_str(), cSize(s)); } //ttt2 perhaps move
inline std::string convStr(const QString& qs) { const QByteArray& b (qs.toUtf8()); return std::string(b.constData(), b.size()); } //ttt2 perhaps move

std::vector<std::string> convStr(const std::vector<QString>&);
std::vector<QString> convStr(const std::vector<std::string>&);


Qt::WindowFlags getMainWndFlags();   // minimize, maximize, no "what's this"
Qt::WindowFlags getDialogWndFlags(); // maximize, no "what's this"
Qt::WindowFlags getNoResizeWndFlags(); // no "what's this"; the window may be resizable, but the min/max icons aren't shown

bool getDefaultForShowCustomCloseButtons();

QString getSystemInfo();


class QGradient;
class QColor;

// sets colors at various points to emulate a non-linear gradient that better suits our needs;
// dStart and dEnd should be between 0 and 1, with dStart < dEnd; they may also be both -1, in which case the gradient will have a solid color
void configureGradient(QGradient& grad, const QColor& col, double dStart, double dEnd);

// opens a web page from the documentation in the default browser;
// first looks in several places on the local computer; if the file can't be found there, it goes to SourceForge
void openHelp(const std::string& strFileName);


// meant for displaying tooltips; converts some spaces to \n, so the tooltips have several short lines instead of a single wide line
QString makeMultiline(const QString& qstrDescr);

QString toNativeSeparators(const QString&);
QString fromNativeSeparators(const QString&);
inline std::string toNativeSeparators(const std::string& s) { return convStr(toNativeSeparators(convStr(s))); }
inline std::string fromNativeSeparators(const std::string& s) { return convStr(fromNativeSeparators(convStr(s))); }

QString getTempDir();


struct ShellIntegration
{
    static bool isShellIntegrationEditable();
    static std::string getShellIntegrationError();

    static void enableTempSession(bool);
    static bool isTempSessionEnabled();
    static void enableVisibleSession(bool);
    static bool isVisibleSessionEnabled();
    static void enableHiddenSession(bool);
    static bool isHiddenSessionEnabled();
};

//======================================================================================================
//======================================================================================================
//======================================================================================================


void traceToFile(const std::string& s, int nLevelChange);

struct Tracer
{
    const std::string m_s;

    Tracer(const std::string& s);
    ~Tracer();
};

#define TRACER(X) Tracer FiLeTrAcEr (X)
#define TRACER1(X, N) Tracer FiLeTrAcEr##N (X)
#define TRACER1A(X, N) Tracer FiLeTrAcEr##N (X#N)


void traceLastStep(const std::string& s, int nLevelChange);

struct LastStepTracer
{
    const std::string m_s;

    LastStepTracer(const std::string& s);
    ~LastStepTracer();
};


#define LAST_STEP(X) LastStepTracer LaStStEp (X);
#define LAST_STEP1(X, N) LastStepTracer LaStStEp##N (X);



//======================================================================================================
//======================================================================================================
//======================================================================================================


class Timer
{
    int64_t m_nStart;
    int64_t m_nFinish;
    int64_t* m_pStoredDuration;

#ifdef _WIN32
    int64_t m_nDurMul; // multiply by this to get a duration in nanoseconds from a duration in ticks
#endif

public:
    CB_LIB_CALL Timer(bool bStart = true);

    CB_LIB_CALL Timer(int64_t& storedDuration, bool bStart = true);

    ~Timer();

    void CB_LIB_CALL start() { m_nStart = getCrtTime(); }
    int64_t CB_LIB_CALL stop() { m_nFinish = getCrtTime(); return getDuration(); }

    int64_t CB_LIB_CALL getDuration() const { return m_nFinish - m_nStart; } // duration in nanoseconds; may return invalid values if no counter is present or if start/stop haven't been called

    int64_t CB_LIB_CALL fromStart() const { return getCrtTime() - m_nStart; }

    int64_t CB_LIB_CALL getCrtTime() const; // returns time in nanoseconds

    static std::string CB_LIB_CALL addThSep(int64_t nTime); // to be used when converting to milli- / micro- seconds

    std::string CB_LIB_CALL getFmtDuration() const { return addThSep(getDuration()); } // "formatted" duration, in nanoseconds using thousands separator

    std::string CB_LIB_CALL getFmtDuration(int64_t nCnt) const { return addThSep(getDuration()/nCnt); } // "formatted" individual duration for a repeated task, in nanoseconds using thousands separator

    static std::string getLongFmt(int64_t dur);
};


// On Windows with QT5 there is a big padding and the headers are truncated. This resizes the font, so things fit
void decreaseRowHeaderFont(QTableView&);

#endif // ifndef HelpersH

