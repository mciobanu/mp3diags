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


#include  <fstream>

#include  <QTextCodec>

#include  "Id3Transf.h"

#include  "Id3V230Stream.h"
#include  "Id3V240Stream.h"
#include  "CommonData.h"
#include  "Mp3Manip.h"
#include  "Helpers.h"
#include  "MpegStream.h"
#include  "OsFile.h"

//#include  <iostream>

using namespace std;


//ttt2 remove id3v1; copy id3v2 to id3v1; copy id3v1 to id3v2;


// ttt1 another option: before a group of transforms is executed on a group of files, a function is called on transforms to allow them to ask config info. most won't use this ...



bool Id3V2Cleaner::processId3V2Stream(Id3V2StreamBase& strm, ofstream& out)
{
    Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg);
    vector<const Id3V2Frame*> v (strm.getKnownFrames());
    for (int i = 0; i < cSize(v); ++i)
    {
        wrt.addNonOwnedFrame(v[i]);
    }

    TagTimestamp time (strm.getTime());
    wrt.setRecTime(time); // !!! needed to make sure the 2.3.0 format is used

    wrt.write(out);

    return !wrt.contentEqualTo(&strm);
}


/*override*/ Transformation::Result Id3V2Cleaner::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    ValueRestorer<string> rst (strTempName);

    const vector<DataStream*>& vpStreams (h.getStreams());
    bool bFoundMatch (false);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        if (0 != dynamic_cast<Id3V2StreamBase*>(vpStreams[i]))
        {
            bFoundMatch = true;
            break;
        }
    }

    if (!bFoundMatch)
    {
        return NOT_CHANGED;
    }

    ifstream in (h.getName().c_str(), ios::binary);


    bool bChanged (false);
    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));

            if (0 != pId3V2)
            {
                bool b (processId3V2Stream(*pId3V2, out));
                bChanged = bChanged || b;
            }
            else
            {
                p->copy(in, out);
            }
        }
    }

    if (bChanged)
    {
        rst.setOk(true);
        return CHANGED_NO_RECALL;
    }

    deleteFile(strTempName);
    return NOT_CHANGED;
}


//========================================================================================================================
//========================================================================================================================
//========================================================================================================================




//ttt1 perhaps if there's only one APIC with some unsupported type convert it to "front cover"


// APIC: if it can be loaded or is unsupported - keep; on error - dump
// remove empty frames
// rewrite text frames to discard null terminators
bool Id3V2Rescuer::processId3V2Stream(Id3V2StreamBase& strm, ofstream& out)
{
    Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg);

    const vector<Id3V2Frame*>& vpFrames (strm.getFrames());

    for (int i = 0, n = cSize(vpFrames); i < n; ++i)
    {
        const Id3V2Frame* pFrm (vpFrames[i]);
        Id3V2FrameDataLoader wrp (*pFrm);

        if ('T' == pFrm->m_szName[0] && KnownFrames::getKnownFrames().count(pFrm->m_szName) > 0)
        { // text frame
            bool bDiscard (false);
            try
            {
                string s (pFrm->getUtf8String());
                bDiscard = s.empty();
            }
            catch (const Id3V2Frame::UnsupportedId3V2Frame&)
            { // !!! nothing to be done here; the frame will get added unchanged
            }

            if (!bDiscard)
            {
                wrt.addNonOwnedFrame(pFrm);
            }
        }
        else if (0 == strcmp(KnownFrames::LBL_IMAGE(), pFrm->m_szName))
        {
            CB_ASSERT (Id3V2Frame::NO_APIC != pFrm->m_eApicStatus);
            if (Id3V2Frame::ERROR != pFrm->m_eApicStatus)
            {
                wrt.addNonOwnedFrame(pFrm);
            }
        }
        else if (pFrm->m_nMemDataSize > 0)
        {
            wrt.addNonOwnedFrame(pFrm);
        }
    }

    TagTimestamp time (strm.getTime());
    wrt.setRecTime(time);

    wrt.write(out);

    return !wrt.contentEqualTo(&strm);
}



