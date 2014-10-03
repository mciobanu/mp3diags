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
#include  <cstdio>

#include  <boost/version.hpp>

#include  <QDesktopServices>

#ifndef WIN32
    #include  <QDir>
    #include  <sys/utsname.h>
    #include  <unistd.h>
#else
    #include  <windows.h>
    #include  <psapi.h>
    #include  <QSettings>
#endif

#include  <QWidget>
#include  <QUrl>
#include  <QFileInfo>
#include  <QDir>
#include  <QSettings>
#include  <QProcess>

#include  "Helpers.h"
#include  "Widgets.h"
#include  "Version.h"
#include  "OsFile.h"

#include  "DataStream.h" // for translation


using namespace std;
using namespace Version;


void assertBreakpoint()
{
    cout << endl;
    //qDebug("assert");
}

CbException::CbException(std::string strMsg, const char* szFile, int nLine) {
    vector<char> a (strMsg.size() + strlen(szFile) + 20);
    sprintf(&a[0], "%s [%s/%d]", strMsg.c_str(), szFile, nLine);
    m_strMsg = &a[0];
}

CbException::CbException(std::string strMsg, const char* szFile, int nLine, const CbException& cause) {
    vector<char> a (strMsg.size() + strlen(szFile) + cause.m_strMsg.length() + 40);
    sprintf(&a[0], "%s [%s/%d] / Caused by: %s", cause.m_strMsg.c_str(), szFile, nLine, cause.what());
    m_strMsg = &a[0];
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

    if (!out)
    {
        TRACER("appendFilePart() failed");
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

        QString initialize(const unsigned char* bfr, bool* pbIsValid);
        QString decodeMpegFrame(const unsigned char* bfr, const char* szSep, bool* pbIsValid);
        string decodeMpegFrameAsXml(const unsigned char* bfr, bool* pbIsValid);
    };

    const char* Decoder::getSzVersion() const
    {
        static const char* s_versionName[] = { QT_TRANSLATE_NOOP("DataStream", "MPEG-1"), QT_TRANSLATE_NOOP("DataStream", "MPEG-2") };
        return s_versionName[m_eVersion];
    }

    const char* Decoder::getSzLayer() const
    {
        static const char* s_layerName[] = { QT_TRANSLATE_NOOP("DataStream", "Layer I"), QT_TRANSLATE_NOOP("DataStream", "Layer II"), QT_TRANSLATE_NOOP("DataStream", "Layer III") };
        return s_layerName[m_eLayer];
    }

    const char* Decoder::getSzChannelMode() const
    {
        static const char* s_channelModeName[] = { QT_TRANSLATE_NOOP("DataStream", "Stereo"), QT_TRANSLATE_NOOP("DataStream", "Joint stereo"), QT_TRANSLATE_NOOP("DataStream", "Dual channel"), QT_TRANSLATE_NOOP("DataStream", "Single channel") };
        return s_channelModeName[m_eChannelMode];
    }


    QString Decoder::initialize(const unsigned char* bfr, bool* pbIsValid) //ttt2 perhaps unify with MpegFrameBase::MpegFrameBase(), using the char* constructor; note that they also share translations
    {
        bool b;
        bool& bRes (0 == pbIsValid ? b : *pbIsValid);
        bRes = true;
        const unsigned char* pHeader (bfr);
    //inspect(bfr, BFR_SIZE);
        DECODE_CHECK (0xff == *pHeader && 0xe0 == (0xe0 & *(pHeader + 1)), DataStream::tr("Not an MPEG frame. Synch missing."));
        ++pHeader;

        {
            int nVer ((*pHeader & 0x18) >> 3);
            switch (nVer)
            {//TRACE
            case 0x00: bRes = false; return DataStream::tr("Not an MPEG frame. Unsupported version (2.5)."); //ttt2 see about supporting this: search for MPEG1 to find other places
                // ttt2 in a way it would make more sense to warn that it's not supported, with "MP3_THROW(SUPPORT, ...)", but before warn, make sure it's a valid 2.5 frame, followed by another frame ...

            case 0x02: m_eVersion = MPEG2; break;
            case 0x03: m_eVersion = MPEG1; break;

            default: bRes = false; return DataStream::tr("Not an MPEG frame. Invalid version.");
            }
        }

        {
            int nLayer ((*pHeader & 0x06) >> 1);
            switch (nLayer)
            {
            case 0x01: m_eLayer = LAYER3; break;
            case 0x02: m_eLayer = LAYER2; break;
            case 0x03: m_eLayer = LAYER1; break;

            default: bRes = false; return DataStream::tr("Not an MPEG frame. Invalid layer.");
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
            DECODE_CHECK (nRateIndex >= 1 && nRateIndex <= 14, DataStream::tr("Not an MPEG frame. Invalid bitrate."));
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

                default: bRes = false; return DataStream::tr("Not an MPEG frame. Invalid frequency for MPEG1.");
                }
                break;

            case MPEG2:
                switch (nSmpl)
                {
                case 0x00: m_nFrequency = 22050; break;
                case 0x01: m_nFrequency = 24000; break;
                case 0x02: m_nFrequency = 16000; break;

                default: bRes = false; return DataStream::tr("Not an MPEG frame. Invalid frequency for MPEG2.");
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


    QString Decoder::decodeMpegFrame(const unsigned char* bfr, const char* szSep, bool* pbIsValid) //ttt2 perhaps unify with MpegFrameBase::MpegFrameBase(), using the char* constructor
    {
        QString s (initialize(bfr, pbIsValid));
        if (!s.isEmpty()) { return s; }

        //ostringstream out;
        /*out << getSzVersion() << " " << getSzLayer() << ", " << m_nBitrate/1000 << "kbps, " << m_nFrequency << "Hz, " << getSzChannelMode() << ", padding=" << (m_nPadding ? "true" : "false") << ", length " <<
                m_nSize << " (0x" << hex << m_nSize << dec << ")";*/


        /*out << boolalpha <<
               getSzVersion() << " " <<
               getSzLayer() <<
               szSep <<
               getSzChannelMode()/ *4* / <<
                szSep <<
               m_nFrequency << "Hz" <<
               szSep <<
               m_nBitrate / *8* / << "bps" <<
               szSep << "CRC=" <<
               boolAsYesNo(m_bCrc) <<
               szSep / *11* /<< "length " <<
               m_nSize <<
               " (0x" << hex << m_nSize << dec << ")" <<
               szSep / *14* /<< "padding=" <<
               (m_nPadding ? "true" : "false");*/




        return DataStream::tr("%1 %2%3%4%5%6Hz%7%8bps%9CRC=%10%11length %12 (0x%13)%14padding=%15")
                .arg(DataStream::tr(getSzVersion()))
                .arg(DataStream::tr(getSzLayer()))
                .arg(szSep)
                .arg(DataStream::tr(getSzChannelMode()))
                .arg(szSep)
                .arg(m_nFrequency)
                .arg(szSep)
                .arg(m_nBitrate)
                .arg(szSep)
                .arg(GlobalTranslHlp::tr(boolAsYesNo(m_bCrc)))
                .arg(szSep)
                .arg(m_nSize)
                .arg(m_nSize, 0, 16)
                .arg(szSep)
                .arg(GlobalTranslHlp::tr(boolAsYesNo(m_nPadding)));
    }


    string Decoder::decodeMpegFrameAsXml(const unsigned char* bfr, bool* pbIsValid) //ttt2 perhaps unify with MpegFrameBase::MpegFrameBase(), using the char* constructor
    {
        QString s (initialize(bfr, pbIsValid)); // !!! XML is not translated
        if (!s.isEmpty()) { return convStr(s); }

        ostringstream out;
        out << " version=\"" << getSzVersion() << "\""
            << " layer=\"" << getSzLayer() << "\""
            << " channelMode=\"" << getSzChannelMode() << "\""
            << " frequency=\"" << m_nFrequency << "\""
            << " bps=\"" << m_nBitrate << "\""
            << " crc=\"" << boolAsYesNo(m_bCrc) << "\""

            << " mpegSize=\"" << m_nSize << "\""
            << " padding=\"" << boolAsYesNo(m_nPadding) << "\""; // !!! XML isn't translated

        return out.str();
    }
} // namespace



string decodeMpegFrame(unsigned int x, const char* szSep, bool* pbIsValid /* = 0*/)
{
    Decoder d;
    unsigned char bfr [4];
    unsigned char* q (reinterpret_cast<unsigned char*>(&x));
    bfr[0] = q[3]; bfr[1] = q[2]; bfr[2] = q[1]; bfr[3] = q[0];

    return convStr(d.decodeMpegFrame(bfr, szSep, pbIsValid));
}


string decodeMpegFrame(const char* bfr, const char* szSep, bool* pbIsValid /* = 0*/)
{
    Decoder d;
    const unsigned char* q (reinterpret_cast<const unsigned char*>(bfr));
    return convStr(d.decodeMpegFrame(q, szSep, pbIsValid));
}


string decodeMpegFrameAsXml(const char* bfr, bool* pbIsValid /* = 0*/)
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
    CB_ASSERT (nCnt >= 0);
    char c (0);
    for (int i = 0; i < nCnt; ++i) //ttt2 perhaps make this faster
    {
        out.write(&c, 1);
    }

    CB_CHECK1 (out, WriteError());
}


void listWidget(QWidget* p, int nIndent /* = 0*/)
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

namespace {

struct DesktopDetector {
    enum Desktop { Unknown, Gnome2 = 1, Gnome3 = 2, Kde3 = 4, Kde4 = 8, Gnome = Gnome2 | Gnome3, Kde = Kde3 | Kde4 };
    DesktopDetector();
    Desktop m_eDesktop;
    const char* m_szDesktop;
    bool onDesktop(Desktop desktop) const
    {
        return (desktop & m_eDesktop) != 0;
    }
};


#if defined(__linux__)

DesktopDetector::DesktopDetector() : m_eDesktop(Unknown)
{
    FileSearcher fs ("/proc");
    string strBfr;

    bool bIsKde (false);
    bool bIsKde4 (false);

    while (fs)
    {
        if (fs.isDir())
        {
            string strCmdLineName (fs.getName());

            if (isdigit(strCmdLineName[6]))
            {

#if 0
                char szBfr[5000] = "mP3DiAgS";
                szBfr[0] = 0;

                int k (readlink((strCmdLineName + "/exe").c_str(), szBfr, sizeof(szBfr)));
                if (k >= 0)
                {
                    szBfr[k] = 0;
                }
                cout << strCmdLineName << "          ";
                szBfr[5000 - 1] = 0;
                //if (0 != szBfr[0])
                    cout << szBfr << "        |";
                    //cout << szBfr << endl;//*/


                strCmdLineName += "/cmdline";
                //cout << strCmdLineName << endl;
                ifstream in (strCmdLineName.c_str());
                if (in)
                {
                    getline(in, strBfr);
                    //if (!strBfr.empty()) { cout << "<<<   " << strBfr.c_str() << "   >>>" << endl; }
                    //if (!strBfr.empty()) { cout << strBfr.c_str() << endl; }
                    cout << "          " << strBfr.c_str();
                    if (string::npos != strBfr.find("gnome-settings-daemon"))
                    {
                        m_eDesktop = string::npos != strBfr.find("gnome-settings-daemon-3.") ? Gnome3 : Gnome2;
                        break;
                    }
                }//*/
                cout << endl;

#endif
                char szBfr[5000] = "mP3DiAgS";
                szBfr[0] = 0;


                strCmdLineName += "/cmdline";
                //cout << strCmdLineName << endl;
                ifstream in (strCmdLineName.c_str());
                if (in)
                {
                    getline(in, strBfr);
                    //if (!strBfr.empty()) { cout << "<<<   " << strBfr.c_str() << "   >>>" << endl; }
                    //if (!strBfr.empty()) { cout << strBfr.c_str() << endl; }
                    //cout << strBfr.c_str() << endl;
                    if (string::npos != strBfr.find("gnome-settings-daemon"))
                    {
                        m_eDesktop = string::npos != strBfr.find("gnome-settings-daemon-3.") ? Gnome3 : Gnome2;
                        break;
                    }

                    if (string::npos != strBfr.find("kdeinit"))
                    {
                        bIsKde = true;
                    }

                    //if (string::npos != strBfr.find("kde4/libexec")) // this gives false positives on openSUSE 11.4 from KDE 3:
                    // for i in `ls /proc | grep '^[0-9]'` ; do a=`cat /proc/$i/cmdline 2>/dev/null` ; echo $a | grep kde4/libexec ; done
                    if (string::npos != strBfr.find("kde4/libexec/start")) // ttt2 probably works only on Suse and only in some cases
                    {
                        bIsKde4 = true;
                    }
                }//*/

            }
        }
        //if (string::npos != strBfr.find("kdeinit"))

        fs.findNext();
    }

    if (m_eDesktop == Unknown)
    {
        if (bIsKde4)
        {
            m_eDesktop = Kde4;
        }
        else if (bIsKde)
        {
            m_eDesktop = Kde3;
        }
    }

    if (Gnome2 == m_eDesktop)
    { // while on openSUSE there's gnome-settings-daemon-3, on Fedora it's always gnome-settings-daemon, regardless of the Gnome version; so if Gnome 3 seems to be installed, we'll override a "Gnome2" value
        QDir dir ("/usr/share/gnome-shell");
        if (dir.exists())
        {
            m_eDesktop = Gnome3;
        }
    }

    switch (m_eDesktop)
    {
    case Gnome2: m_szDesktop = "Gnome 2"; break;
    case Gnome3: m_szDesktop = "Gnome 3"; break;
    case Kde3: m_szDesktop = "KDE 3"; break;
    case Kde4: m_szDesktop = "KDE 4"; break;
    default: m_szDesktop = "Unknown";
    }
    //cout << "desktop: " << m_eDesktop << endl;
}

#else // #if defined(__linux__)

DesktopDetector::DesktopDetector() : m_eDesktop(Unknown) {}

#endif

const DesktopDetector& getDesktopDetector()
{
    static DesktopDetector desktopDetector;
    return desktopDetector;
}


} // namespace


bool getDefaultForShowCustomCloseButtons()
{
    return DesktopDetector::Gnome3 == getDesktopDetector().m_eDesktop;
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

//ttt0 look at Qt::CustomizeWindowHint
#ifndef WIN32
    //Qt::WindowFlags getMainWndFlags() { return isRunningOnGnome() ? Qt::Window : Qt::WindowTitleHint; } // !!! these are incorrect, but seem the best option; the values used for Windows are supposed to be OK; they work as expected with KDE but not with Gnome (asking for maximize button not only fails to show it, but causes the "Close" button to disappear as well); Since in KDE min/max buttons are shown when needed anyway, it's sort of OK // ttt2 see if there is workaround/fix
    Qt::WindowFlags getMainWndFlags() { const DesktopDetector& dd = getDesktopDetector(); return dd.onDesktop(DesktopDetector::Kde) ? Qt::WindowTitleHint : Qt::Window; }
#if QT_VERSION >= 0x040500
    //Qt::WindowFlags getDialogWndFlags() { const DesktopDetector& dd = getDesktopDetector(); return dd.onDesktop(DesktopDetector::Kde) ? Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint : (dd.onDesktop(DesktopDetector::Gnome3) ? Qt::Window : Qt::WindowTitleHint); }
    Qt::WindowFlags getDialogWndFlags() { const DesktopDetector& dd = getDesktopDetector(); return dd.onDesktop(DesktopDetector::Kde) ? Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint : (/*dd.onDesktop(DesktopDetector::Gnome3) ? Qt::Window :*/ Qt::WindowTitleHint); }
#else
    //Qt::WindowFlags getDialogWndFlags() { const DesktopDetector& dd = getDesktopDetector(); return dd.onDesktop(DesktopDetector::Gnome3) ? Qt::Window : Qt::WindowTitleHint; }
    Qt::WindowFlags getDialogWndFlags() { const DesktopDetector& dd = getDesktopDetector(); return /*dd.onDesktop(DesktopDetector::Gnome3) ? Qt::Window :*/ Qt::WindowTitleHint; }
    // ttt0 perhaps better to make sure all dialogs have their ok/cancel buttons, so there's no need for a dedicated close button and let the app look more "native"
#endif
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


// !!! don't translate
QString getSystemInfo() //ttt2 perhaps store this at startup, so fewer things may go wrong fhen the assertion handler needs it
{
    QString s ("OS: ");
    QString qstrDesktop;

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

    const DesktopDetector& dd = getDesktopDetector();
    qstrDesktop = "Desktop: " + QString(dd.m_szDesktop) + "\n";

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
    s = QString("Version: %1 %2\nWord size: %3 bit\nQt version: %4\nBoost version: %5\n").arg(getAppName()).arg(getAppVer()).arg(QSysInfo::WordSize).arg(qVersion()).arg(BOOST_LIB_VERSION) + qstrDesktop + s;
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
        s_v.push_back(QString("/usr/share/") + getHelpPackageName() + "-doc/html/"); //ttt0 lower/uppercase variations
        s_v.push_back(QString("/usr/share/doc/") + getHelpPackageName() + "/html/");
        s_v.push_back(QString("/usr/share/doc/") + getHelpPackageName() + "-QQQVERQQQ/html/");
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
        qs = "http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/";
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
QString makeMultiline(const QString& qstrDescr)
{
    QString s (qstrDescr);
    int SIZE (50);
    for (int i = SIZE; i < qstrDescr.size(); ++i)
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


#if defined(__linux__)

namespace {

//const char* DSK_FOLDER ("~/.local/share/applications/");
const string& getDesktopIntegrationDir()
{
    static string s_s;
    if (s_s.empty())
    {
        s_s = convStr(QDir::homePath() + "/.local/share/applications");
        try
        {
            createDir(s_s);
        }
        catch (...)
        { // nothing; this will cause shell integration to be disabled
            cerr << "failed to create dir " << s_s << endl;
        }
        s_s += "/";
    }
    return s_s;
}

const char* DSK_EXT (".desktop");

class ShellIntegrator
{
    Q_DECLARE_TR_FUNCTIONS(ShellIntegrator)

    string m_strFileName;
    string m_strAppName;
    string m_strArg;
    bool m_bRebuildAssoc;

    static string escape(const string& s)
    {
        string s1;
        static string s_strReserved (" '\\><~|&;$*?#()`"); // see http://standards.freedesktop.org/desktop-entry-spec/latest/ar01s06.html
        for (int i = 0; i < cSize(s); ++i)
        {
            char c (s[i]);
            if (s_strReserved.find(c) != string::npos)
            {
                s1 += '\\';
            }
            s1 += c;
        }
        return s1;
    }

public:
    ShellIntegrator(const string& strFileNameBase, const char* szSessType, const string& strArg, bool bRebuildAssoc) :
        m_strFileName(getDesktopIntegrationDir() + strFileNameBase + DSK_EXT),
        m_strAppName(convStr(tr("%1 - %2").arg(getAppName()).arg(tr(szSessType)))),
        m_strArg(strArg),
        m_bRebuildAssoc(bRebuildAssoc) {}

    enum { DONT_REBUILD_ASSOC = 0, REBUILD_ASSOC = 1 };

    bool isEnabled()
    {
        return fileExists(m_strFileName);
    }

    void enable(bool b)
    {
        if (!(b ^ isEnabled())) { return; }

        if (b)
        {
            char szBfr [5000] = "mP3DiAgS";
            szBfr[0] = 0;
            int k (readlink("/proc/self/exe", szBfr, sizeof(szBfr)));
            if (k >= 0)
            {
                szBfr[k] = 0;
            }
            szBfr[5000 - 1] = 0;

            ofstream_utf8 out (m_strFileName.c_str());
            if (!out)
            {
                qDebug("couldn't open file %s", m_strFileName.c_str());
            }

            out << "[Desktop Entry]" << endl;
            out << "Comment=" << m_strAppName << endl;
            out << "Encoding=UTF-8" << endl;
            out << "Exec=" << escape(szBfr) << " " << m_strArg << " %f" << endl;
            out << "GenericName=" << m_strAppName << endl;
            out << "Icon=" << getIconName() << endl;
            out << "Name=" << m_strAppName << endl;
            out << "Path=" << endl;
            out << "StartupNotify=false" << endl;
            out << "Terminal=0" << endl;
            out << "TerminalOptions=" << endl;
            out << "Type=Application" << endl;
            out << "X-KDE-SubstituteUID=false" << endl;
            out << "X-KDE-Username=" << endl;
            out << "MimeType=inode/directory" << endl;
            out << "NoDisplay=true" << endl;

            out.close();

            static bool s_bErrorReported (false);
            bool bError (false);

            if (m_bRebuildAssoc && getDesktopDetector().onDesktop(DesktopDetector::Kde))
            {
                TRACER1A("ShellIntegrator::enable()", 1);
                QProcess kbuildsycoca4;
                kbuildsycoca4.start("kbuildsycoca4");
                TRACER1A("ShellIntegrator::enable()", 2);

                if (!kbuildsycoca4.waitForStarted()) //ttt1 switch to non-blocking calls if these don't work well enough
                {
                    TRACER1A("ShellIntegrator::enable()", 3);
                    bError = true;
                }
                else
                {
                    TRACER1A("ShellIntegrator::enable()", 4);
                    kbuildsycoca4.closeWriteChannel();
                    TRACER1A("ShellIntegrator::enable()", 5);
                    if (!kbuildsycoca4.waitForFinished())
                    {
                        TRACER1A("ShellIntegrator::enable()", 6);
                        bError = true;
                    }
                }
                TRACER1A("ShellIntegrator::enable()", 7);

                if (bError && !s_bErrorReported)
                {
                    s_bErrorReported = true;
                    HtmlMsg::msg(0, 0, 0, 0, HtmlMsg::CRITICAL, tr("Error setting up shell integration"), tr("It appears that setting up shell integration didn't complete successfully. You might have to configure it manually.") + "<p/>"
                                 + tr("This message will not be shown again until the program is restarted, even if more errors occur.")
                                 , 400, 300, tr("O&K"));
                }
            }
        }
        else
        {
            try
            {
                deleteFile(m_strFileName);
            }
            catch (...)
            { //ttt2 do something
            }
        }
    }
};

ShellIntegrator g_tempShellIntegrator ("mp3DiagsTempSess", QT_TRANSLATE_NOOP("ShellIntegrator", "temporary folder"), "-t", ShellIntegrator::REBUILD_ASSOC);
ShellIntegrator g_hiddenShellIntegrator ("mp3DiagsHiddenSess", QT_TRANSLATE_NOOP("ShellIntegrator", "hidden folder"), "-f", ShellIntegrator::REBUILD_ASSOC);
ShellIntegrator g_visibleShellIntegrator ("mp3DiagsVisibleSess", QT_TRANSLATE_NOOP("ShellIntegrator", "visible folder"), "-v", ShellIntegrator::REBUILD_ASSOC);

ShellIntegrator g_testShellIntegrator ("mp3DiagsTestSess_000", "test", "", ShellIntegrator::DONT_REBUILD_ASSOC);

} // namespace


/*static*/ bool ShellIntegration::isShellIntegrationEditable()
{
    g_testShellIntegrator.enable(true);
    bool b (g_testShellIntegrator.isEnabled());
    g_testShellIntegrator.enable(false);
    return b;
}


/*static*/ string ShellIntegration::getShellIntegrationError()
{
    return "";
}


/*static*/ void ShellIntegration::enableTempSession(bool b)
{
    g_tempShellIntegrator.enable(b);
}

/*static*/ bool ShellIntegration::isTempSessionEnabled()
{
    return g_tempShellIntegrator.isEnabled();
}

/*static*/ void ShellIntegration::enableVisibleSession(bool b)
{
    g_visibleShellIntegrator.enable(b);
}

/*static*/ bool ShellIntegration::isVisibleSessionEnabled()
{
    return g_visibleShellIntegrator.isEnabled();
}

/*static*/ void ShellIntegration::enableHiddenSession(bool b)
{
    g_hiddenShellIntegrator.enable(b);
}

/*static*/ bool ShellIntegration::isHiddenSessionEnabled()
{
    return g_hiddenShellIntegrator.isEnabled();
}


#elif defined (WIN32)

namespace {

//ttt2 use a class instead of functions, to handle errors better

struct RegKey
{
    HKEY m_hKey;
    RegKey() : m_hKey(0) {}

    ~RegKey()
    {
        RegCloseKey(m_hKey);
    }
};

/*
{
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Directory\\shell", 0, KEY_READ, &hkey))
    {
        RegCloseKey(hkey);
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Directory\\shell", 0, KEY_WRITE, &hkey))
    {
        HKEY hSubkey;
        if (ERROR_SUCCESS == RegCreateKeyEx(hkey, L"MySubkey", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubkey, NULL))
        {
            if (ERROR_SUCCESS == RegSetValueExA(hSubkey, NULL, 0, REG_SZ, (const BYTE*)("my string"), 10))
            {
                cout << "OK\n";
            }
            RegCloseKey(hSubkey);
        }
        RegCloseKey(hkey);
    }
}

*/


//bool doesKeyExist(const wchar_t* wszPath)
bool doesKeyExist(const char* szPath)
{
    RegKey key;
    return ERROR_SUCCESS == RegOpenKeyExA(HKEY_CLASSES_ROOT, szPath, 0, KEY_READ, &key.m_hKey);
}

//bool createEntries(const wchar_t* wszPath, const wchar_t* wszSubkey, const wchar_t* wszDescr, const wchar_t* wszCommand)
bool createEntries(const char* szPath, const char* szSubkey, const char* szDescr, const char* szParam)
{
    string s (string("\"") + _pgmptr + "\" " + szParam);
    const char* szCommand (s.c_str());

    RegKey key;

    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_CLASSES_ROOT, szPath, 0, KEY_WRITE, &key.m_hKey)) { return false; }

    RegKey subkey;
    if (ERROR_SUCCESS != RegCreateKeyExA(key.m_hKey, szSubkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &subkey.m_hKey, NULL)) { return false; }
    if (ERROR_SUCCESS != RegSetValueExA(subkey.m_hKey, NULL, 0, REG_SZ, (const BYTE*)szDescr, strlen(szDescr) + 1)) { return false; }

    RegKey commandKey;
    if (ERROR_SUCCESS != RegCreateKeyExA(subkey.m_hKey, "command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &commandKey.m_hKey, NULL)) { return false; }
    if (ERROR_SUCCESS != RegSetValueExA(commandKey.m_hKey, NULL, 0, REG_SZ, (const BYTE*)szCommand, strlen(szCommand) + 1)) { return false; }

    return true;
}

bool deleteKey(const char* szPath, const char* szSubkey)
{
    RegKey key;

    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_CLASSES_ROOT, szPath, 0, KEY_WRITE, &key.m_hKey)) { return false; }

    RegKey subkey;
    if (ERROR_SUCCESS != RegCreateKeyExA(key.m_hKey, szSubkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &subkey.m_hKey, NULL)) { return false; }

    if (ERROR_SUCCESS != RegDeleteKeyA(subkey.m_hKey, "command")) { return false; }

    if (ERROR_SUCCESS != RegDeleteKeyA(key.m_hKey, szSubkey)) { return false; }

    return true;
}

} // namespace

//--------------------------------------------------------------

//ttt1 use something like ShellIntegrator in the Linux code

/*static*/ bool ShellIntegration::isShellIntegrationEditable()
{
    //RegKey key;
    // return ERROR_SUCCESS == RegOpenKeyExA(HKEY_CLASSES_ROOT, "Directory\\shell", 0, KEY_WRITE, &key.m_hKey); //!!! if compiled with MinGW on XP there's this issue on W7: it seems to work but it looks like it uses a temporary path, which gets soon deleted

    createEntries("Directory\\shell", "test_000_mp3diags", "test", "-t \"%1\"");
    if (!doesKeyExist("Directory\\shell\\test_000_mp3diags")) { return false; }

    deleteKey("Directory\\shell", "test_000_mp3diags");

    return true;
}


/*static*/ string ShellIntegration::getShellIntegrationError()
{
    if (isShellIntegrationEditable()) { return ""; }
    return convStr(GlobalTranslHlp::tr("These settings cannot currently be changed. In order to make changes you should probably run the program as an administrator."));
}



/*static*/ void ShellIntegration::enableTempSession(bool b)
{
    if (!(isTempSessionEnabled() ^ b)) { return; } // no change needed
    if (b)
    {
        createEntries("Directory\\shell", "mp3diags_temp_dir", "Open as temporary folder in MP3 Diags", "-t \"%1\"");
        createEntries("Drive\\shell", "mp3diags_temp_dir", "Open as temporary folder in MP3 Diags", "-t %1");
    }
    else
    {
        deleteKey("Directory\\shell", "mp3diags_temp_dir");
        deleteKey("Drive\\shell", "mp3diags_temp_dir");
    }
}

/*static*/ bool ShellIntegration::isTempSessionEnabled()
{
    return doesKeyExist("Directory\\shell\\mp3diags_temp_dir");
}


/*static*/ void ShellIntegration::enableVisibleSession(bool b)
{
    if (!(isVisibleSessionEnabled() ^ b)) { return; } // no change needed
    if (b)
    {
        createEntries("Directory\\shell", "mp3diags_visible_dir", "Open as visible folder in MP3 Diags", "-v \"%1\"");
        createEntries("Drive\\shell", "mp3diags_visible_dir", "Open as visible folder in MP3 Diags", "-v %1");
    }
    else
    {
        deleteKey("Directory\\shell", "mp3diags_visible_dir");
        deleteKey("Drive\\shell", "mp3diags_visible_dir");
    }
}

/*static*/ bool ShellIntegration::isVisibleSessionEnabled()
{
    return doesKeyExist("Directory\\shell\\mp3diags_visible_dir");
}


/*static*/ void ShellIntegration::enableHiddenSession(bool b)
{
    if (!(isHiddenSessionEnabled() ^ b)) { return; } // no change needed
    if (b)
    {
        createEntries("Directory\\shell", "mp3diags_hidden_dir", "Open as hidden folder in MP3 Diags", "-f \"%1\"");
        createEntries("Drive\\shell", "mp3diags_hidden_dir", "Open as hidden folder in MP3 Diags", "-f %1");
    }
    else
    {
        deleteKey("Directory\\shell", "mp3diags_hidden_dir");
        deleteKey("Drive\\shell", "mp3diags_hidden_dir");
    }
}

/*static*/ bool ShellIntegration::isHiddenSessionEnabled()
{
    return doesKeyExist("Directory\\shell\\mp3diags_hidden_dir");
}

#else

/*static*/ bool ShellIntegration::isShellIntegrationEditable()
{
    return false;
}


/*static*/ string ShellIntegration::getShellIntegrationError()
{
    return convStr(GlobalTranslHlp::tr("Platform not supported"));
}


/*static*/ void ShellIntegration::enableTempSession(bool)
{
}

/*static*/ bool ShellIntegration::isTempSessionEnabled()
{
    return false;
}

/*static*/ void ShellIntegration::enableVisibleSession(bool)
{
}

/*static*/ bool ShellIntegration::isVisibleSessionEnabled()
{
    return false;
}

/*static*/ void ShellIntegration::enableHiddenSession(bool)
{
}

/*static*/ bool ShellIntegration::isHiddenSessionEnabled()
{
    return false;
}

#endif



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





//ttt1 maybe switch to new spec, lower-case for exe name, package, and icons



