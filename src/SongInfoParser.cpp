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


#include  <stack>
#include  <sstream>

#include  "SongInfoParser.h"

#include  "Helpers.h"
#include  "DataStream.h"


using namespace std;
using namespace pearl;

namespace SongInfoParser
{



//ttt2 perhaps null namespace (some classes are used in the header, though)

// helper for TrackTextParser;
//
// base class for reading fields (artist, track#, ...) from a string containing the name of a file;
// the string is passed in init(), which can be called repeatedly, without a need for new Reader objects to be created;
// subclasses: BoundReader, UnboundReader, SequenceReader and OptionalReader
struct Reader
{
    enum Optional { OPT, NON_OPT };
    enum Bound { BOUND, UNBOUND };
    enum State { UNASSIGNED, ASSIGNED, FAILED };

    enum { RIGHT, LEFT };

    Optional m_eOptional;
    Bound getBound(bool bLeft) const { return bLeft ? m_eLeftBound : m_eRightBound; }
    Bound getLeftBound() const { return m_eLeftBound; }
    Bound getRightBound() const { return m_eRightBound; }
    State m_eState;

    const char* m_pcLeft; // set during the call to init() if it returns true
    const char* m_pcRight; // set during the call to init() if it returns true

    Reader(Optional eOptional, Bound eLeftBound, Bound eRightBound) : m_eOptional(eOptional), m_eState(UNASSIGNED), m_eLeftBound(eLeftBound), m_eRightBound(eRightBound)
    {
    }

    virtual ~Reader() {}

    enum { RIGHT_FIRST, LEFT_FIRST };
    enum { MOBILE_LEFT, FIXED_LEFT };
    enum { MOBILE_RIGHT, FIXED_RIGHT };
    virtual bool init(const char* pcLeft, const char* pcRight, bool bLeftFirst, bool bFixedLeft, bool bFixedRight) = 0; // sets m_pcLeft and m_pcRight such that the field is matched; returns false if the assignment couldn't be made

    virtual void clearState() { m_eState = UNASSIGNED; }

    virtual void set(vector<string>&) {}
protected:
    Bound m_eLeftBound, m_eRightBound; // rating, year, track - know when to stop
    bool failure() { m_eState = FAILED; return false; }
    bool success() { m_eState = ASSIGNED; return true; }
private:
    Reader(const Reader&);
    Reader& operator=(const Reader&);
};



struct BoundReader : public Reader
{
    BoundReader(Optional eOptional) : Reader(eOptional, BOUND, BOUND) {}

    /*override*/ bool init(const char* pcLeft, const char* pcRight, bool bLeftFirst, bool bFixedLeft, bool bFixedRight);
private:
    virtual int checkMatch(const char* pcLeft, const char* pcRight, const char* pcStart) = 0; // returns the size of the matching string, starting at pcStart; returns -1 if no match is found
};


/*override*/ bool BoundReader::init(const char* pcLeft, const char* pcRight, bool bLeftFirst, bool bFixedLeft, bool bFixedRight)
{
    const char* p (bLeftFirst ? pcLeft : pcRight - 1);
    int nInc (bLeftFirst ? 1 : -1);
    for (; p < pcRight && p >= pcLeft; p += nInc)
    {
        int nMatch (checkMatch(pcLeft, pcRight, p));
        if (-1 != nMatch)
        {
            m_pcLeft = p;
            m_pcRight = p + nMatch;
            if ((bFixedLeft && m_pcLeft != pcLeft) || (bFixedRight && m_pcRight != pcRight))
            {
                return failure();
            }
            return success();
        }

        //if (bFixedLeft && m_pcLeft != pcLeft) || (bFixedRight && m_pcRight != pcRight)) // ttt2 suboptimal;
        /*if (bFixedLeft || bFixedRight)
        { // if either end was supposed to be fixed and the first step wasn't a match, there's no point in going to another step
            return failure(); // !!! incorrect; the reason is the "-1" in the first line; for example a static reader "ab" fixed at right would fail on "xxab", because the first test would be for "b", and only the second test for "ab"; if we fail after first test there's no chance for the second; replacing "-1" with "-2" would fix the problem in this case, but that doesn't quite work, because static readers aren't the only bound readers, and the other ones may have variable length
        }*/

        if (bLeftFirst && bFixedLeft)
        { // if the left end was supposed to be fixed and the first step wasn't a match, there's no point in going to another step; OTOH something similar cannot be done for the right side
            return failure();
        }

    }

    return failure();
}


struct StaticReader : public BoundReader
{
    StaticReader() : BoundReader(NON_OPT) {}
    string m_str;
private:
    /*override*/ int checkMatch(const char* /*pcLeft*/, const char* pcRight, const char* pcStart)
    {
        int nSize (cSize(m_str));
        if (pcRight - pcStart < nSize) { return -1; }
        string s (pcStart, nSize);
        if (s == m_str) { return nSize; }
        return -1;
    }
};


struct RatingReader : public BoundReader
{
    RatingReader() : BoundReader(NON_OPT) {}

