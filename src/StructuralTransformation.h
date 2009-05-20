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


#ifndef StructuralTransformationH
#define StructuralTransformationH

#include  <set>

#include  "Transformation.h"

class DataStream;

//===================================================================================================================


// Searches for unknown streams following audio streams. If found, it toggles each bit of the 4 bytes of such a stream, to see if this leads to a compatible frame. If it does, it transforms the file by actually toggling that bit
class SingleBitRepairer : public Transformation
{
    static bool isUnknown(const DataStream*);
public:
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Sometimes a bit gets flipped in a file. This tries to identify places having this issue in audio frame headers. If found, it fixes the problem. It is most likely to apply to files that have 2 audio streams."; }

    static const char* getClassName() { return "Restore flipped bit in audio"; }
};


//===================================================================================================================



// base class for stream removal
class GenericRemover : public Transformation
{
    virtual bool matches(DataStream*) const = 0; // if this returns true the stream must be removed
public:
    virtual ~GenericRemover() {}
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
};





// removes all streams between audio streams
class InnerNonAudioRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
    std::set<DataStream*> m_spStreamsToDiscard;
    void setupDiscarded(const Mp3Handler& h);
public:
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes all non-audio data that is found between audio streams. In this context, VBRI streams are considered audio streams (while Xing streams are not.)"; }

    static const char* getClassName() { return "Remove inner non-audio"; }
};



class UnknownDataStreamRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes all unknown streams."; }

    static const char* getClassName() { return "Remove unknown streams"; }
};

class BrokenDataStreamRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes all broken streams."; }

    static const char* getClassName() { return "Remove broken streams"; }
};

class UnsupportedDataStreamRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes all unsupported streams."; }

    static const char* getClassName() { return "Remove unsupported streams"; }
};

class TruncatedMpegDataStreamRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes all truncated audio streams."; }

    static const char* getClassName() { return "Remove truncated audio streams"; }
};

class NullStreamRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes all null streams."; }

    static const char* getClassName() { return "Remove null streams"; }
};



#if 0
class StreamRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes all streams that are broken, truncated, unsupported or have some errors making them unusable."; }

    static const char* getClassName() { return "General cleanup"; }
};
#endif




class BrokenId3V2Remover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes broken Id3V2 streams."; }

    static const char* getClassName() { return "Remove broken Id3V2 streams"; }
};


class UnsupportedId3V2Remover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Removes unsupported Id3V2 streams."; }

    static const char* getClassName() { return "Remove unsupported ID3V2 streams"; }
};



class MultipleId3StreamRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
    std::set<DataStream*> m_spStreamsToDiscard;
    void setupDiscarded(const Mp3Handler& h);
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "If a file has multiple ID3 streams it keeps only the last ID3V1 and the first ID3V2 stream."; }

    static const char* getClassName() { return "Remove multiple ID3 streams"; }

    /*override*/ Transformation::Result apply(const Mp3Handler& h, const TransfConfig& cfg, const std::string& strOrigSrcName, std::string& strTempName)
    {
        setupDiscarded(h);
        return GenericRemover::apply(h, cfg, strOrigSrcName, strTempName);
    }
};


class MismatchedXingRemover : public GenericRemover
{
    /*override*/ bool matches(DataStream* p) const;
    std::set<DataStream*> m_spStreamsToDiscard;
    void setupDiscarded(const Mp3Handler& h);
public:
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Sometimes the number of frames in an audio stream is different from the number of frames in a preceding Xing (or Lame) header, usually because the audio stream was damaged. It's probably best for the Xing header to be removed in this case. If the audio is VBR, you may want to try \"Repair VBR data\" first."; }

    static const char* getClassName() { return "Remove mismatched Xing headers"; }

    /*override*/ Transformation::Result apply(const Mp3Handler& h, const TransfConfig& cfg, const std::string& strOrigSrcName, std::string& strTempName)
    {
        setupDiscarded(h);
        return GenericRemover::apply(h, cfg, strOrigSrcName, strTempName);
    }
};


//===================================================================================================================


class TruncatedAudioPadder : public Transformation
{
public:
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "Pads truncated audio frames with 0 to the right. Its usefulness hasn't been determined yet (it might be quite low)."; }

    static const char* getClassName() { return "Pad truncated audio"; }
};


//===================================================================================================================


class VbrRepairerBase : public Transformation
{
protected:
    enum { DONT_FORCE_REBUILD, FORCE_REBUILD };
    Transformation::Result repair(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName, bool bForceRebuild);
};

class VbrRepairer : public VbrRepairerBase
{
public:
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "If a file contains VBR audio but doesn't have a Xing header, one such header is added. If a VBRI header exists, it is removed. If a Xing header exists but is determined to be incorrect, it is corrected or replaced. Only the first audio stream is considered; if a file contains more than one audio stream, this should be fixed first."; }

    static const char* getClassName() { return "Repair VBR data"; }
};


class VbrRebuilder : public VbrRepairerBase
{
public:
    /*override*/ Transformation::Result apply(const Mp3Handler&, const TransfConfig&, const std::string& strOrigSrcName, std::string& strTempName);
    /*override*/ const char* getActionName() const { return getClassName(); }
    /*override*/ const char* getDescription() const { return "If a file contains VBR audio, any existing VBRI or Xing headers are removed and a new Xing header is created. Only the first audio stream is considered; if a file contains more than one audio stream, this should be fixed first."; }

    static const char* getClassName() { return "Rebuild VBR data"; }
};





#endif // #ifndef StructuralTransformationH
