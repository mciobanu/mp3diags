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


#include  "fstream_unicode.h"

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


// ttt2 another option: before a group of transforms is executed on a group of files, a function is called on transforms to allow them to ask config info. most won't use this ...



bool Id3V2Cleaner::processId3V2Stream(Id3V2StreamBase& strm, ofstream_utf8& out)
{
    Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), 0, strm.getFileName()); //ttt2 unify code with Id3V2Rescuer
    vector<const Id3V2Frame*> v (strm.getKnownFrames());
    for (int i = 0; i < cSize(v); ++i)
    {
        const Id3V2Frame* pFrm (v[i]);

        if ('T' == pFrm->m_szName[0])
        { // text frame
            try
            {
                string s (pFrm->getRawUtf8String());
                if (!s.empty())
                {
                    wrt.addTextFrame(pFrm->m_szName, s);
                }
            }
            catch (const Id3V2Frame::UnsupportedId3V2Frame&)
            { // add unchanged
                wrt.addNonOwnedFrame(pFrm);
            }
        }
        else if (0 == strcmp(KnownFrames::LBL_IMAGE(), pFrm->m_szName))
        {
            CB_ASSERT (Id3V2Frame::NO_APIC != pFrm->m_eApicStatus);
            if (Id3V2Frame::COVER == pFrm->m_eApicStatus || Id3V2Frame::NON_COVER == pFrm->m_eApicStatus) // not sure about link; OTOH going to the tab editor will get rid of links, so we do it here as well
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

    ifstream_utf8 in (h.getName().c_str(), ios::binary);


    bool bChanged (false);
    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
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







// APIC: if it can be loaded or is unsupported - keep; on error - dump
// remove empty frames
// rewrite text frames to discard null terminators
bool Id3V2Rescuer::processId3V2Stream(Id3V2StreamBase& strm, ofstream_utf8* pOut) // nothing gets written if pOut is 0
{
    Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), 0, strm.getFileName());

    const vector<Id3V2Frame*>& vpFrames (strm.getFrames());

    for (int i = 0, n = cSize(vpFrames); i < n; ++i)
    {
        const Id3V2Frame* pFrm (vpFrames[i]);

        if ('T' == pFrm->m_szName[0])
        { // text frame
            try
            {
                string s (pFrm->getRawUtf8String());
                if (!s.empty())
                {
                    wrt.addTextFrame(pFrm->m_szName, s);
                }
            }
            catch (const Id3V2Frame::UnsupportedId3V2Frame&)
            { // add unchanged
                wrt.addNonOwnedFrame(pFrm);
            }
        }
        else if (0 == strcmp(KnownFrames::LBL_IMAGE(), pFrm->m_szName))
        {
            CB_ASSERT (Id3V2Frame::NO_APIC != pFrm->m_eApicStatus);
            if (Id3V2Frame::COVER == pFrm->m_eApicStatus || Id3V2Frame::NON_COVER == pFrm->m_eApicStatus) // not sure about link; OTOH going to the tab editor will get rid of links, so we do it here as well
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

    if (0 != pOut) { wrt.write(*pOut); }

    return !wrt.contentEqualTo(&strm);
}

//ttt2 consider empty frames; or identifying valid id3v2 frames instead of stopping at first error, also looking in next unknown streams;

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

    ifstream_utf8 in (h.getName().c_str(), ios::binary);

    {
        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));
            BrokenDataStream* pBrk (dynamic_cast<BrokenDataStream*>(p));

            if (0 != pId3V2)
            {
                if (processId3V2Stream(*pId3V2, 0)) { goto e1; }
            }
            else if (0 != pBrk && (pBrk->getBaseName() == Id3V230Stream::getClassDisplayName() || pBrk->getBaseName() == Id3V240Stream::getClassDisplayName()))
            {
                goto e1;
            }
        }

        return NOT_CHANGED;
    }

e1:

    bool bChanged (false);
    bool bRecall (false);

    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
        in.seekg(0);

        for (int i = 0, n = cSize(vpStreams); i < n; ++i)
        {
            DataStream* p (vpStreams[i]);
            Id3V2StreamBase* pId3V2 (dynamic_cast<Id3V2StreamBase*>(p));
            BrokenDataStream* pBrk (dynamic_cast<BrokenDataStream*>(p));

            if (0 != pId3V2)
            {
                bool b (processId3V2Stream(*pId3V2, &out));
                bChanged = bChanged || b;
            }
            else if (0 != pBrk && pBrk->getBaseName() == Id3V230Stream::getClassDisplayName())
            {
                in.seekg(pBrk->getPos());
                NoteColl notes (20);
                StringWrp fileName (h.getName());
                Id3V230Stream strm (0, notes, in, &fileName, Id3V230Stream::ACCEPT_BROKEN);
                Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), &strm, h.getName());
                wrt.write(out);
                bChanged = true;
                bRecall = true; // !!! now we read whatever frames are available from a broken stream, next time we check for empty or otherwise invalid frames
            }
            else if (0 != pBrk && pBrk->getBaseName() == Id3V240Stream::getClassDisplayName())
            {
                in.seekg(pBrk->getPos());
                NoteColl notes (20);
                StringWrp fileName (h.getName());
                Id3V240Stream strm (0, notes, in, &fileName, Id3V240Stream::ACCEPT_BROKEN);
                Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), &strm, h.getName()); //ttt2 if useFastSave is true there should probably be an automatic reload; OTOH we may want to delay until more transforms are applied, so probably it's OK as is
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


