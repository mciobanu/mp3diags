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


#include  <iostream>
#include  "fstream_unicode.h"
#include  <sstream>
#include  <iomanip>

#include  <boost/version.hpp>

#include  <QDesktopServices>

#ifndef WIN32
    #include  <QDir>
    #include  <sys/utsname.h>
#else
    #include  <windows.h>
    #include  <psapi.h>
    #include  <QSettings>
#endif

#include  <QWidget>
#include  <QUrl>
#include  <QFileInfo>
#include  <QDir>

#include  "Helpers.h"
#include  "Widgets.h"
#include  "Version.h"


using namespace std;


void assertBreakpoint()
{
    cout << endl;
    //qDebug("assert");
}



void appendFilePart(istream& in, ostream& out, streampos pos, streamoff nSize)
{
    const int BFR_SIZE (1024*128);
    char* pBfr (new char[BFR_SIZE]);
    pearl::ArrayPtrRelease<char> rel (pBfr);
    in.seekg(pos);

    for (; nSize > 0;)
    {
        streamoff nCrtRead (nSize > BFR_SIZE ? BFR_SIZE : nSize);
        CB_CHECK1 (nCrtRead == read(in, pBfr, nCrtRead), EndOfFile());
        out.write(pBfr, nCrtRead);

        nSize -= nCrtRead;
    }

    CB_CHECK1 (out, WriteError());
}




// prints to stdout the content of a memory location, as ASCII and hex;
// GDB has a tendency to not see char arrays and other local variables; actually GCC seems to be the culprit (bug 34767);
void inspect(const void* q, int nSize)
{
    ostringstream out;
    const char* p ((const char*)q);
    for (int i = 0; i < nSize; ++i)
    {
        char c (p[i]);
        if (c < 32 || c > 126) { c = '.'; }
        out << c;
    }
    out << "\n(";

    out << hex << setfill('0');
    bool b (false);
    for (int i = 0; i < nSize; ++i)
    {
        if (b) { out << " "; }
        b = true;
        unsigned char c (p[i]);
        out << setw(2) << (int)c;
    }
    out << dec << ")\n";
    qDebug("%s", out.str().c_str());
    //logToGlobalFile(out.str());
}


int get32BitBigEndian(const char* bfr)
{
    const unsigned char* p (reinterpret_cast<const unsigned char*>(bfr));
    int n ((p[0] << 24) + (p[1] << 16) + (p[2] << 8) + (p[3] << 0));
    return n;
}


void put32BitBigEndian(int n, char* bfr)
{
    unsigned u (n);
    bfr[3] = u & 0xff; u >>= 8;
    bfr[2] = u & 0xff; u >>= 8;
    bfr[1] = u & 0xff; u >>= 8;
    bfr[0] = u & 0xff;
}



string utf8FromLatin1(const string& strSrc)
{
    int i (0);
    int n (cSize(strSrc));

    for (; i < n; ++i)
    {
        unsigned char c (strSrc[i]);
        if (c >= 128) { goto e1; }
    }
    return strSrc;

e1:
    string s (strSrc.substr(0, i));
    for (; i < n; ++i)
    {
        unsigned char c (strSrc[i]);
        if (c < 128)
        {
            s += char(c);
        }
        else
        {
            unsigned char c1 (0xc0 | (c >> 6));
            unsigned char c2 (0x80 | (c & 0x3f));
            s += char(c1);
            s += char(c2);
        }
    }
    return s;
}


// removes whitespaces at the end of the string
bool CB_LIB_CALL rtrim(string& s)
{
    int n (cSize(s));
    int i (n - 1);
    for (; i >= 0; --i)
    {
        unsigned char c (s[i]);
        if (c >= 128 || !isspace(c)) { break; } //!!! isspace() returns true for some non-ASCII chars, e.g. 0x9f, at least on MinGW (it matters that char is signed)
    }

    if (i < n - 1)
    {
        s.erase(i + 1);
        return true;
    }
    return false;
}


