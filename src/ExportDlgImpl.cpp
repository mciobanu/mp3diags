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


#include  <algorithm>
#include  <iomanip>

#include  <QFileDialog>
#include  <QTextDocument>

#include  "ExportDlgImpl.h"

#include  "Helpers.h"
#include  "CommonData.h"
#include  "Widgets.h"
#include  "StoredSettings.h"
#include  "DataStream.h"
#include  "fstream_unicode.h"
#include  "Id3V2Stream.h"
#include  "MpegStream.h"
#include  "ApeStream.h"
#include  "LyricsStream.h"


using namespace std;

ExportDlgImpl::ExportDlgImpl(QWidget* pParent) : QDialog(pParent, getDialogWndFlags()), Ui::ExportDlg()
{
    setupUi(this);

    int nWidth, nHeight;
    bool bSortByShortNames;
    string strFile;
    bool bUseVisible;
    string strM3uRoot;

    getCommonData()->m_settings.loadExportSettings(nWidth, nHeight, bSortByShortNames, strFile, bUseVisible, strM3uRoot);
    if (nWidth > 400 && nHeight > 200)
    {
        resize(nWidth, nHeight);
    }
    else
    {
        //defaultResize(*this);
    }

    m_pSortByShortNamesCkB->setChecked(bSortByShortNames);
    m_pFileNameE->setText(toNativeSeparators(convStr(strFile)));
    (bUseVisible ? m_pVisibleRB : m_pSelectedRB)->setChecked(true);

    m_pM3uRootE->setText(toNativeSeparators(convStr(strM3uRoot)));

    m_pM3uRB->setChecked(true);
    setFormatBtn();
}


ExportDlgImpl::~ExportDlgImpl()
{
}



void ExportDlgImpl::run()
{
    if (QDialog::Accepted != exec()) { return; }

    getCommonData()->m_settings.saveExportSettings(width(), height(), m_pSortByShortNamesCkB->isChecked(), convStr(fromNativeSeparators(m_pFileNameE->text())), m_pVisibleRB->isChecked(), convStr(fromNativeSeparators(m_pM3uRootE->text())));
}


void ExportDlgImpl::on_m_pExportB_clicked()
{
    QString qs (fromNativeSeparators(m_pFileNameE->text()));
    if (QFileInfo(qs).isFile())
    {
        HtmlMsg::msg(this, 1, 1, 0, 0, "Warning", "A file called \"" + m_pFileNameE->text() + "\" already exists. Do you want to overwrite it?", 600, 200, "Overwrite", "Cancel");
    }

    bool b (false);

    {
        CursorOverrider crs;
        string s (convStr(qs));
        if (m_pTextRB->isChecked())
        {
            b = exportAsText(s);
        }
        else if (m_pM3uRB->isChecked())
        {
            b = exportAsM3u(s);
        }
        else if (m_pXmlRB->isChecked())
        {
            b = exportAsXml(s);
        }
        else
        {
            CB_ASSERT(false);
        }
    }

    if (b)
    {
        HtmlMsg::msg(this, 0, 0, 0, 0, "Info", "Successfully created file \"" + m_pFileNameE->text() + "\"", 600, 200, "OK");
    }
    else
    {
        HtmlMsg::msg(this, 0, 0, 0, 0, "Error", "There was an error writing to the file \"" + m_pFileNameE->text() + "\"", 600, 200, "OK");
    }
}


void ExportDlgImpl::on_m_pChooseFileB_clicked()
{
    QString qstrDir (QFileInfo(fromNativeSeparators(m_pFileNameE->text())).path());
    QFileDialog dlg (this, "Choose destination file", qstrDir, "XML files (*.xml);;Text files (*.txt);;M3U files (*.m3u)");

    /*QStringList filters;
    filters << "Text files (*.txt)" << "M3U files (*.m3u)";
    dlg.setFilters(filters);*/

    dlg.setFileMode(QFileDialog::AnyFile);
    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; }

    QString s (fileNames.first());

    QString flt (dlg.selectedFilter());
    if (flt.endsWith("xml)") && !s.endsWith(".xml"))
    {
        s += ".xml";
    }
    else if (flt.endsWith("txt)") && !s.endsWith(".txt"))
    {
        s += ".txt";
    }
    else if (flt.endsWith("m3u)") && !s.endsWith(".m3u"))
    {
        s += ".m3u";
    }

    m_pFileNameE->setText(toNativeSeparators(s));

    setFormatBtn();
}


void ExportDlgImpl::setFormatBtn()
{
    QString qs (m_pFileNameE->text());

    if (qs.endsWith(".xml"))
    {
        m_pXmlRB->setChecked(true);
    }
    else if (qs.endsWith(".txt"))
    {
        m_pTextRB->setChecked(true);
    }
    else if (qs.endsWith(".m3u"))
    {
        m_pM3uRB->setChecked(true);
    }
}
//ttt0 README.TXT in root with build instructions