/*override*/ Transformation::Result Id3V2Rescuer::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    ValueRestorer<string> rst (strTempName);

    const vector<DataStream*>& vpStreams (h.getStreams());
    bool bFoundMatch (false);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        if (0 != dynamic_cast<Id3V2StreamBase*>(vpStreams[i]) || 0 != dynamic_cast<BrokenDataStream*>(vpStreams[i]))
        {
            bFoundMatch = true;
            break;
        }
    }

    if (!bFoundMatch)
    {
        return NOT_CHANGED;
    }

    ifstream in (h.getName().c_str(), ios::binary);

    bool bChanged (false);
    bool bRecall (false);

    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));
            BrokenDataStream* pBrk (dynamic_cast<BrokenDataStream*>(p));

            if (0 != pId3V2)
            {
                bool b (processId3V2Stream(*pId3V2, out));
                bChanged = bChanged || b;
            }
            else if (0 != pBrk && pBrk->getBaseName() == Id3V230Stream::getClassDisplayName())
            {
                NoteColl notes (20);
                StringWrp fileName (h.getName());
                Id3V230Stream strm (0, notes, in, &fileName, Id3V230Stream::ACCEPT_BROKEN);
                Id3V230StreamWriter wrt (&strm, m_pCommonData->m_bKeepOneValidImg);
                wrt.write(out);
                bChanged = true;
                bRecall = true; // !!! now we read whatever frames are available from a broken stream, next time we check for empty or otherwise invalid frames
            }
            else if (0 != pBrk && pBrk->getBaseName() == Id3V240Stream::getClassDisplayName())
            {
                NoteColl notes (20);
                StringWrp fileName (h.getName());
                Id3V240Stream strm (0, notes, in, &fileName, Id3V230Stream::ACCEPT_BROKEN);
                Id3V230StreamWriter wrt (&strm, m_pCommonData->m_bKeepOneValidImg);
                wrt.write(out);
                bChanged = true;
                bRecall = true;
            }
            else
            {
                p->copy(in, out);
            }
        }
    }

    if (bChanged)
    {
        rst.setOk(true);
        return bRecall ? CHANGED : CHANGED_NO_RECALL;
    }

    deleteFile(strTempName);
    return NOT_CHANGED;
}



//========================================================================================================================
//========================================================================================================================
//========================================================================================================================



void Id3V2UnicodeTransformer::processId3V2Stream(Id3V2StreamBase& strm, ofstream& out)
{
    Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg);

    const vector<Id3V2Frame*>& vpFrames (strm.getFrames());

    for (int i = 0, n = cSize(vpFrames); i < n; ++i)
    {
        const Id3V2Frame* pFrm (vpFrames[i]);
        //Id3V2FrameDataLoader wrp (*pFrm);

        if ('T' == pFrm->m_szName[0] && pFrm->m_bHasLatin1NonAscii)
        {
            Id3V2FrameDataLoader wrp (*pFrm);
            const char* pData (wrp.getData());
            CB_ASSERT (0 == pData[0]); // "Latin1" encoding
            QByteArray arr (pData + 1, pFrm->m_nMemDataSize - 1);
            QString qstrTxt (m_pCommonData->m_pCodec->toUnicode(arr));
            wrt.addTextFrame(pFrm->m_szName, convStr(qstrTxt));
        }
        else
        {
            wrt.addNonOwnedFrame(pFrm); //ttt2 removes duplicates of POPM/ratings; OTOH it removes duplicates of artist or tite; well, those were supposed to not have duplicates
        }
    }

    TagTimestamp time (strm.getTime());
    wrt.setRecTime(time);

    wrt.write(out);
}



/*override*/ Transformation::Result Id3V2UnicodeTransformer::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    const vector<DataStream*>& vpStreams (h.getStreams());

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(vpStreams[i]));
        if (0 != pId3V2)
        {
            const vector<Id3V2Frame*>& vpFrames (pId3V2->getFrames());
            for (int i = 0, n = cSize(vpFrames); i < n; ++i)
            {
                const Id3V2Frame* pFrm (vpFrames[i]);
                if (pFrm->m_bHasLatin1NonAscii)
                {
                    goto e1;
                }
            }
        }
    }

    return NOT_CHANGED;