// removes whitespaces at the beginning of the string
bool CB_LIB_CALL ltrim(string& s)
{
    int n (cSize(s));
    int i (0);
    for (; i < n; ++i)
    {
        unsigned char c (s[i]);
        if (c >= 128 || !isspace(c)) { break; } //!!! isspace() returns true for some non-ASCII chars, e.g. 0x9f, at least on MinGW
    }

    if (i > 0)
    {
        s.erase(0, i);
        return true;
    }
    return false;
}

bool CB_LIB_CALL trim(string& s)
{
    bool b1 (ltrim(s));
    bool b2 (rtrim(s));
    return b1 || b2;
}



/*
// multi-line hex printing
void printHex(const string& s, ostream& out, bool bShowAsciiCode = true) //ttt3 see if anybody needs this
{
    int nSize (cSize(s));
    int nCrt (0);

    for (;;)
    {
        if (nCrt >= nSize) { return; }
        int nMax (16);
        if (nCrt + nMax > nSize)
        {
            nMax = nSize - nCrt;
        }

        for (int i = 0; i < nMax; ++i)
        {
            char c (s[i + nCrt]);
            if (c < 32 || c >= 127) { c = '?'; }
            out << " " << c << " ";
        }
        out << endl;

        for (int i = 0; i < nMax; ++i)
        {
            unsigned int x ((unsigned char)s[i + nCrt]);
            if (!bShowAsciiCode && x >= 32 && x < 127)
            {
                out << "   ";
            }
            else
            {
                out << setw(2) << hex << x << dec << " ";
            }
        }
        out << endl;
        nCrt += 16;
    }
}
*/



std::string asHex(const char* p, int nSize)
{
    ostringstream out;
    out << "\"";
    for (int i = 0; i < nSize; ++i)
    {
        char c (p[i]);
        out << (c >= 32 && c < 127 ? c : '.');
    }
    out << "\" (" << hex;
    for (int i = 0; i < nSize; ++i)
    {
        if (i > 0) { out << " "; }
        unsigned char c (p[i]);
        out << setw(2) << setfill('0') << (int)c;
    }
    out << ")";
    return out.str();
}




// the total memory currently used by the current process, in kB
long getMemUsage()
{
#ifndef WIN32
    //pid_t
    int n ((int)getpid());
    char a [30];
    sprintf (a, "/proc/%d/status", n); // ttt2 linux-specific; not sure how version-specific this is;
    // sprintf (a, "/proc/self/status", n); // ttt2 try this (after checking portability)
    ifstream_utf8 in (a);
    string s;
    while (getline(in, s))
    {
        if (0 == s.find("VmSize:"))
        {
            return atol(s.c_str() + 7);
        }
    }
    return 0;
#else
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        return pmc.WorkingSetSize;
    }

    return 0;
#endif

}


void logToGlobalFile(const string& s) //tttc make sure it is disabled in public releases
{
#ifndef WIN32
    ofstream_utf8 out (
            "/tmp/Mp3DiagsLog.txt",
            ios_base::app);
#else
    char a [500];
    int n (GetModuleFileNameA(NULL, a, 500)); //ttt3 using GetModuleFileNameA isn't quite right, but since it's a debug function ...
    a[n - 4] = 0;
    ofstream_utf8 out (
            //"C:/Mp3DiagsLog.txt",
            //"Mp3DiagsLog.txt",
            //"C:/temp/Mp3DiagsLog.txt",
            (string(a) + "Log.txt").c_str(),
            ios_base::app);
#endif
    out << s << endl;
}






















#define DECODE_CHECK(COND, MSG) { if (!(COND)) { bRes = false; return MSG; } }

namespace
{

    struct Decoder
    {
        enum Version { MPEG1, MPEG2 };
        enum Layer { LAYER1, LAYER2, LAYER3 };
        enum ChannelMode { STEREO, JOINT_STEREO, DUAL_CHANNEL, SINGLE_CHANNEL };

        Version m_eVersion;
        Layer m_eLayer;
        int m_nBitrate;
        int m_nFrequency;
        int m_nPadding;
        ChannelMode m_eChannelMode;

