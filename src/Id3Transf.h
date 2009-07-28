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


#ifndef Id3TransfH
#define Id3TransfH

#include  "fstream_unicode.h"

#include  "Transformation.h"


class CommonData;

class Id3V2StreamBase;

class Id3V2Cleaner : public Transformation
{
    bool processId3V2Stream(Id3V2StreamBase& frm, ofstream_utf8& out);
    CommonData* m_pCommonData;
public:
    Id3V2Cleaner(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes all ID3V2 frames that aren't used by MP3 Diags. You normally don't want to do such a thing, but it may help if some other program misbehaves because of invalid or unknown frames in an ID3V2 tag. Note that this only keeps the first image in a file that has several images."; }

    static const char* getClassName() { return "Remove non-basic ID3V2 frames"; }
};


class Id3V2Rescuer : public Transformation
{
    bool processId3V2Stream(Id3V2StreamBase& frm, ofstream_utf8* pOut); // nothing gets written if pOut is 0
    CommonData* m_pCommonData;
public:
    Id3V2Rescuer(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Copies only ID3V2 frames that seem valid, discarding those that are invalid (e.g. an APIC frame claiming to hold a picture although it doesn't.) Handles both loadable and broken ID3V2 tags, in the latter case copying being stopped when a fatal error occurs."; }

    static const char* getClassName() { return "Discard invalid ID3V2 data"; }
};



class Id3V2UnicodeTransformer : public Transformation
{
    void processId3V2Stream(Id3V2StreamBase& frm, ofstream_utf8& out);
    CommonData* m_pCommonData;
public:
    Id3V2UnicodeTransformer(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Transforms text frames in ID3V2 encoded as Latin1 to Unicode (UTF16.) The reason to do this is that sometimes non-conforming software treats these frames as they are encoded in a different code page, causing other programs to display unexpected data."; }

    static const char* getClassName() { return "Convert non-ASCII ID3V2 text frames to Unicode"; }
};



class Id3V2CaseTransformer : public Transformation
{
    bool processId3V2Stream(Id3V2StreamBase& frm, ofstream_utf8& out);
    CommonData* m_pCommonData;
public:
    Id3V2CaseTransformer(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Transforms the case of text frames in ID3V2 tags, according to global settings."; }

    static const char* getClassName() { return "Change case for ID3V2 text frames"; }
};


class Id3V1Stream;

class Id3V1ToId3V2Copier : public Transformation
{
    bool processId3V2Stream(Id3V2StreamBase& frm, ofstream_utf8& out, Id3V1Stream* pId3V1Stream);
    CommonData* m_pCommonData;
public:
    Id3V1ToId3V2Copier(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Copies frames from ID3V1 to ID3V2 if those tags don't exist in the destination or if the destination doesn't exist at all."; }

    static const char* getClassName() { return "Copy missing ID3V2 frames from ID3V1"; }
};


class Id3V2ComposerAdder : public Transformation
{
    CommonData* m_pCommonData;
public:
    Id3V2ComposerAdder(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Adds the value of the composer field to the beginning of the artist field in ID3V2 frames. Useful for players that don't use the composer field."; }

    static const char* getClassName() { return "Add composer field to the artist field in ID3V2 frames"; }
};


class Id3V2ComposerRemover : public Transformation
{
    CommonData* m_pCommonData;
public:
    Id3V2ComposerRemover(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "\"Undo\" for \"adding composer field.\" Removes the value of the composer field from the beginning of the artist field in ID3V2 frames, if it was previously added."; }

    static const char* getClassName() { return "Remove composer field from the artist field in ID3V2 frames"; }
};


class Id3V2ComposerCopier : public Transformation
{
    CommonData* m_pCommonData;
public:
    Id3V2ComposerCopier(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Copies to the \"Composer\" field the beginning of an \"Artist\" field that is formatted as \"Composer [Artist]\". Does nothing if the \"Artist\" field doesn't have this format."; }

    static const char* getClassName() { return "Fill in composer field based on artist in ID3V2 frames"; }
};


class SmallerImageRemover : public Transformation
{
    CommonData* m_pCommonData;
public:
    SmallerImageRemover(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Keeps only the biggest (and supposedly the best) image in a file. The image type is set to Front Cover. (This may result in the replacement of the Front Cover image.)"; }

    static const char* getClassName() { return "Make the largest image \"Front Cover\" and remove the rest"; }
};



class Id3V2Expander : public Transformation
{
    CommonData* m_pCommonData;
public:
    Id3V2Expander(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Add extra spacing to the ID3V2 tag. This allows subsequent saving from the tag editor to complete quicker."; }

    static const char* getClassName() { return "Reserve space in ID3V2 for fast tag editing"; }
    static const int EXTRA_SPACE; // this gets added to whatever the current frames alrady occupy;
};


class Id3V2Compactor : public Transformation
{
    CommonData* m_pCommonData;
public:
    Id3V2Compactor(CommonData* pCommonData) : m_pCommonData(pCommonData) {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes large unused blocks from ID3V2 tags. (Usually these have been reserved for fast tag editing.)"; }

    static const char* getClassName() { return "Remove extra space from ID3V2"; }
};




#endif // ifndef Id3TransfH