/*override*/ const char* Id3V2UnicodeTransformer::getVisibleActionName() const
{
    string strActionName (string("Convert non-ASCII ID3V2 text frames to Unicode assuming codepage ") + m_pCommonData->m_pCodec->name().constData());
    if (strActionName != m_strActionName)
    {
        m_strActionName = strActionName; // to make sure that pointer comparisons still work (though they should probably be replaced by string comparisons) //ttt2 replace ptr comparisons
    }
    return m_strActionName.c_str();
}



void Id3V2UnicodeTransformer::processId3V2Stream(Id3V2StreamBase& strm, ofstream_utf8& out)
{
    Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), 0, strm.getFileName());

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

    ifstream_utf8 in (h.getName().c_str(), ios::binary);

    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
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
            cout << i << " " << q.toUtf8().constData() << " " << (int)c.category() << endl;
        }
    }
};


PLPAS llpwwe;
*/



/*override*/ const char* Id3V2CaseTransformer::getVisibleActionName() const
{
    string strActionName (string("Change case for ID3V2 text frames: Artists - ") + getCaseAsStr(m_pCommonData->m_eCaseForArtists) + "; Others - " + getCaseAsStr(m_pCommonData->m_eCaseForOthers));

    if (strActionName != m_strActionName)
    {
        m_strActionName = strActionName; // to make sure that pointer comparisons still work (though they should probably be replaced by string comparisons) //ttt2 replace ptr comparisons
    }
    return m_strActionName.c_str();
}


bool Id3V2CaseTransformer::processId3V2Stream(Id3V2StreamBase& strm, ofstream_utf8& out)
{
    Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), &strm, strm.getFileName());

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_ARTIST()));
        if (0 != pFrm)
        {
            QString s (convStr(pFrm->getUtf8String()));
            wrt.addTextFrame(KnownFrames::LBL_ARTIST(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForArtists)));
        }
    }

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_ALBUM()));
        if (0 != pFrm)
        {
            QString s (convStr(pFrm->getUtf8String()));
            wrt.addTextFrame(KnownFrames::LBL_ALBUM(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForOthers)));
        }
    }

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_COMPOSER()));
        if (0 != pFrm)
        {
            QString s (convStr(pFrm->getUtf8String()));
            wrt.addTextFrame(KnownFrames::LBL_COMPOSER(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForArtists)));
        }
    }

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_TITLE()));
        if (0 != pFrm)
        {
            QString s (convStr(pFrm->getUtf8String()));
            wrt.addTextFrame(KnownFrames::LBL_TITLE(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForOthers)));
        }
    }

    {
        const Id3V2Frame* pFrm (strm.getFrame(KnownFrames::LBL_GENRE()));
        if (0 != pFrm)
        {
            QString s (convStr(pFrm->getUtf8String()));
            wrt.addTextFrame(KnownFrames::LBL_GENRE(), convStr(getCaseConv(s, m_pCommonData->m_eCaseForOthers)));
        }
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

    ifstream_utf8 in (h.getName().c_str(), ios::binary);


    bool bChanged (false);
    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
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




bool Id3V1ToId3V2Copier::processId3V2Stream(Id3V2StreamBase& strm, ofstream_utf8& out, Id3V1Stream* pId3V1Stream)
{
    Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), &strm, strm.getFileName());

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
        Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), 0, h.getName());

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

        ifstream_utf8 in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
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

    ifstream_utf8 in (h.getName().c_str(), ios::binary);

    bool bChanged (false);
    { // temp
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
        in.seekg(0); // ttt2 doesn't seem needed; search for similar cases and perhaps remove them all

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
        Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), pId3V2, h.getName());
        wrt.addTextFrame(KnownFrames::LBL_ARTIST(), strComp + " [" + strArtist + "]");

        ifstream_utf8 in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
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
        Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), pId3V2, h.getName());
        //wrt.addTextFrame(KnownFrames::LBL_ARTIST(), strComp + " [" + strArtist + "]");
        wrt.addTextFrame(KnownFrames::LBL_ARTIST(), strArtist.substr(strComp.size() + 2, strArtist.size() - strComp.size() - 3));

        ifstream_utf8 in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
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
        Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), pId3V2, h.getName());
        wrt.addTextFrame(KnownFrames::LBL_COMPOSER(), strComp);

        ifstream_utf8 in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
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



