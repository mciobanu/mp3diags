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


#include  <QFrame>

class QLineEdit;
class QLabel;

struct TagReader;

class TagReadPanel : public QFrame
{
    Q_OBJECT

    QLineEdit* m_pTrackNameE;
    QLineEdit* m_pTrackArtistE;
    QLineEdit* m_pTrackNumberE;
    QLineEdit* m_pTrackTimeE;
    QLineEdit* m_pTrackGenreE;
    QLineEdit* m_pAlbumNameE;

    QLabel* m_pPictureL;
    QLabel* m_pPictureSizeL;

    TagReader* m_pTagReader;
public:

    TagReadPanel(QWidget *pParent, TagReader* pTagReader);
};