    /*override*/ void set(vector<string>& v)
    {
        v[TagReader::RATING].assign(m_pcLeft, 1);
    }
private:
    /*override*/ int checkMatch(const char* pcLeft, const char* pcRight, const char* pcStart)
    {
        if (pcStart > pcLeft && isalnum(*(pcStart - 1))) { return -1; }
        if (pcStart < pcRight - 1 && !isdigit(*(pcStart + 1))) { return -1; }
        char c (*pcStart);
        if (c < 'a' || c > 'z') { return -1; }
        return 1;
    }
};


struct TrackNoReader : public BoundReader
{
    TrackNoReader() : BoundReader(NON_OPT) {}

    /*override*/ void set(vector<string>& v)
    {
        //v[TagReader::TRACK_NUMBER].assign(m_pcLeft, m_pcRight - m_pcLeft);
        string s;
        s.assign(m_pcLeft, m_pcRight - m_pcLeft);
        if ("00" == s) { s.clear(); }
        v[TagReader::TRACK_NUMBER] = s;
    }

private:
    enum { DIGIT_LIMIT = 2 }; //ttt2 maybe make configurable

    /*override*/ int checkMatch(const char* pcLeft, const char* pcRight, const char* pcStart)
    {
        if (!isdigit(*(pcStart)) || (pcStart > pcLeft && isdigit(*(pcStart - 1)))) { return -1; }
        const char* p (pcStart);
        for (; p < pcRight && isdigit(*p); ++p) { }
        int nCnt (p - pcStart);
        if (nCnt > DIGIT_LIMIT) { return -1; }
        return nCnt;
    }
};



struct YearReader : public BoundReader
{
    YearReader() : BoundReader(NON_OPT) {}
    /*override*/ void set(vector<string>& v)
    {
        v[TagReader::TIME].assign(m_pcLeft, 4);
    }
private:
    /*override*/ int checkMatch(const char* pcLeft, const char* pcRight, const char* pcStart)
    {
        if (!isdigit(*(pcStart)) || (pcStart > pcLeft && isdigit(*(pcStart - 1)))) { return -1; }
        const char* p (pcStart);
        for (; p < pcRight && isdigit(*p); ++p) { }
        int nCnt (p - pcStart);
        if (4 != nCnt) { return -1; }
        char q [5];
        strncpy(q, pcStart, 4); q[4] = 0;
        int nYear (atoi(q));
        if (nYear < 1900 || nYear > 2100) { return -1; }

        return nCnt;
    }
};



struct UnboundReader : public Reader
{
    UnboundReader(int nIndex) : Reader(NON_OPT, UNBOUND, UNBOUND), m_nIndex(nIndex) {}
    /*override*/ bool init(const char* pcLeft, const char* pcRight, bool /*bLeftFirst*/, bool /*bFixedLeft*/, bool /*bFixedRight*/)
    {
        m_pcLeft = pcLeft; m_pcRight = pcRight;
        m_eState = m_pcLeft != m_pcRight ? ASSIGNED : FAILED; //ttt2 see about the test; not sure if allowing an empty string is ok; //ttt2 perhaps at least %i should be allowed to be empty; maybe add constructor param to tell if empty is OK
        return ASSIGNED == m_eState;
    }

    /*override*/ void set(vector<string>& v)
    {
        v[m_nIndex].assign(m_pcLeft, m_pcRight - m_pcLeft);
    }
private:
    int m_nIndex;
};

// handles a sequence of other Readers
struct SequenceReader : public Reader
{
    struct ReaderInfo
    {
        Reader* m_pReader;
        bool m_bFromLeft;
        int m_nOrigPos;
        ReaderInfo(Reader* pReader, bool bFromLeft, int nOrigPos) : m_pReader(pReader), m_bFromLeft(bFromLeft), m_nOrigPos(nOrigPos) {}
    };


    vector<Reader*> m_vpReaders;

    SequenceReader() : Reader(NON_OPT, BOUND, BOUND) {}