        int m_nSize;
        bool m_bCrc;

        const char* getSzVersion() const;
        const char* getSzLayer() const;
        const char* getSzChannelMode() const;

        string initialize(const unsigned char* bfr, bool* pbIsValid);
        string decodeMpegFrame(const unsigned char* bfr, const char* szSep, bool* pbIsValid);
        string decodeMpegFrameAsXml(const unsigned char* bfr, bool* pbIsValid);
    };

    const char* Decoder::getSzVersion() const
    {
        static const char* s_versionName[] = { "MPEG-1", "MPEG-2" };
        return s_versionName[m_eVersion];
    }

    const char* Decoder::getSzLayer() const
    {
        static const char* s_layerName[] = { "Layer I", "Layer II", "Layer III" };
        return s_layerName[m_eLayer];
    }

    const char* Decoder::getSzChannelMode() const
    {
        static const char* s_channelModeName[] = { "Stereo", "Joint stereo", "Dual channel", "Single channel" };
        return s_channelModeName[m_eChannelMode];
    }


    string Decoder::initialize(const unsigned char* bfr, bool* pbIsValid) //ttt2 perhaps unify with MpegFrameBase::MpegFrameBase(), using the char* constructor
    {
        bool b;
        bool& bRes (0 == pbIsValid ? b : *pbIsValid);
        bRes = true;
        const unsigned char* pHeader (bfr);
    //inspect(bfr, BFR_SIZE);
        DECODE_CHECK (0xff == *pHeader && 0xe0 == (0xe0 & *(pHeader + 1)), "Not an MPEG frame. Synch missing.");
        ++pHeader;

        {
            int nVer ((*pHeader & 0x18) >> 3);
            switch (nVer)
            {//TRACE
            case 0x00: bRes = false; return "Not an MPEG frame. Unsupported version (2.5)."; //ttt2 see about supporting this: search for MPEG1 to find other places
                // ttt2 in a way it would make more sense to warn that it's not supported, with "MP3_THROW(SUPPORT, ...)", but before warn, make sure it's a valid 2.5 frame, followed by another frame ...

            case 0x02: m_eVersion = MPEG2; break;
            case 0x03: m_eVersion = MPEG1; break;

            default: bRes = false; return "Not an MPEG frame. Invalid version.";
            }
        }

        {
            int nLayer ((*pHeader & 0x06) >> 1);
            switch (nLayer)
            {
            case 0x01: m_eLayer = LAYER3; break;
            case 0x02: m_eLayer = LAYER2; break;
            case 0x03: m_eLayer = LAYER1; break;

            default: bRes = false; return "Not an MPEG frame. Invalid layer.";
            }
        }

        {
            m_bCrc = !(*pHeader & 0x01);
        }

        ++pHeader;
        {
            static int s_bitrates [14][5] =
                {
                    {  32,      32,      32,      32,       8 },
                    {  64,      48,      40,      48,      16 },
                    {  96,      56,      48,      56,      24 },
                    { 128,      64,      56,      64,      32 },
                    { 160,      80,      64,      80,      40 },
                    { 192,      96,      80,      96,      48 },
                    { 224,     112,      96,     112,      56 },
                    { 256,     128,     112,     128,      64 },
                    { 288,     160,     128,     144,      80 },
                    { 320,     192,     160,     160,      96 },
                    { 352,     224,     192,     176,     112 },
                    { 384,     256,     224,     192,     128 },
                    { 416,     320,     256,     224,     144 },
                    { 448,     384,     320,     256,     160 }
                };
            int nRateIndex ((*pHeader & 0xf0) >> 4);
            DECODE_CHECK (nRateIndex >= 1 && nRateIndex <= 14, "Not an MPEG frame. Invalid bitrate.");
            int nTypeIndex (m_eVersion*3 + m_eLayer);
            if (nTypeIndex == 5) { nTypeIndex = 4; }
            m_nBitrate = s_bitrates[nRateIndex - 1][nTypeIndex]*1000;
        }

        {
            int nSmpl ((*pHeader & 0x0c) >> 2);
            switch (m_eVersion)
            {
            case MPEG1:
                switch (nSmpl)
                {
                case 0x00: m_nFrequency = 44100; break;
                case 0x01: m_nFrequency = 48000; break;
                case 0x02: m_nFrequency = 32000; break;

                default: bRes = false; return "Not an MPEG frame. Invalid frequency for MPEG1.";
                }
                break;

            case MPEG2:
                switch (nSmpl)
                {
                case 0x00: m_nFrequency = 22050; break;
                case 0x01: m_nFrequency = 24000; break;
                case 0x02: m_nFrequency = 16000; break;

                default: bRes = false; return "Not an MPEG frame. Invalid frequency for MPEG2.";
                }
                break;

            default: throw 1; // it should have thrown before getting here
            }
        }

        {
            m_nPadding = (0x02 & *pHeader) >> 1;
        }

        ++pHeader;
        {
            int nChMode ((*pHeader & 0xc0) >> 6);
            m_eChannelMode = (ChannelMode)nChMode;
        }

        switch (m_eLayer)
        {
        case LAYER1:
            m_nSize = (12*m_nBitrate/m_nFrequency + m_nPadding)*4;
            break;

        case LAYER2:
            m_nSize = 144*m_nBitrate/m_nFrequency + m_nPadding;
            break;

        case LAYER3:
            m_nSize = (MPEG1 == m_eVersion ? 144*m_nBitrate/m_nFrequency + m_nPadding : 72*m_nBitrate/m_nFrequency + m_nPadding);
            break;

        default: throw 1; // it should have thrown before getting here
        }

        return "";
    }