void ExportDlgImpl::setExt(const char* szExt)
{
    CB_ASSERT (3 == strlen(szExt));
    QString s (m_pFileNameE->text());
    int n (s.size() - 4);
    if (n > 0 && '.' == s[n])
    {
        QString s1 (s);
        s1.remove(n + 1, 3);
        s1 += szExt;
        if (s1 != s)
        {
            m_pFileNameE->setText(s1);
        }
    }
}


/*$SPECIALIZATION$*/


namespace {

struct CmpMp3HandlerByShortNameAndSize
{
    bool operator()(const Mp3Handler* p1, const Mp3Handler* p2)
    {
        if (p1->getShortName() < p2->getShortName()) { return true; }
        if (p2->getShortName() < p1->getShortName()) { return false; }
        if (p1->getSize() < p2->getSize()) { return true; }
        if (p2->getSize() < p1->getSize()) { return false; }
        return p1->getName() < p2->getName();
    }
};

}


void ExportDlgImpl::getHandlers(vector<const Mp3Handler*>& v)
{
    v.clear();

    const deque<const Mp3Handler*>& vpHndl (m_pVisibleRB->isChecked() ? getCommonData()->getViewHandlers() : getCommonData()->getSelHandlers());

    v.insert(v.end(), vpHndl.begin(), vpHndl.end());
    if (m_pSortByShortNamesCkB->isChecked())
    {
        sort(v.begin(), v.end(), CmpMp3HandlerByShortNameAndSize());
    }
}



bool ExportDlgImpl::exportAsText(const string& strFileName)
{
    vector<const Mp3Handler*> v;
    getHandlers(v);

    ofstream_utf8 out (strFileName.c_str());
    const char* aSeverity = "EWST";
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        const Mp3Handler* p (v[i]);
        out << p->getName() << " ";
        out << p->getSize() << endl;

        const vector<DataStream*>& vpStreams (p->getStreams());
        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            out << "  " << hex << p->getPos() << "-" << (p->getPos() + (p->getSize() - 1)) << dec << " (" << p->getSize() << ") " << p->getDisplayName();
            const string& s (p->getInfo());
            if (!s.empty())
            {
                out << ": " << s;
            }
            out << endl;
        }

        const NoteColl& notes (p->getNotes());
        out << "  --------------------------------------------\n";
        vector<const Note*> vpNotes (notes.getList().begin(), notes.getList().end());
        sort(vpNotes.begin(), vpNotes.end(), CmpNotePtrByPosAndId());
        for (int i = 0, n = cSize(vpNotes); i < n; ++i)
        {
            const Note* p (vpNotes[i]);
            if (getCommonData()->m_bUseAllNotes || getCommonData()->findPos(p) >= 0) // !!! "ignored" notes shouldn't be exported unless UseAllNotes is checked, so there is consistency between what is shown on the screen and what is saved
            {
                out << "  " << aSeverity[p->getSeverity()] << " ";
                const string& q (p->getPosHex());
                if (!q.empty())
                {
                    out << q << " ";
                }
                const string& s (p->getDetail());
                if (s.empty()) // ttt2 perhaps show descr anyway
                {
                    out << p->getDescription();
                }
                else
                {
                    out << s;
                }
                out << endl;
            }
        }

        out << "\n\n";
    }

    return out;
}

bool ExportDlgImpl::exportAsM3u(const std::string& strFileName)
{
    vector<const Mp3Handler*> v; //ttt0 use Codepage
    getHandlers(v);

    ofstream_utf8 out (strFileName.c_str());
    string strRoot (convStr(fromNativeSeparators(m_pM3uRootE->text())));
    int nRootSize (cSize(strRoot));
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        const Mp3Handler* p (v[i]);
        string s (p->getName());
        if (nRootSize > 0 && beginsWith(s, strRoot))
        {
            s.erase(0, nRootSize);
        }
        out << s << endl;
    }

    return out;
}