    bool setBounds() // to be called after all elems have been added; returns false if there's something wrong with the elements (e.g. touching unbounded)
    {
        int nCnt (cSize(m_vpReaders));
        //CB_ASSERT (0 != nCnt); // empty sequence are not allowed;
        if (0 == nCnt) { return false; }

        /*m_eLeftBound = m_eRightBound = UNBOUND;

        if (NON_OPT == m_vpReaders[0]->m_eOptional && BOUND == m_vpReaders[0]->getLeftBound())
        {
            m_eLeftBound = BOUND;
        }

        if (NON_OPT == m_vpReaders[nCnt - 1]->m_eOptional && BOUND == m_vpReaders[nCnt - 1]->getRightBound())
        {
            m_eRightBound = BOUND;
        }*/


        m_eLeftBound = m_eRightBound = BOUND;

        // skip all opt that are bound; if any unbound found, set unbound
        for (int i = 0; i < nCnt; ++i)
        {
            if (NON_OPT == m_vpReaders[i]->m_eOptional && BOUND == m_vpReaders[i]->getLeftBound())
            {
                break;
            }

            if (UNBOUND == m_vpReaders[i]->getLeftBound())
            {
                m_eLeftBound = UNBOUND;
                break;
            }
        }

        // skip all opt that are bound; if any unbound found, set unbound
        for (int i = nCnt - 1; i >= 0; --i)
        {
            if (NON_OPT == m_vpReaders[i]->m_eOptional && BOUND == m_vpReaders[i]->getRightBound())
            {
                break;
            }

            if (UNBOUND == m_vpReaders[i]->getRightBound())
            {
                m_eRightBound = UNBOUND;
                break;
            }
        }


        // don't allow unbound ends to touch one another; optionals make it more complicated: a bound optional is ignored, while an unbound optional is considered non-optional
        bool bPrevRightBound (true);
        for (int i = 0; i < nCnt; ++i)
        {
            Reader* p (m_vpReaders[i]);
            if (!bPrevRightBound && UNBOUND == p->getLeftBound()) { return false; }

            if (NON_OPT == p->m_eOptional)
            { // non-optional
                bPrevRightBound = BOUND == p->getRightBound();
            }
            else
            { // optional
                bPrevRightBound = bPrevRightBound && (BOUND == p->getRightBound());
            }
        }

        return true;
    }

    /*override*/ void clearState()
    {
        m_eState = UNASSIGNED;
        for (int i = 0, n = cSize(m_vpReaders); i < n; ++i)
        {
            m_vpReaders[i]->clearState();
        }
    }

