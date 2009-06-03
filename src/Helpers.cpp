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
#include  <fstream>
#include  <sstream>
#include  <iomanip>

#ifndef WIN32
#else
    #include  <windows.h>
    #include  <psapi.h>
#endif

#include  <QWidget>
#include  <QUrl>

#include  "Helpers.h"


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




// prints to cout the content of a memory location, as ASCII and hex;
// GDB has a tendency to not see char arrays and other local variables; actually GCC seems to be the culprit (bug 34767);
void inspect(const void* q, int nSize)
{
    const char* p ((const char*)q);
    for (int i = 0; i < nSize; ++i)
    {
        char c (p[i]);
        if (c < 32 || c > 126) { c = '.'; }
        cout << c;
    }
    cout << "\n(";

    cout << hex << setfill('0');
    bool b (false);
    for (int i = 0; i < nSize; ++i)
    {
        if (b) { cout << " "; }
        b = true;
        unsigned char c (p[i]);
        cout << setw(2) << (int)c;
    }
    cout << dec << ")\n";
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
        if (!isspace(s[i])) { break; } //ttt2 force C locale
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
        if (!isspace(s[i])) { break; } //ttt2 force C locale
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



// the total memory currently used by the current process, in kB
long getMemUsage()
{
#ifndef WIN32
    //pid_t
    int n ((int)getpid());
    char a [30];
    sprintf (a, "/proc/%d/status", n); // ttt2 linux-specific; not sure how version-specific this is;
    // sprintf (a, "/proc/self/status", n); // ttt1 try this (after checking portability)
    ifstream in (a);
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


void logToFile(const string& s) //tttc make sure it is disabled in public releases
{
    ofstream out (
#ifndef WIN32
            "/tmp/Mp3DiagsLog.txt",
#else
            "C:/Mp3DiagsLog.txt",
#endif
            ios_base::app);
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

        string decodeMpegFrame(const unsigned char* bfr, const char* szSep, bool* pbIsValid);
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

    string Decoder::decodeMpegFrame(const unsigned char* bfr, const char* szSep, bool* pbIsValid) //ttt1 perhaps unify with MpegFrameBase::MpegFrameBase(), using the char* constructor
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
            case 0x00: bRes = false; return "Not an MPEG frame. Unsupported version (2.5)."; //ttt1 see about supporting this: search for MPEG1 to find other places
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

        ostringstream out;
        /*out << getSzVersion() << " " << getSzLayer() << ", " << m_nBitrate/1000 << "kbps, " << m_nFrequency << "Hz, " << getSzChannelMode() << ", padding=" << (m_nPadding ? "true" : "false") << ", length " <<
                m_nSize << " (0x" << hex << m_nSize << dec << ")";*/
        out << boolalpha << getSzVersion() << " " << getSzLayer() << szSep << getSzChannelMode() <<
                szSep << m_nFrequency << "Hz" << szSep << m_nBitrate << "bps" << szSep << "CRC=" << boolAsYesNo(m_bCrc) << szSep <<
                "length " << m_nSize << " (0x" << hex << m_nSize << dec << ")" << szSep << "padding=" << (m_nPadding ? "true" : "false");

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
    return '/'; // ttt1 linux-specific
}

const string& getPathSepAsStr()
{
    static string s ("/");
    return s; // ttt1 linux-specific
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