    string Decoder::decodeMpegFrame(const unsigned char* bfr, const char* szSep, bool* pbIsValid) //ttt2 perhaps unify with MpegFrameBase::MpegFrameBase(), using the char* constructor
    {
        string s (initialize(bfr, pbIsValid));
        if (!s.empty()) { return s; }

        ostringstream out;
        /*out << getSzVersion() << " " << getSzLayer() << ", " << m_nBitrate/1000 << "kbps, " << m_nFrequency << "Hz, " << getSzChannelMode() << ", padding=" << (m_nPadding ? "true" : "false") << ", length " <<
                m_nSize << " (0x" << hex << m_nSize << dec << ")";*/
        out << boolalpha << getSzVersion() << " " << getSzLayer() << szSep << getSzChannelMode() <<
                szSep << m_nFrequency << "Hz" << szSep << m_nBitrate << "bps" << szSep << "CRC=" << boolAsYesNo(m_bCrc) << szSep <<
                "length " << m_nSize << " (0x" << hex << m_nSize << dec << ")" << szSep << "padding=" << (m_nPadding ? "true" : "false");

        return out.str();
    }


    string Decoder::decodeMpegFrameAsXml(const unsigned char* bfr, bool* pbIsValid) //ttt2 perhaps unify with MpegFrameBase::MpegFrameBase(), using the char* constructor
    {
        string s (initialize(bfr, pbIsValid));
        if (!s.empty()) { return s; }

        ostringstream out;
        out << " version=\"" << getSzVersion() << "\""
            << " layer=\"" << getSzLayer() << "\""
            << " channelMode=\"" << getSzChannelMode() << "\""
            << " frequency=\"" << m_nFrequency << "\""
            << " bps=\"" << m_nBitrate << "\""
            << " crc=\"" << boolAsYesNo(m_bCrc) << "\""

            << " mpegSize=\"" << m_nSize << "\""
            << " padding=\"" << boolAsYesNo(m_nPadding) << "\"";

        return out.str();
    }
} // namespace



string decodeMpegFrame(unsigned int x, const char* szSep, bool* pbIsValid /*= 0*/)
{
    Decoder d;
    unsigned char bfr [4];
    unsigned char* q (reinterpret_cast<unsigned char*>(&x));
    bfr[0] = q[3]; bfr[1] = q[2]; bfr[2] = q[1]; bfr[3] = q[0];

    return d.decodeMpegFrame(bfr, szSep, pbIsValid);
}