namespace {


string escapeXml(const string& s)
{
    return convStr(Qt::escape(convStr(s)));
}

void printDataStream(ostream& out, DataStream* p)
{
    out << " type=\"" << p->getDisplayName() << "\" begin=\"0x" << hex << p->getPos() << "\" end=\"0x" << (p->getPos() + (p->getSize() - 1)) << "\" size=\"" << dec << p->getSize() << "\"";
}


void printTagReader(ostream& out, TagReader* p)
{

    bool b;
    string s;

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::TITLE))
    {
        s = p->getTitle(&b);
        if (b)
        {
            out << " title=\"" << escapeXml(s) << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::ARTIST))
    {
        s = p->getArtist(&b);
        if (b)
        {
            out << " artist=\"" << escapeXml(s) << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::TRACK_NUMBER))
    {
        s = p->getTrackNumber(&b);
        if (b)
        {
            out << " trackNo=\"" << escapeXml(s) << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::TIME))
    {
        p->getTime(&b);
        if (b)
        {
            out << " time=\"" << p->getValue(TagReader::TIME) << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::GENRE))
    {
        s = p->getGenre(&b);
        if (b)
        {
            out << " genre=\"" << escapeXml(s) << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::IMAGE))
    {
        s = p->getImageData(&b);
        if (b)
        {
            out << " image=\"" << escapeXml(s) << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::ALBUM))
    {
        s = p->getAlbumName(&b);
        if (b)
        {
            out << " album=\"" << escapeXml(s) << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::RATING))
    {
        double d (p->getRating(&b));
        if (b)
        {
            out << " rating=\"" << setprecision(1) << d << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::COMPOSER))
    {
        s = p->getComposer(&b);
        if (b)
        {
            out << " composer=\"" << escapeXml(s) << "\"";
        }
    }

    if (TagReader::NOT_SUPPORTED != p->getSupport(TagReader::VARIOUS_ARTISTS))
    {
        p->getVariousArtists(&b);
        if (b)
        {
            out << " variousArtists=\"" << p->getValue(TagReader::VARIOUS_ARTISTS) << "\"";
        }
    }
}


enum { DONT_PRINT_BPS, PRINT_BPS };

void printMpegInfo(ostream& out, const MpegFrame& frm, bool bPrintBps)
{
    out << " version=\"" << frm.getSzVersion() << "\""
        << " layer=\"" << frm.getSzLayer() << "\""
        << " channelMode=\"" << frm.getSzChannelMode() << "\""
        << " frequency=\"" << frm.getFrequency() << "\"";

    if (bPrintBps)
    {
        out << " bps=\"" << frm.getBitrate() << "\"";
    }

    out << " crc=\"" << boolAsYesNo(frm.getCrcUsage()) << "\"";
}

} // namespace