    /*override*/ bool init(const char* pcLeft, const char* pcRight, bool bLeftFirst, bool bFixedLeft, bool bFixedRight)
    {
        CB_ASSERT (UNASSIGNED == m_eState);

        if (m_vpReaders.empty())
        {
            if (pcLeft == pcRight)
            {
                return success();
            }
            else
            {
                return failure();
            }
        }

        m_eState = FAILED;
        m_pcLeft = pcLeft; m_pcRight = pcRight; // needed to allow findLimits() to work

        const int nCnt (cSize(m_vpReaders));
        vector<ReaderInfo> v; // ttt2 perhaps move this to setBounds(), to avoid building it every time; it seems that both a "left" and a "right" vector would be needed, since it's possible for bLeftFirst to come with either value, depending on other Readers failing or not (to be further analyzed)
        {
            int l (0);
            for (; l < nCnt; ++l) // bound at the beginning, on the left (or rather on the side indicated by bLeftFirst, but to understand the comments in this block it's easier to assume that bLeftFirst is true)
            {
                int f (bLeftFirst ? l : nCnt - 1 - l);
                Reader* p (m_vpReaders[f]);
                if (BOUND != p->getBound(!bLeftFirst)) { break; } // !!! keep adding from the left as long as there is a RIGHT bound
                v.push_back(ReaderInfo(p, bLeftFirst, f));
            }

            int r (nCnt - 1);
            for (; r > l; --r) // bound at the end, on the right
            {
                int f (bLeftFirst ? r : nCnt - 1 - r);
                Reader* p (m_vpReaders[f]);
                if (BOUND != p->getBound(bLeftFirst)) { break; }
                v.push_back(ReaderInfo(p, !bLeftFirst, f));
            }

            for (int k = l; k <= r; ++k) // all remaining statics, from the left
            {
                int f (bLeftFirst ? k : nCnt - 1 - k);
                Reader* p (m_vpReaders[f]);
                if (0 != dynamic_cast<StaticReader*>(p)) // !!! StaticReader is always bound at both sides
                {
                    v.push_back(ReaderInfo(p, bLeftFirst, f));
                }
            }

            for (int k = l; k <= r; ++k) // all remaining bound at both sides, from the left
            {
                int f (bLeftFirst ? k : nCnt - 1 - k);
                Reader* p (m_vpReaders[f]);
                if (BOUND == p->getBound(LEFT) && BOUND == p->getBound(RIGHT) && 0 == dynamic_cast<StaticReader*>(p))
                {
                    v.push_back(ReaderInfo(p, bLeftFirst, f));
                }
            }

            // now what is left is sourrounded by bound readers at their unbounded side(s) (or are at a margin)
            for (int k = l; k <= r; ++k) // all remaining, from the left
            {
                int f (bLeftFirst ? k : nCnt - 1 - k);
                Reader* p (m_vpReaders[f]);
                if (UNBOUND == p->getBound(LEFT) || UNBOUND == p->getBound(RIGHT))
                {
                    v.push_back(ReaderInfo(p, bLeftFirst, f));
                }
            }
        }

        CB_ASSERT (cSize(v) == cSize(m_vpReaders));

        for (int i = 0; i < nCnt; ++i)
        {
            Reader* pCrt (v[i].m_pReader);
            bool bCrtDir (v[i].m_bFromLeft);

            const char* pcCrtLeft;
            const char* pcCrtRight;
            bool bCrtFixedLeft, bCrtFixedRight;

            findLimits(v[i].m_nOrigPos, pcCrtLeft, pcCrtRight, bFixedLeft, bFixedRight, bCrtFixedLeft, bCrtFixedRight);
            pCrt->init(pcCrtLeft, pcCrtRight, bCrtDir, bCrtFixedLeft, bCrtFixedRight);
            CB_ASSERT (pCrt->m_eState != UNASSIGNED);

            bool bFail (pCrt->m_eState != ASSIGNED);

            if (!bFail)
            { // make sure no unusable space is left to the left
                int k (v[i].m_nOrigPos - 1);

                for (; k >= 0 && m_vpReaders[k]->m_eState == FAILED; --k) {}
                if (k >= 0)
                { // found something
                    if (ASSIGNED == m_vpReaders[k]->m_eState && m_vpReaders[k]->m_pcRight != pCrt->m_pcLeft)
                    {
                        bFail = true;
                    }
                }
            }

            if (!bFail)
            { // make sure no unusable space is left to the right
                int k (v[i].m_nOrigPos + 1);
                for (; k < nCnt && m_vpReaders[k]->m_eState == FAILED; ++k) {}
                if (k < nCnt)
                { // found something
                    if (ASSIGNED == m_vpReaders[k]->m_eState && m_vpReaders[k]->m_pcLeft != pCrt->m_pcRight)
                    {
                        bFail = true;
                    }
                }
            }

            if (bFail)
            {
                if (OPT != pCrt->m_eOptional)
                { // failure on non-optional
                    return false;
                }

                pCrt->m_eState = FAILED;
            }
        }

        m_pcLeft = 0; m_pcRight = 0;

        { // assign m_pcLeft and m_pcRight + assert there are no holes;
            const char* pLastRight (pcLeft);
            Reader* pLastRd (0);
            for (int i = 0; i < nCnt; ++i)
            {
                Reader* p (m_vpReaders[i]);
                CB_ASSERT (UNASSIGNED != p->m_eState);
                if (ASSIGNED == p->m_eState)
                {
                    //CB_ASSERT (pLastRight == p->m_pcLeft || (!m_bTopLevel && 0 == pLastRd));
                    if (pLastRight != p->m_pcLeft && 0 != pLastRd)
                    { // there is a gap between the previous and current reader
                        return false;
                    }
                    //CB_ASSERT (0 == pLastRd || BOUND == pLastRd->m_eBound || BOUND == p->m_eBound); // 2 unbound can't be neighbors, because then there's no way to split something between them; tttx 2008.12.17 - this doesn't allow handling of "%t[ - %a]" but allowing it here is not enough: actually "[ - %a]" is "bound to the left", and should have priority vs "%t"; 2009.01.13 - seems no longer relevant
                    pLastRd = p;
                    pLastRight = p->m_pcRight;

                    if (0 == m_pcLeft) { m_pcLeft = p->m_pcLeft; }
                    m_pcRight = p->m_pcRight;
                }
            }

            if (0 == pLastRd && 0 == m_pcLeft && 0 == m_pcRight)
            { // sequence of optionals, all failed;
                return false; // !!! otherwise there would be issues matching it's left/right to the neighbours; also, the only place a seq with all failed is legal is inside an Optional
            }

            CB_ASSERT (0 != pLastRd); // must be true, because !m_vpReaders.empty()
            /*if (pLastRight != pcRight && m_bTopLevel)
            { // there is a match for the sequence, but some charachters remain to the left; that's invalid in the top level reader
                return false;
            }*/
            //CB_ASSERT (pLastRight == pcRight || !m_bTopLevel);
            CB_ASSERT (0 != m_pcLeft);
            CB_ASSERT (0 != m_pcRight);
        }

        if ((bFixedLeft && m_pcLeft != pcLeft) || (bFixedRight && m_pcRight != pcRight))
        {
            return failure();
        }

        return success();
    }