string decodeMpegFrame(const char* bfr, const char* szSep, bool* pbIsValid /*= 0*/)
{
    Decoder d;
    const unsigned char* q (reinterpret_cast<const unsigned char*>(bfr));
    return d.decodeMpegFrame(q, szSep, pbIsValid);
}


string decodeMpegFrameAsXml(const char* bfr, bool* pbIsValid /*= 0*/)
{
    Decoder d;
    const unsigned char* q (reinterpret_cast<const unsigned char*>(bfr));
    return d.decodeMpegFrameAsXml(q, pbIsValid);
}


StreamStateRestorer::StreamStateRestorer(istream& in) : m_in(in), m_pos(in.tellg()), m_bOk(false)
{
}


StreamStateRestorer::~StreamStateRestorer()
{
    if (!m_bOk)
    {
        m_in.clear();
        m_in.seekg(m_pos);
    }
    m_in.clear();
}

char getPathSep()
{
    return '/'; // ttt2 linux-specific
}

const string& getPathSepAsStr()
{
    static string s ("/");
    return s; // ttt2 linux-specific // ttt look at QDir::fromNativeSeparators
}




streampos getSize(istream& in)
{
    streampos crt (in.tellg());
    in.seekg(0, ios_base::end);
    streampos size (in.tellg());
    in.seekg(crt);
    return size;
}



void writeZeros(ostream& out, int nCnt)
{
    char c (0);
    for (int i = 0; i < nCnt; ++i) //ttt2 perhaps make this faster
    {
        out.write(&c, 1);
    }

    CB_CHECK1 (out, WriteError());
}


void listWidget(QWidget* p, int nIndent /*= 0*/)
{
    //if (nIndent > 1) { return; }
    if (0 == nIndent) { cout << "\n----------------------------\n"; }
    cout << string(nIndent*2, ' ') << convStr(p->objectName()) << " " << p->x() << " " << p->y() << " " << p->width() << " " << p->height() << endl;
    QList<QWidget*> lst (p->findChildren<QWidget*>());
    for (QList<QWidget*>::iterator it = lst.begin(); it != lst.end(); ++it)
    {
        QWidget* q (*it);
        if (q->parentWidget() == p) // !!! needed because findChildren() reurns all descendants, not only children
        {
            listWidget(q, nIndent + 1);
        }
    }
}


// replaces invalid HTTP characters like ' ' or '"' with their hex code (%20 or %22)
string escapeHttp(const string& s)
{
    QUrl url (convStr(s));
    return convStr(QString(url.toEncoded()));
}




vector<string> convStr(const vector<QString>& v)
{
    vector<string> u;
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        u.push_back(convStr(v[i]));
    }
    return u;
}

vector<QString> convStr(const vector<string>& v)
{
    vector<QString> u;
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        u.push_back(convStr(v[i]));
    }
    return u;
}

/*
Gnome:

Qt::Window - minimize, maximize, close; dialog gets its own taskbar entry
Qt::WindowTitleHint - close
Qt::Dialog - close
Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint - nothing
Qt::Dialog | Qt::WindowMaximizeButtonHint - nothing
Qt::Window | Qt::WindowMaximizeButtonHint - maximize
Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint - maximize, minimize
Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint - nothing

Ideally a modal dialog should minimize its parent. If that's not possible, it shouldn't be minimizable.
*/

#ifndef WIN32
    Qt::WindowFlags getMainWndFlags() { return Qt::WindowTitleHint; } // !!! these are incorrect, but seem the best option; the values used for Windows are supposed to be OK; they work as expected with KDE but not with Gnome (asking for maximize button not only fails to sho it, but causes the "Close" button to disappear as well); Since in KDE min/max buttons are shown when needed anyway, it's sort of OK // ttt2 see if there is workaround/fix
    Qt::WindowFlags getDialogWndFlags() { return Qt::WindowTitleHint; }
    Qt::WindowFlags getNoResizeWndFlags() { return Qt::WindowTitleHint; }