/*override*/ Transformation::Result SmallerImageRemover::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName) //ttt1 recompress large images
{
    LAST_STEP("SmallerImageRemover::apply() " + h.getName());

    const vector<DataStream*>& vpStreams (h.getStreams());
    Id3V2StreamBase* pId3V2 (0);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        DataStream* p (vpStreams[i]);
        pId3V2 = dynamic_cast<Id3V2StreamBase*>(p);
        if (0 != pId3V2) { break; }
    }

    if (0 == pId3V2) { return NOT_CHANGED; }

    const vector<Id3V2Frame*>& vpFrames (pId3V2->getFrames());

    const Id3V2Frame* pLargestPic (0);
    int nPicCnt (0);

    for (int i = 0; i < cSize(vpFrames); ++i)
    {
        const Id3V2Frame* p (vpFrames[i]);
        //if (0 == strcmp(p->m_szName, KnownFrames::LBL_IMAGE()))
        if (Id3V2Frame::COVER == p->m_eApicStatus || Id3V2Frame::NON_COVER == p->m_eApicStatus)
        {
            ++nPicCnt;
            if (0 == pLargestPic || pLargestPic->m_nImgSize < p->m_nImgSize)
            {
                pLargestPic = p;
            }
        }
    }

    if (0 == nPicCnt || (1 == nPicCnt && Id3V2Frame::PT_COVER == pLargestPic->m_nPictureType)) { return NOT_CHANGED; }



    { // temp
        Id3V230StreamWriter wrt (Id3V230StreamWriter::KEEP_ONE_VALID_IMG, m_pCommonData->useFastSave(), pId3V2, h.getName());

        wrt.removeFrames(KnownFrames::LBL_IMAGE());

        Id3V2FrameDataLoader ldr (*pLargestPic);
        vector<char> v;
        copy (ldr.getData(), ldr.getData() + pLargestPic->m_nMemDataSize, back_inserter(v));

        //wrt.addBinaryFrame(KnownFrames::LBL_IMAGE(), v);
        wrt.addImg(v);

        ifstream_utf8 in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
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


/*static*/ const int Id3V2Expander::EXTRA_SPACE (4096); // frames other than APIC; title, artist, lyrics, ...

/*override*/ Transformation::Result Id3V2Expander::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
{
    const vector<DataStream*>& vpStreams (h.getStreams());
    Id3V2StreamBase* pId3V2 (0);

    for (int i = 0, n = cSize(vpStreams); i < n; ++i)
    {
        DataStream* p (vpStreams[i]);
        pId3V2 = dynamic_cast<Id3V2StreamBase*>(p);
        if (0 != pId3V2) { break; }
    }

    //if (0 == pId3V2) { return NOT_CHANGED; }
//(int(pId3V2->getSize())
    int nOldPaddingSize (0), nOldSize (0);
    if (0 != pId3V2)
    {
        nOldPaddingSize = pId3V2->getPaddingSize();
        nOldSize = int(pId3V2->getSize());
    }

    int nExtraSize (ImageInfo::MAX_IMAGE_SIZE + EXTRA_SPACE); // it is possible for existing pictures with non-cover types to be kept, so we want additional space even if there is already a (big) image // !!! cannot just remove the size of whatever APIC is currently used, because it may be of a non-cover type
    if (nExtraSize <= nOldPaddingSize) { return NOT_CHANGED; }

    { // temp
        Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), pId3V2, h.getName());

        ifstream_utf8 in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
        wrt.write(out, nOldSize + nExtraSize - nOldPaddingSize); // may throw, but it will be caught

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



/*override*/ Transformation::Result Id3V2Compactor::apply(const Mp3Handler& h, const TransfConfig& transfConfig, const std::string& strOrigSrcName, std::string& strTempName)
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

    if (pId3V2->getPaddingSize() < Id3V230StreamWriter::DEFAULT_EXTRA_SPACE + 512) { return NOT_CHANGED; }

    { // temp
        Id3V230StreamWriter wrt (m_pCommonData->m_bKeepOneValidImg, m_pCommonData->useFastSave(), pId3V2, h.getName());

        ifstream_utf8 in (h.getName().c_str(), ios::binary);
        transfConfig.getTempName(strOrigSrcName, getActionName(), strTempName);
        ofstream_utf8 out (strTempName.c_str(), ios::binary);
        wrt.write(out, 0); // may throw, but it will be caught

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



//ttt2 warn in config if user enables fast save and then hides Id3V2Compactor