    /*override*/ void set(vector<string>& v)
    {
        if (ASSIGNED != m_eState) { return; }
        for (int i = 0, n = cSize(m_vpReaders); i < n; ++i)
        {
            Reader* p (m_vpReaders[i]);
            if (ASSIGNED == p->m_eState)
            {
                p->set(v);
            }
        }
    }

private:
    void findLimits(int nPos, const char*& pcLeft, const char*& pcRight, bool bSeqFixedLeft, bool bSeqFixedRight, bool& bFixedLeft, bool& bFixedRight); // finds limits for m_vpReader[nPos], based on the status of the other elements

    //bool m_bTopLevel;
};


void SequenceReader::findLimits(int nPos, const char*& pcLeft, const char*& pcRight, bool bSeqFixedLeft, bool bSeqFixedRight, bool& bFixedLeft, bool& bFixedRight) // finds limits for m_vpReader[nPos], based on the status of the other elements
{
    pcLeft = m_pcLeft;
    for (int k = nPos - 1; k >= 0; --k)
    {
        Reader* p (m_vpReaders[k]);
        if (ASSIGNED == p->m_eState)
        {
            pcLeft = p->m_pcRight;
            break;
        }
    }

    int n (cSize(m_vpReaders));
    pcRight = m_pcRight;
    for (int k = nPos + 1; k < n; ++k)
    {
        Reader* p (m_vpReaders[k]);
        if (ASSIGNED == p->m_eState)
        {
            pcRight = p->m_pcLeft;
            break;
        }
    }

    int nLeft (nPos - 1); // the reader to the left of that on nPos, skipping those that failed; -1 if there's no such thing
    for (; nLeft >= 0 && FAILED == m_vpReaders[nLeft]->m_eState; --nLeft) {}
    bFixedLeft = (-1 == nLeft && bSeqFixedLeft) || (-1 != nLeft && ASSIGNED == m_vpReaders.at(nLeft)->m_eState);

    //bFixedRight = (n - 1 == nPos && bSeqFixedRight) || (n - 1 != nPos && ASSIGNED == m_vpReaders[nPos + 1]->m_eState);
    int nRight (nPos + 1); // the reader to the right of that on nPos, skipping those that failed; n if there's no such thing
    for (; nRight <= n - 1 && FAILED == m_vpReaders[nRight]->m_eState; ++nRight) {}
    bFixedRight = (n == nRight && bSeqFixedRight) || (n != nRight && ASSIGNED == m_vpReaders.at(nRight)->m_eState);
}



struct OptionalReader : public Reader
{
    SequenceReader* m_pSequenceReader;
    OptionalReader(SequenceReader* pSequenceReader) : Reader(OPT, pSequenceReader->getLeftBound(), pSequenceReader->getRightBound()), m_pSequenceReader(pSequenceReader) {}

    /*override*/ bool init(const char* pcLeft, const char* pcRight, bool bLeftFirst, bool bFixedLeft, bool bFixedRight)
    {
        m_pSequenceReader->init(pcLeft, pcRight, bLeftFirst, bFixedLeft, bFixedRight);
        m_eState = m_pSequenceReader->m_eState;
        m_pcLeft = m_pSequenceReader->m_pcLeft;
        m_pcRight = m_pSequenceReader->m_pcRight;

        return ASSIGNED == m_eState;
    }

    /*override*/ void clearState()
    {
        m_eState = UNASSIGNED;
        m_pSequenceReader->clearState();
    }