e1:

    ifstream in (h.getName().c_str(), ios::binary);

    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));

            if (0 != pId3V2)
            {
                processId3V2Stream(*pId3V2, out);
            }
            else
            {
                p->copy(in, out);
            }
        }
    }

    return CHANGED_NO_RECALL;
}




//========================================================================================================================
//========================================================================================================================
//========================================================================================================================


/*
struct PLPAS
{
    PLPAS()
    {
        for (int i = 32; i < 512; ++i)
        {
            QChar c (i);
            QString q; q += c;
            cout << i << " " << q.toUtf8().data() << " " << (int)c.category() << endl;
        }
    }
};


PLPAS llpwwe;
*/

static QString getCaseConv(const QString& s, CommonData::Case eCase)
{
/*
    lNames << "Lower case: first part. second part.";
    lNames << "Upper case: FIRST PART. SECOND PART.";
    lNames << "Title case: First Part. Second Part.";
    lNames << "Phrase case: First part. Second part.";
*/
    switch(eCase)
    {
    case CommonData::LOWER: return s.toLower();

    case CommonData::UPPER: return s.toUpper();

    case CommonData::TITLE:
        {
            int n (s.size());
            QString res;
            bool bWhitesp (true);
            for (int i = 0; i < n; ++i)
            {
                const QChar& qc (s[i]);
                if (bWhitesp)
                {
                    res += qc.toUpper();
                }
                else
                {
                    res += qc.toLower();
                }

                bWhitesp = qc.isSpace() || qc == '.';
            }
            return res;
        }

    case CommonData::PHRASE:
        {
            int n (s.size());
            QString res;
            bool bPer (true);
            for (int i = 0; i < n; ++i)
            {
                const QChar& qc (s[i]);
                if (bPer)
                {
                    res += qc.toUpper();
                }
                else
                {
                    res += qc.toLower();
                }

                if (!qc.isSpace()) { bPer = (qc == '.'); }
            }
            return res;
        }
    }

    CB_ASSERT (false);
}




bool Id3V2CaseTransformer::processId3V2Stream(Id3V2StreamBase& strm, ofstream& out)
{
    Id3V230StreamWriter wrt (&strm, m_pCommonData->m_bKeepOneValidImg);

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_ARTIST()));
        QString s (convStr(pFrm->getUtf8String()));
        wrt.addTextFrame(KnownFrames::LBL_ARTIST(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForArtists)));
    }

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_ALBUM()));
        QString s (convStr(pFrm->getUtf8String()));
        wrt.addTextFrame(KnownFrames::LBL_ALBUM(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForOthers)));
    }

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_COMPOSER()));
        QString s (convStr(pFrm->getUtf8String()));
        wrt.addTextFrame(KnownFrames::LBL_COMPOSER(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForArtists)));
    }

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_TITLE()));
        QString s (convStr(pFrm->getUtf8String()));
        wrt.addTextFrame(KnownFrames::LBL_TITLE(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForOthers)));
    }

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_GENRE()));
        QString s (convStr(pFrm->getUtf8String()));
        wrt.addTextFrame(KnownFrames::LBL_GENRE(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForOthers)));
    }

    wrt.write(out);

    return !wrt.contentEqualTo(&strm);
}


/*override*/ Transformation::Result Id3V2CaseTransformer::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{ //ttt2 identical / similar code to the other Id3V2 Transformers
    ValueRestorer<string> rst (strTempName);

    const vector<DataStream*>& vpStreams (h.getStreams());
    bool bFoundMatch (false);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        if (0 != dynamic_cast<Id3V2StreamBase*>(vpStreams[i]))
        {
            bFoundMatch = true;
            break;
        }
    }

    if (!bFoundMatch)
    {
        return NOT_CHANGED;
    }

    ifstream in (h.getName().c_str(), ios::binary);


    bool bChanged (false);
    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));

            if (0 != pId3V2)
            {
                bool b (processId3V2Stream(*pId3V2, out));
                bChanged = bChanged || b;
            }
            else
            {
                p->copy(in, out);
            }
        }
    }

    if (bChanged)
    {
        rst.setOk(true);
        return CHANGED_NO_RECALL;
    }

    deleteFile(strTempName);
    return NOT_CHANGED;
}