#else
    Qt::WindowFlags getMainWndFlags() { return Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint; } // minimize, maximize, no "what's this"
    Qt::WindowFlags getDialogWndFlags() { return Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint; } // minimize, no "what's this"
    Qt::WindowFlags getNoResizeWndFlags() { return Qt::WindowTitleHint; } // no "what's this"
#endif


#if 0
//ttt2 add desktop, distribution, WM, ...
#ifndef WIN32
    utsname info;
    uname(&info);

    cat /proc/version
    cat /etc/issue
    dmesg | grep "Linux version"
    cat /etc/*-release
*/
    cat /etc/*_version

*/
#else
//
#endif
#endif

#ifndef WIN32
static void removeStr(string& main, const string& sub)
{
    for (;;)
    {
        string::size_type n (main.find(sub));
        if (string::npos == n) { return; }
        main.erase(n, sub.size());
    }
}
#endif


QString getSystemInfo() //ttt2 perhaps store this at startup, so fewer things may go wrong fhen the assertion handler needs it
{
    QString s ("OS: ");

#ifndef WIN32
    QDir dir ("/etc");

    QStringList filters;
    filters << "*-release" << "*_version";
    dir.setNameFilters(filters);
    QStringList lFiles (dir.entryList(QDir::Files));
    utsname utsInfo;
    uname(&utsInfo);
    s += utsInfo.sysname; s += " ";
    s += utsInfo.release; s += " ";
    s += utsInfo.version; s += " ";
    for (int i = 0; i < lFiles.size(); ++i)
    {
        //qDebug("%s", lFiles[i].toUtf8().constData());
        if ("lsb-release" != lFiles[i])
        {
            QFile f ("/etc/" + lFiles[i]);
            if (f.open(QIODevice::ReadOnly))
            {
                QByteArray b (f.read(1000));
                s += b;
                s += " ";
            }
        }
    }

    QFile f ("/etc/issue");
    if (f.open(QIODevice::ReadOnly))
    {
        QByteArray b (f.read(1000));
        string s1 (b.constData());

        removeStr(s1, "Welcome to");
        removeStr(s1, "Kernel");
        trim(s1);

        string::size_type n (s1.find('\\'));
        if (string::npos != n)
        {
            s1.erase(n);
        }
        trim(s1);

        if (endsWith(s1, "-"))
        {
            s1.erase(s1.size() - 1);
            trim(s1);
        }

        s += convStr(s1);
//qDebug("a: %s", s.toUtf8().constData());
        /*for (;;)
        {
            string::size_type n (s.find('\n'));
            if (string::npos == n) { break; }
            s1[n] = ' ';
        }*/
//qDebug("b: %s", s.toUtf8().constData());
    }
//ttt2 search /proc for kwin, metacity, ...


#else
    //qstrVer += QString(" Windows version ID: %1").arg(QSysInfo::WinVersion);
    QSettings settings ("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", QSettings::NativeFormat);
    //qstrVer += QString(" Windows version: %1").arg(WIN32);
    s += QString(" Windows version: %1 %2 Build %3 %4").arg(settings.value("ProductName").toString())
               .arg(settings.value("CSDVersion").toString())
               .arg(settings.value("CurrentBuildNumber").toString())
               .arg(settings.value("BuildLab").toString());

#endif
    s.replace('\n', ' ');
    s = QString("Version: %1 %2.\nWord size: %3 bit.\nQt version: %4.\nBoost version: %5\n").arg(APP_NAME).arg(APP_VER).arg(QSysInfo::WordSize).arg(qVersion()).arg(BOOST_LIB_VERSION) + s;
    return s;
}