    /*override*/ void set(vector<string>& v) { if (ASSIGNED == m_eState) { m_pSequenceReader->set(v); } }
};



TrackTextParser::TrackTextParser(const string& strPattern) : m_strPattern(strPattern)
{
    if (strPattern.empty()) { throw InvalidPattern(0); }
    string::size_type n (0);
    char cSep (getPathSep());
    enum FoundSep { NO, ESC, FREE };
    FoundSep e (NO);
    for (;;)
    { // don't allow mixing of free and escaped patterns
        n = strPattern.find(cSep, n);
        if (n == string::npos) { break; }
        if (0 == n || '%' != strPattern[n - 1])
        {
            if (ESC == e)
            {
                throw InvalidPattern(n);
            }
            else
            {
                e = FREE;
            }
        }
        else
        {
            if (FREE == e)
            {
                throw InvalidPattern(n);
            }
            else
            {
                e = ESC;
            }
        }
        ++n;
    }
    m_bSplitAtPathSep = (FREE == e);
    construct(strPattern); // so we can debug
}







void TrackTextParser::construct(const std::string& strPattern)
{
    const char* pBegin (strPattern.c_str());
    char cSep (getPathSep());
    if (cSep == *pBegin)
    {
        CB_ASSERT (m_bSplitAtPathSep);
        ++pBegin;
    }

    const char* p (pBegin);
    int nErrPos (-1);

    StaticReader* pCrtStatic (0);
    stack<SequenceReader*> stack;

    stack.push(new SequenceReader());
    m_vpAllReaders.push_back(stack.top());
    m_vpSeqReaders.push_back(stack.top());

    for (;; ++p)
    {
        char c (*p);
        if (0 == c) { break; }

        bool bRegularChar (c != '[' && c != ']' && c != '%' && c != cSep);
        if ('%' == c)
        {
            char c1 (*(p + 1));
            if ('%' == c1 || '[' == c1 || ']' == c1 || cSep == c1)
            {
                c = c1;
                ++p;
                bRegularChar = true;
            }
        }

        if (bRegularChar)
        {
            if (0 == pCrtStatic)
            {
                pCrtStatic = new StaticReader();
                m_vpAllReaders.push_back(pCrtStatic);
                stack.top()->m_vpReaders.push_back(pCrtStatic);
            }
            pCrtStatic->m_str += c;
        }
        else
        {
            pCrtStatic = 0;

            if (cSep == c)
            {
                CB_ASSERT (m_bSplitAtPathSep);
                if (1 != cSize(stack)) { nErrPos = p - pBegin; goto err; }

                if (!stack.top()->setBounds()) { nErrPos = p - pBegin; goto err; }
                stack.pop();
                stack.push(new SequenceReader());
                m_vpAllReaders.push_back(stack.top());
                m_vpSeqReaders.push_back(stack.top());
            }
            else
            {
                switch (c)
                {
                case '[':
                    {
                        stack.push(new SequenceReader());
                        m_vpAllReaders.push_back(stack.top());
                        break; // !!! there's no point in creating an OptionalReader now; it will be created at the end of the optional sequence
                    }
                case ']':
                    {
                        if (1 == cSize(stack)) { nErrPos = p - pBegin; goto err; }

                        if (!stack.top()->setBounds()) { nErrPos = p - pBegin; goto err; }

                        OptionalReader* o (new OptionalReader(stack.top()));
                        m_vpAllReaders.push_back(o);

                        stack.pop();
                        stack.top()->m_vpReaders.push_back(o);
                        break;
                    }
                case '%':
                    {
                        c = *++p;
                        switch (c)
                        {
                        case 'n': { stack.top()->m_vpReaders.push_back(new TrackNoReader()); break; }
                        case 'a': { stack.top()->m_vpReaders.push_back(new UnboundReader(TagReader::ARTIST)); break; }
                        case 't': { stack.top()->m_vpReaders.push_back(new UnboundReader(TagReader::TITLE)); break; }
                        case 'b': { stack.top()->m_vpReaders.push_back(new UnboundReader(TagReader::ALBUM)); break; }
                        case 'y': { stack.top()->m_vpReaders.push_back(new YearReader()); break; }
                        case 'g': { stack.top()->m_vpReaders.push_back(new UnboundReader(TagReader::GENRE)); break; }
                        case 'r': { stack.top()->m_vpReaders.push_back(new RatingReader()); break; }
                        case 'c': { stack.top()->m_vpReaders.push_back(new UnboundReader(TagReader::COMPOSER)); break; }
                        //ttt2 perhaps add something for "various artists"
                        case 'i': { stack.top()->m_vpReaders.push_back(new UnboundReader(TagReader::LIST_END)); break; } // !!! LIST_END for ignored items
                        default: { nErrPos = p - pBegin; goto err; }
                        }
                        break;
                    }
                default:
                    CB_ASSERT(false);
                }
            }
        }
    }

    if (1 == cSize(stack) && stack.top()->setBounds())
    {
        return;
    }

    nErrPos = p - pBegin;

err:
    clearPtrContainer(m_vpAllReaders);
    throw InvalidPattern(nErrPos);
}

TrackTextParser::~TrackTextParser()
{
    clearPtrContainer(m_vpAllReaders);
}


void TrackTextParser::assign(const string& s, vector<string>& v)
{
    CB_ASSERT (TagReader::LIST_END + 1 == cSize(v)); // !!! "LIST_END + 1" instead of just "LIST_END", so the last entry can be used for "%i" (i.e. "ignored")
    for (int i = cSize(m_vpSeqReaders) - 1; i >= 0; --i)
    {
        m_vpSeqReaders[i]->clearState();
    }

    string::size_type nPrev (s.size()); // last position to search (inclusive)
    if (m_bSplitAtPathSep)
    {
        if (nPrev <= 4) { return; }
        nPrev -= 5; // discard ".mp3" extension
    }
    else
    {
        --nPrev; // needed because a "1" will be added below
    }
    const char* szName (s.c_str());
    int n (cSize(m_vpSeqReaders));
    CB_ASSERT (1 == n || m_bSplitAtPathSep);
    for (int i = n - 1; i >= 0; --i)
    {
        string::size_type nCrt (m_bSplitAtPathSep ? s.rfind(getPathSep(), nPrev) : 0);
        if (string::npos == nCrt) { break; }
        if (!m_bSplitAtPathSep)
        {
            --nCrt; // now it's -1 (or rather npos), to get it ready for the addition of 1
        }

        SequenceReader& rd (*m_vpSeqReaders[i]);
        rd.init(szName + nCrt + 1, szName + nPrev + 1, Reader::LEFT_FIRST, Reader::FIXED_LEFT, Reader::FIXED_RIGHT);
        if (SequenceReader::ASSIGNED == rd.m_eState)
        {
            CB_ASSERT(rd.m_pcRight - rd.m_pcLeft == (int)nPrev - (int)nCrt);
            rd.set(v);

            /*if (rd.m_pcRight - rd.m_pcLeft == (int)nPrev - (int)nCrt) // have to test this for top-level SeqReaders, which must cover the whole in this case
            {
                rd.set(v);
            }*/
        }

        nPrev = nCrt;
        if (0 == nPrev) { break; }
        --nPrev;
    }

    for (int i = 0; i < TagReader::LIST_END; ++i)
    {
        trim(v[i]);
    }
}


// returns an empty string if the pattern is valid and an error message (including the column) if it's invalid
string testPattern(const string& strPattern)
{
    try
    {
        TrackTextParser rd (strPattern);
        return "";
    }
    catch (const TrackTextParser::InvalidPattern& ex)
    {
        ostringstream s;
        s << "\"" << toNativeSeparators(strPattern) << "\" is not a valid pattern. Error in column " << ex.m_nPos <<  "."; //ttt2 perhaps more details
        return s.str();
    }
}


} // namespace SongInfoParser




