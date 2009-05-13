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


#include  "LyricsStream.h"

#include  "Helpers.h"


using namespace std;


LyricsStream::LyricsStream(int nIndex, NoteColl& notes, std::istream& in) : DataStream(nIndex), m_pos(in.tellg())
{
    StreamStateRestorer rst (in);

    const int BFR_SIZE (256);
    char bfr [BFR_SIZE];

    streamoff nRead (read(in, bfr, BFR_SIZE));

    MP3_CHECK_T (nRead >= 11 + 9 && 0 == strncmp("LYRICSBEGIN", bfr, 11), m_pos, "Invalid Lyrics stream tag. Header not found.", NotLyricsStream());

    streampos pos (m_pos);
    for (;;)
    {
        in.seekg(pos);
        streamoff nRead (read(in, bfr, BFR_SIZE));
        MP3_CHECK (nRead >= 9, pos, lyrTooShort, NotLyricsStream());

        for (int i = 0; i <= BFR_SIZE - 9; ++i)
        {
            if (0 == strncmp("LYRICS200", bfr + i, 9))
            {
                pos += i + 9;
                goto e1;
            }
        }
        pos += BFR_SIZE - 9;
    }
e1:
    in.seekg(pos);
    m_nSize = pos - m_pos;

    MP3_TRACE (m_pos, "LyricsStream built.");

    rst.setOk();
}


/*override*/ void LyricsStream::copy(std::istream& in, std::ostream& out)
{
    appendFilePart(in, out, m_pos, m_nSize);
}


/*override*/ std::string LyricsStream::getInfo() const
{
    return "";
}