// sets colors at various points to emulate a non-linear gradient that better suits our needs;
// dStart and dEnd should be between 0 and 1, with dStart < dEnd; they may also be both -1, in which case the gradient will have a solid color
void configureGradient(QGradient& grad, const QColor& col, double dStart, double dEnd)
{
    if (-1 == dStart && -1 == dEnd)
    {
        grad.setColorAt(0, col);
        grad.setColorAt(1, col);
        return;
    }

    CB_ASSERT (dStart < dEnd && 0 <= dStart && dEnd < 1.0001);
    static vector<double> vdPoints;
    static vector<double> vdValues;
    static bool s_bInit (false);
    static int SIZE;
    if (!s_bInit)
    {
        s_bInit = true;
/*        vdPoints.push_back(-0.1); vdValues.push_back(1.3);
        vdPoints.push_back(0.0); vdValues.push_back(1.3);
        vdPoints.push_back(0.1); vdValues.push_back(1.2);
        vdPoints.push_back(0.2); vdValues.push_back(1.15);
        vdPoints.push_back(0.8); vdValues.push_back(0.85);
        vdPoints.push_back(0.9); vdValues.push_back(0.8);
        vdPoints.push_back(1.0); vdValues.push_back(0.7);
        vdPoints.push_back(1.1); vdValues.push_back(0.7);*/

        /*vdPoints.push_back(-0.1); vdValues.push_back(1.1);
        vdPoints.push_back(0.0); vdValues.push_back(1.1);
        vdPoints.push_back(0.1); vdValues.push_back(1.07);
        vdPoints.push_back(0.2); vdValues.push_back(1.03);
        vdPoints.push_back(0.8); vdValues.push_back(0.97);
        vdPoints.push_back(0.9); vdValues.push_back(0.93);
        vdPoints.push_back(1.0); vdValues.push_back(0.9);
        vdPoints.push_back(1.1); vdValues.push_back(0.9);*/

        /*vdPoints.push_back(-0.1); vdValues.push_back(1.3);
        vdPoints.push_back(0.0); vdValues.push_back(1.3);
        vdPoints.push_back(0.1); vdValues.push_back(1.15);
        vdPoints.push_back(0.2); vdValues.push_back(1.03);
        vdPoints.push_back(0.8); vdValues.push_back(0.95);
        vdPoints.push_back(0.9); vdValues.push_back(0.9);
        vdPoints.push_back(1.0); vdValues.push_back(0.8);
        vdPoints.push_back(1.1); vdValues.push_back(0.8);*/

        vdPoints.push_back(-0.1); vdValues.push_back(1.03);
        vdPoints.push_back(0.0); vdValues.push_back(1.03);
        vdPoints.push_back(0.1); vdValues.push_back(1.02);
        vdPoints.push_back(0.2); vdValues.push_back(1.01);
        vdPoints.push_back(0.8); vdValues.push_back(0.95);
        vdPoints.push_back(0.9); vdValues.push_back(0.9);
        vdPoints.push_back(1.0); vdValues.push_back(0.8);
        vdPoints.push_back(1.1); vdValues.push_back(0.8);

        SIZE = cSize(vdPoints);

//findFont();
    }

#if 1
    for (int i = 0; i < SIZE; ++i)
    {
        double x0 (vdPoints[i]), y0 (vdValues[i]), x1 (vdPoints[i + 1]), y1 (vdValues[i + 1]);
        double x;
        x = dStart;
        if (x0 <= x && x < x1)
        {
            double y (y0 + (y1 - y0)*(x - x0)/(x1 - x0));
            grad.setColorAt((x - dStart)/(dEnd - dStart), col.lighter(int(100*y)));
        }

        if (dStart < x0 && x0 < dEnd)
        {
            grad.setColorAt((x0 - dStart)/(dEnd - dStart), col.lighter(int(100*y0)));
        }

        x = dEnd;
        if (x < x1)
        {
            double y (y0 + (y1 - y0)*(x - x0)/(x1 - x0));
            grad.setColorAt((x - dStart)/(dEnd - dStart), col.lighter(int(100*y)));
            break;
        }
    }
#else
    grad.setColorAt(0, col.lighter(dStart < 0.0001 ? 119 : 100)); //ttt2 perhaps use this or at least add an option
    grad.setColorAt(0.48, col);
    grad.setColorAt(0.52, col);
    grad.setColorAt(1, col.lighter(dEnd > 0.9999 ? 80 : 100));
#endif
}