#ifdef LPPPLPJOJAOIIIOJ

#include <iostream>
struct TestTrackTextParser
{
    string f (string s)
    {
        for (;;)
        {
            string::size_type n (s.find(' '));
            if (string::npos == n)
            {
                return " " + s + "\n";
            }
            s[n] = '*';
        }
    }

    TestTrackTextParser()
    {
/*
        {
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/%n[%r]");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/b/q/12ab.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }

        {
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/[%r]%n");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/b/q/ab12.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }

        {
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/%a/%b/[%r]%n [-[ ]]%t");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/b/q/wa04 Snow Come Down.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }
        */

        /*{
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/%a/%t/[%r]%n");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/e/q/ab12.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }

        {
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/%a/%t/[%r]%n");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/e/q/b12.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }*/


        /*{
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/[.%a.]%b");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/e/q/.aaaa.bbbb.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }*/

        /*{
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/[.]%b");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/e/q/aaaa.bbbb.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }

        {
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/[.]%b");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/e/q/aaaa,bbbb.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }

        try
        {
            //SongInfoParser::TrackTextParser fr ("/%a%b");
            //SongInfoParser::TrackTextParser fr ("/%a[%b]");
            //%a/%b/[%r]%n [-[ ]]%t
            SongInfoParser::TrackTextParser fr ("/[-[ ]]%t");
        }
        catch (...)
        {
            cout << "err\n";
        }*/



        /*{
            //ttt2 make these work:
            SongInfoParser::TrackTextParser fr ("/[ [ ]][-[ [ ]]]%t");
            //SongInfoParser::TrackTextParser fr ("/%t");
            const char* s ("-------------------------------\n");
            vector<string> v; 
            vector<string> (TagReader::LIST_END + 1).swap(v);
            fr.init(".../ tst01.mp3", v);
            cout << "2 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }//*/


        /*{
            //ttt2 make these work:
            SongInfoParser::TrackTextParser fr ("/[[%r]%n][[ ] ][-[[ ] ]]%t");
            //SongInfoParser::TrackTextParser fr ("/%t");
            const char* s ("-------------------------------\n");
            vector<string> v; 
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.init(".../b06 -tst01.mp3", v); cout << "2 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }*/


        /*{
            SongInfoParser::TrackTextParser fr ("/[[%r]%n][ [ ]][-[ [ ]]]%t");
            const char* s ("-------------------------------\n");
            vector<string> v; 
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06tst01.mp3", v); cout << "1 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06 tst01.mp3", v); cout << "2 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06  tst01.mp3", v); cout << "3 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06-tst01.mp3", v); cout << "4 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06- tst01.mp3", v); cout << "5 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06 -tst01.mp3", v); cout << "6 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06 - tst01.mp3", v); cout << "7 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06  - tst01.mp3", v); cout << "8 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06 -  tst01.mp3", v); cout << "9 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign(".../b06  -  tst01.mp3", v); cout << "10 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
        }//*/


        /*{
            SongInfoParser::TrackTextParser fr ("[[%r]%n][ [ ]][-[ [ ]]]%t");
            const char* s ("-------------------------------\n");
            vector<string> v; 
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06tst01.mp3", v); cout << "1 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06 tst01.mp3", v); cout << "2 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06  tst01.mp3", v); cout << "3 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06-tst01.mp3", v); cout << "4 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06- tst01.mp3", v); cout << "5 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06 -tst01.mp3", v); cout << "6 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06 - tst01.mp3", v); cout << "7 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06  - tst01.mp3", v); cout << "8 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06 -  tst01.mp3", v); cout << "9 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("b06  -  tst01.mp3", v); cout << "10 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
        }//*/


        {
            SongInfoParser::TrackTextParser fr ("%t-%an");
            const char* s ("-------------------------------\n");
            vector<string> v; 
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("HHHHHHHHHHHHHH - QQQQQQQ  Listen", v); cout << "1 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("H-Listen", v); cout << "2 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
        }//*/

        {
            SongInfoParser::TrackTextParser fr ("%t-%aen");
            const char* s ("-------------------------------\n");
            vector<string> v; 
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("HHHHHHHHHHHHHH - QQQQQQQ  Listen", v); cout << "3 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
            vector<string> (TagReader::LIST_END + 1).swap(v); fr.assign("H-Len", v); cout << "4 " << s; for (int i = 0; i < cSize(v); ++i) { cout << i << f(v[i]); }
        }//*/


/*
patterns\value0000=%a/%b/[%r]%n [-[ ]]%t
patterns\value0001=%t
patterns\value0002=%i %t[go]
patterns\value0003=[%a][%b]
patterns\value0004=%a-%b
patterns\value0005=%a[-%b]
patterns\value0006=[%a-]%b
patterns\value0007=[%a]%b

        {
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/%a/%b/[%r]%n [-[ ]]%t");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/b/q/wa04 Snow Come Down.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }

        {
            cout << "-------------------------------\n";
            SongInfoParser::TrackTextParser fr ("/%a/%b/[%r]%n [-[ ]]%t");
            vector<string> v (TagReader::LIST_END + 1);
            fr.init("/r/temp/1/tmp2/c pic/b/q/a04 Snow Come Down.mp3", v);
            for (int i = 0; i < cSize(v); ++i) { cout << i << " " << v[i] << endl; }
        }*/
    }
};