bool ExportDlgImpl::exportAsXml(const std::string& strFileName)
{
    vector<const Mp3Handler*> v; //ttt0 use Codepage
    getHandlers(v);

    ofstream_utf8 out (strFileName.c_str());

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<files>\n";

    const char* aszSeverity[] = { "error", "warning", "support", "trace" };
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        const Mp3Handler* p (v[i]);
        out << "  <file name=\"" << escapeXml(p->getName()) << "\" size=\"" << p->getSize() << "\">\n";
        out << "    <streams>\n";

        const vector<DataStream*>& vpStreams (p->getStreams());
        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);

            if (0 != dynamic_cast<Id3V2StreamBase*>(p))
            {
                Id3V2StreamBase* p1 (dynamic_cast<Id3V2StreamBase*>(p));

                out << "      <id3V2Stream";
                printDataStream(out, p1);
                printTagReader(out, p1);
                out << ">\n";
                vector<Id3V2Frame*> vpFrames (p1->getFrames());
                for (int i = 0, n = cSize(vpFrames); i < n; ++i)
                {
                    Id3V2Frame* p (vpFrames[i]);

                    if (Id3V2Frame::NO_APIC != p->m_eApicStatus)
                    {
                        out << "        <pictureFrame name=\"" << p->getReadableName() << "\" size=\"" << p->m_nDiskDataSize << "\" width=\"" << p->m_nWidth << "\" height=\"" << p->m_nHeight << "\" type=\"" << p->getImageType() << "\" status=\"" << p->getImageStatus() << "\" compr=\"" << ImageInfo::getComprStr(p->m_eCompr) << "\"/>\n";
                    }
                    else if ('T' == p->m_szName[0])
                    {
                        out << "        <textFrame name=\"" << p->getReadableName() << "\" size=\"" << p->m_nDiskDataSize << "\">\n";
                        out << "          " << escapeXml(p->getUtf8String()) << "\n";
                        out << "        </textFrame>\n";
                    }
                    else if (0 == strcmp(KnownFrames::LBL_RATING(), p->m_szName))
                    {
                        out << "        <ratingFrame name=\"" << p->getReadableName() << "\" size=\"" << p->m_nDiskDataSize << "\" normalizedValue=\"" << setprecision(1) << p->getRating() << "\"/>\n";
                    }
                    else
                    {
                        out << "        <binaryFrame name=\"" << p->getReadableName() << "\" size=\"" << p->m_nDiskDataSize << "\"/>\n";
                    }
                }
                out << "      </id3V2Stream>\n";
            }
            else if (0 != dynamic_cast<Id3V1Stream*>(p))
            {
                Id3V1Stream* p1 (dynamic_cast<Id3V1Stream*>(p));

                out << "      <id3V1Stream version=\"" << p1->getVersion() << "\"";
                printDataStream(out, p);
                printTagReader(out, p1);
                out << "/>\n";
            }
            else if (0 != dynamic_cast<MpegStream*>(p))
            {
                MpegStream* p1 (dynamic_cast<MpegStream*>(p));

                out << "      <mpegAudioStream";
                printDataStream(out, p);

                out << " duration=\"" << p1->getDuration() << "\"";
                printMpegInfo(out, p1->getFirstFrame(), DONT_PRINT_BPS);

                out << " bitrate=\"" << p1->getBitrate() << "\""
                    << " vbr=\"" << boolAsYesNo(p1->isVbr()) << "\""
                    << " frameCount=\"" << p1->getFrameCount() << "\"";
                    // << " bitrate=\"" << p1->getBitrate() << "\""  (m_bRemoveLastFrameCalled ? " removed; it was" : "") << " located at 0x" << hex << m_posLastFrame

                out << "/>\n";
            }
            else if (0 != dynamic_cast<XingStream*>(p))
            {
                XingStream* p1 (dynamic_cast<XingStream*>(p));

                out << "      <xingStream";
                printDataStream(out, p);

                printMpegInfo(out, p1->getFirstFrame(), PRINT_BPS);
                out << p1->getInfoForXml() << "/>\n";
            }
            else if (0 != dynamic_cast<LameStream*>(p))
            {
                LameStream* p1 (dynamic_cast<LameStream*>(p));

                out << "      <lameStream";
                printDataStream(out, p);

                printMpegInfo(out, p1->getFirstFrame(), PRINT_BPS);
                out << p1->getInfoForXml() << "/>\n";
            }
            else if (0 != dynamic_cast<VbriStream*>(p))
            {
                VbriStream* p1 (dynamic_cast<VbriStream*>(p));

                out << "      <vbriStream";
                printDataStream(out, p);

                printMpegInfo(out, p1->getFirstFrame(), PRINT_BPS);
                out << "/>\n";
            }
            else if (0 != dynamic_cast<TruncatedMpegDataStream*>(p))
            {
                TruncatedMpegDataStream* p1 (dynamic_cast<TruncatedMpegDataStream*>(p));

                out << "      <truncatedMpegAudioStream";
                printDataStream(out, p);

                out << decodeMpegFrameAsXml(p1->getBegin(), 0);

                out << "/>\n";
            }
            else if (0 != dynamic_cast<ApeStream*>(p))
            {
                ApeStream* p1 (dynamic_cast<ApeStream*>(p));

                out << "      <apeStream";
                printDataStream(out, p);
                printTagReader(out, p1);
                out << "/>\n";
            }
            else if (0 != dynamic_cast<LyricsStream*>(p))
            {
                LyricsStream* p1 (dynamic_cast<LyricsStream*>(p));

                out << "      <lyricsStream";
                printDataStream(out, p);
                printTagReader(out, p1);
                out << "/>\n";
            }
            else
            {
                out << "      <stream";
                printDataStream(out, p);
                out << ">\n";
                const string& s (p->getInfo());
                if (!s.empty())
                {
                    out << "        <info>\n";
                    out << "          " << escapeXml(s) << "\n";
                    out << "        </info>\n";
                }
                out << "      </stream>\n";
            }
        }
        out << "    </streams>\n";

        const NoteColl& notes (p->getNotes());
        out << "    <notes>\n";
        vector<const Note*> vpNotes (notes.getList().begin(), notes.getList().end());
        sort(vpNotes.begin(), vpNotes.end(), CmpNotePtrByPosAndId());
        for (int i = 0, n = cSize(vpNotes); i < n; ++i)
        {
            const Note* p (vpNotes[i]);
            if (getCommonData()->m_bUseAllNotes || getCommonData()->findPos(p) >= 0) // !!! "ignored" notes shouldn't be exported unless UseAllNotes is checked, so there is consistency between what is shown on the screen and what is saved
            {
                out << "    <note type=\"" << aszSeverity[p->getSeverity()] << "\"";

                const string& q (p->getPosHex());
                if (!q.empty())
                {
                    out << " pos=\"" << q << "\"";
                }

                out << " description=\"" << escapeXml(p->getDescription()) << "\"";
                const string& s (p->getDetail());
                if (!s.empty())
                {
                    out << " detail=\"" << escapeXml(s) << "\"";
                }
                out << "/>\n";
            }
        }
        out << "    </notes>\n";

        out << "  </file>\n";
    }

    out << "</files>\n";

    return out;
}