//========================================================================================================================
//========================================================================================================================
//========================================================================================================================




bool Id3V1ToId3V2Copier::processId3V2Stream(Id3V2StreamBase& strm, ofstream& out, Id3V1Stream* pId3V1Stream)
{
    Id3V230StreamWriter wrt (&strm, m_pCommonData->m_bKeepOneValidImg);

    if (strm.getTitle().empty())
    {
        string s (pId3V1Stream->getTitle());
        if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_TITLE(), s); }
    }

    if (strm.getArtist().empty())
    {
        string s (pId3V1Stream->getArtist());
        if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_ARTIST(), s); }
    }

    if (strm.getTrackNumber().empty())
    {
        string s (pId3V1Stream->getTrackNumber());
        if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_TRACK_NUMBER(), s); }
    }

    if (0 == strm.getTime().asString()[0])
    {
        string s (pId3V1Stream->getTime().getYear());
        if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_TIME_YEAR_230(), s); }
    }

    if (strm.getGenre().empty())
    {
        string s (pId3V1Stream->getGenre());
        if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_GENRE(), s); }
    }

    if (strm.getAlbumName().empty())
    {
        string s (pId3V1Stream->getAlbumName());
        if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_ALBUM(), s); }
    }

    wrt.write(out);

    return !wrt.contentEqualTo(&strm);
}



/*override*/ Transformation::Result Id3V1ToId3V2Copier::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    ValueRestorer<string> rst (strTempName);

    const vector<DataStream*>& vpStreams (h.getStreams());
    bool bId3V2Found (false);
    Id3V1Stream* pId3V1Stream (0);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        if (0 != dynamic_cast<Id3V2StreamBase*>(vpStreams[i]))
        {
            bId3V2Found = true;
        }
        else if (0 != dynamic_cast<Id3V1Stream*>(vpStreams[i]))
        {
            pId3V1Stream = dynamic_cast<Id3V1Stream*>(vpStreams[i]);
        }
    }

    if (0 == pId3V1Stream)
    {
        return NOT_CHANGED;
    }

    if (!bId3V2Found)
    {
        Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg);

        {
            string s (pId3V1Stream->getTitle());
            if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_TITLE(), s); }
        }

        {
            string s (pId3V1Stream->getArtist());
            if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_ARTIST(), s); }
        }

        {
            string s (pId3V1Stream->getTrackNumber());
            if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_TRACK_NUMBER(), s); }
        }

        {
            string s (pId3V1Stream->getTime().getYear());
            if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_TIME_YEAR_230(), s); }
        }

        {
            string s (pId3V1Stream->getGenre());
            if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_GENRE(), s); }
        }

        {
            string s (pId3V1Stream->getAlbumName());
            if (!s.empty()) { wrt.addTextFrame(KnownFrames::LBL_ALBUM(), s); }
        }

        if (wrt.isEmpty())
        {
            return NOT_CHANGED;
        }

        ifstream in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        in.seekg(0);
        wrt.write(out);
        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            p->copy(in, out);
        }

        rst.setOk(true);
        return CHANGED_NO_RECALL;
    }

    ifstream in (h.getName().c_str(), ios::binary);

    bool bChanged (false);
    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        in.seekg(0); // ttt1 see if still needed

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));

            if (0 != pId3V2)
            {
                bool b (processId3V2Stream(*pId3V2, out, pId3V1Stream));
                bChanged = bChanged || b;
            }
            else
            {
                p->copy(in, out);
            }
        }
    }

    if (bChanged)
    {
        rst.setOk(true);
        return CHANGED_NO_RECALL;
    }

    deleteFile(strTempName);
    return NOT_CHANGED;
}



//========================================================================================================================
//========================================================================================================================
//========================================================================================================================