/*


---------------------

cp "tst01.mp3" "d1/b06tst01.mp3"
cp "tst01.mp3" "d1/b06 tst01.mp3"
cp "tst01.mp3" "d1/b06  tst01.mp3"
cp "tst01.mp3" "d1/b06-tst01.mp3"
cp "tst01.mp3" "d1/b06- tst01.mp3"
cp "tst01.mp3" "d1/b06 -tst01.mp3"
cp "tst01.mp3" "d1/b06 - tst01.mp3"
cp "tst01.mp3" "d1/b06  - tst01.mp3"
cp "tst01.mp3" "d1/b06 -  tst01.mp3"
cp "tst01.mp3" "d1/b06  -  tst01.mp3"

cp "tst01.mp3" "d1/06tst01.mp3"
cp "tst01.mp3" "d1/06 tst01.mp3"
cp "tst01.mp3" "d1/06  tst01.mp3"
cp "tst01.mp3" "d1/06-tst01.mp3"
cp "tst01.mp3" "d1/06- tst01.mp3"
cp "tst01.mp3" "d1/06 -tst01.mp3"
cp "tst01.mp3" "d1/06 - tst01.mp3"
cp "tst01.mp3" "d1/06  - tst01.mp3"
cp "tst01.mp3" "d1/06 -  tst01.mp3"
cp "tst01.mp3" "d1/06  -  tst01.mp3"

cp "tst01.mp3" "d1/btst01.mp3"
cp "tst01.mp3" "d1/b tst01.mp3"
cp "tst01.mp3" "d1/b  tst01.mp3"
cp "tst01.mp3" "d1/b-tst01.mp3"
cp "tst01.mp3" "d1/b- tst01.mp3"
cp "tst01.mp3" "d1/b -tst01.mp3"
cp "tst01.mp3" "d1/b - tst01.mp3"
cp "tst01.mp3" "d1/b  - tst01.mp3"
cp "tst01.mp3" "d1/b -  tst01.mp3"
cp "tst01.mp3" "d1/b  -  tst01.mp3"


*/


TestTrackTextParser qqwtrh;
#endif