vector<QString> getLocalHelpDirs()
{
    static vector<QString> s_v;
    if (s_v.empty())
    {
#ifndef WIN32
        //s_v.push_back("/home/ciobi/cpp/Mp3Utils/mp3diags/trunk/mp3diags/doc/html/");
        s_v.push_back("/usr/share/mp3diags-doc/html/");
        s_v.push_back("/usr/share/doc/mp3diags/html/");
        s_v.push_back("/usr/share/doc/mp3diags-QQQVERQQQ/html/");
#else
        wchar_t wszModule [200];
        int nRes (GetModuleFileName(0, wszModule, 200));
        //qDebug("%s", QString::fromWCharArray(wszModule).toUtf8().constData());
        if (0 < nRes && nRes < 200)
        {
            s_v.push_back(QFileInfo(
                    fromNativeSeparators(QString::fromWCharArray(wszModule))).dir().absolutePath() + "/doc/");
            //qDebug("%s", s_v.back().toUtf8().constData());
        }
#endif
    }

    return s_v;
}


// opens a web page from the documentation in the default browser;
// first looks in several places on the local computer; if the file can't be found there, it goes to SourceForge
void openHelp(const string& strFileName)
{
    const vector<QString>& v (getLocalHelpDirs());
    QString strDir;
    for (int i = 0; i < cSize(v); ++i)
    {
        if (QFileInfo(v[i] + convStr(strFileName)).isFile())
        {
            strDir = v[i];
            break;
        }
    }

    QString qs (strDir);
    if (qs.isEmpty())
    {
        qs = "http://mp3diags.sourceforge.net" + QString(APP_BRANCH) + "/";
    }
    else
    {
        qs = QUrl::fromLocalFile(qs).toString();
    }

    qs = qs + convStr(strFileName);


//qDebug("open %s", qs.toUtf8().constData());
//logToGlobalFile(qs.toUtf8().constData());
    CursorOverrider ovr;
    QDesktopServices::openUrl(QUrl(qs, QUrl::TolerantMode));
}


// meant for displaying tooltips; converts some spaces to \n, so the tooltips have several short lines instead of a single wide line
QString makeMultiline(const char* szDescr) //ttt2 param should probably be changed for localized versions, so this takes and returns QString
{
    QString s (szDescr);
    int SIZE (50);
    for (int i = SIZE; i < s.size(); ++i)
    {
        if (' ' == s[i])
        {
            s[i] = '\n';
            i += SIZE;
        }
    }
    return s;
}


QString toNativeSeparators(const QString& s)
{
    return QDir::toNativeSeparators(s);
}

QString fromNativeSeparators(const QString& s)
{
    return QDir::fromNativeSeparators(s);
}

QString getTempDir()
{
/*
#ifndef WIN32
    return "/tmp";
#else
    wchar_t wszTmp [200];
    if (GetTempPath(200, wszTmp) > 1999) { return ""; }
    return QString::fromWCharArray(wszTmp);
#endif*/

    static QString s; // ttt3 these static variables are not really thread safe, but in this case it doesn't matter, because they all get called from a single thread (the UI thread)
    if (s.isEmpty())
    {
        s = QDir::tempPath();
        if (s.endsWith(getPathSep()))
        {
            s.remove(s.size() - 1, 1);
        }
    }
    return s;
}


//=============================================================================================
//=============================================================================================
//=============================================================================================

Tracer::Tracer(const std::string& s) : m_s(s)
{
    traceToFile("> " + s, 1);
}

Tracer::~Tracer()
{
    traceToFile(" < " + m_s, -1);
}


//=============================================================================================

LastStepTracer::LastStepTracer(const std::string& s) : m_s(s)
{
    traceLastStep("> " + s, 1);
}

LastStepTracer::~LastStepTracer()
{
    traceLastStep(" < " + m_s, -1);
}

//=============================================================================================
//=============================================================================================
//=============================================================================================


//ttt2 F1 help was very slow on XP once, not sure why; later it was OK