/*override*/ Transformation::Result Id3V2ComposerAdder::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    const vector<DataStream*>& vpStreams (h.getStreams());
    Id3V2StreamBase* pId3V2 (0);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        DataStream* p (vpStreams[i]);
        pId3V2 = dynamic_cast<Id3V2StreamBase*>(p);
        if (0 != pId3V2) { break; }
    }

    if (0 == pId3V2) { return NOT_CHANGED; }

    string strComp (pId3V2->getComposer());
    if (strComp.empty()) { return NOT_CHANGED; }
    string strArtist (pId3V2->getArtist());
    if (beginsWith(strArtist, strComp + " [") && endsWith(strArtist, "]")) { return NOT_CHANGED; }

    { // temp
        Id3V230StreamWriter wrt (pId3V2, m_pCommonData->m_bKeepOneValidImg);
        wrt.addTextFrame(KnownFrames::LBL_ARTIST(), strComp + " [" + strArtist + "]");

        ifstream in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        wrt.write(out); // may throw, but it will be caught

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);

            if (p != pId3V2)
            {
                p->copy(in, out);
            }
        }
    }

    return CHANGED_NO_RECALL;
}



//========================================================================================================================
//========================================================================================================================
//========================================================================================================================



/*override*/ Transformation::Result Id3V2ComposerRemover::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    const vector<DataStream*>& vpStreams (h.getStreams());
    Id3V2StreamBase* pId3V2 (0);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        DataStream* p (vpStreams[i]);
        pId3V2 = dynamic_cast<Id3V2StreamBase*>(p);
        if (0 != pId3V2) { break; }
    }

    if (0 == pId3V2) { return NOT_CHANGED; }

    string strComp (pId3V2->getComposer());
    if (strComp.empty()) { return NOT_CHANGED; }
    string strArtist (pId3V2->getArtist());
    if (!(beginsWith(strArtist, strComp + " [") && endsWith(strArtist, "]"))) { return NOT_CHANGED; }

    { // temp
        Id3V230StreamWriter wrt (pId3V2, m_pCommonData->m_bKeepOneValidImg);
        //wrt.addTextFrame(KnownFrames::LBL_ARTIST(), strComp + " [" + strArtist + "]");
        wrt.addTextFrame(KnownFrames::LBL_ARTIST(), strArtist.substr(strComp.size() + 2, strArtist.size() - strComp.size() - 3));

        ifstream in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        wrt.write(out); // may throw, but it will be caught

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);

            if (p != pId3V2)
            {
                p->copy(in, out);
            }
        }
    }

    return CHANGED_NO_RECALL;
}


//========================================================================================================================
//========================================================================================================================
//========================================================================================================================



/*override*/ Transformation::Result Id3V2ComposerCopier::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    const vector<DataStream*>& vpStreams (h.getStreams());
    Id3V2StreamBase* pId3V2 (0);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        DataStream* p (vpStreams[i]);
        pId3V2 = dynamic_cast<Id3V2StreamBase*>(p);
        if (0 != pId3V2) { break; }
    }

    if (0 == pId3V2) { return NOT_CHANGED; }

    string strArtist (pId3V2->getArtist());
    if (beginsWith(strArtist, "[") || !endsWith(strArtist, "]")) { return NOT_CHANGED; }

    string::size_type n (strArtist.find(" ["));
    if (string::npos == n) { return NOT_CHANGED; }

    string strComp (strArtist.substr(0, n));
    string strExistingComp (pId3V2->getComposer());
    if (strComp == strExistingComp) { return NOT_CHANGED; }

    { // temp
        Id3V230StreamWriter wrt (pId3V2, m_pCommonData->m_bKeepOneValidImg);
        wrt.addTextFrame(KnownFrames::LBL_COMPOSER(), strComp);

        ifstream in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream out (strTempName.c_str(), ios::binary);
        wrt.write(out); // may throw, but it will be caught

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);

            if (p != pId3V2)
            {
                p->copy(in, out);
            }
        }
    }

    return CHANGED_NO_RECALL;
}



//========================================================================================================================
//========================================================================================================================
//========================================================================================================================


//ttt2 perhaps be able to extract composer even when the field is empty, if artist is "composer [artist]", but doesn't look too useful
//ttt1 perhaps something to discard invalid ID3V2 frames, especially invalid pictures
