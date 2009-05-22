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


#include  <QFile>

#include  "AboutDlgImpl.h"

extern const char* APP_VER;

AboutDlgImpl::AboutDlgImpl(QWidget* pParent /*= 0*/) : QDialog(pParent), Ui::AboutDlg()
{
    setupUi(this);

    QPalette pal (m_pMainTextM->palette());
    pal.setColor(QPalette::Base, pal.color(QPalette::Disabled, QPalette::Window));

    m_pMainTextM->setPalette(pal);
/*
<a href=\"DDDDDDDDDDDDD\">NNNNNNNNNNNNNN</a>
*/
    m_pMainTextM->setHtml(
        "Written by <a href=\"mailto:ciobi@inbox.com?subject=000 MP3 Diags\">Marian Ciobanu (Ciobi)</a>, 2008 - 2009<p/>"
        "Distributed under <a href=\"http://www.gnu.org/licenses/gpl-2.0.html#TOC1\">GPL V2</a><p/>"
        //"Using <a href=\"http://doc.trolltech.com/4/opensourceedition.html\">Qt 4 Open Source Edition</a><p/>"
        "Using <a href=\"http://www.qtsoftware.com/\">Qt Free Edition</a>, released under <a href=\"http://www.gnu.org/licenses/lgpl-2.1.html\">LGPL 2.1</a><p/>"
        "Using <a href=\"http://www.zlib.net/\">zlib</a>, released under the <a href=\"http://www.zlib.net/zlib_license.html\">zlib License</a><p/>"
        "Using <a href=\"http://www.boost.org/doc/libs/1_39_0/libs/serialization/doc/index.html\">Boost Serialization</a>, distributed under the <a href=\"http://www.boost.org/users/license.html\">Boost Software License</a><p/>"
        //"Most icons are either copies or modified versions of icons from the <a href=\"http://www.oxygen-icons.org/\">Oxygen Project</a> for <a href=\"http://www.kde.org/\">KDE 4</a>. They are distributed under <a href=\"http://www.gnu.org/licenses/lgpl.html\">LGPL V3</a><p/>"
        "Using original and modified icons from the <a href=\"http://www.oxygen-icons.org/\">Oxygen Project</a> for <a href=\"http://www.kde.org/\">KDE 4</a>, distributed under <a href=\"http://www.gnu.org/licenses/lgpl.html\">LGPL V3</a><p/>"
        "Using web services provided by <a href=\"http://www.discogs.com/\">Discogs</a> to retrieve album data<p/>"
        "Using web services provided by <a href=\"http://musicbrainz.org/\">MusicBrainz</a> to retrieve album data<p/><p/>"
        "Home page: <a href=\"http://www.sourceforge.com/\">http://www.sourceforge.com</a><p/>"
        "Documentation: <a href=\"http://www.sourceforge.com/\">http://www.sourceforge.com</a>"); //ttt0

    m_pVersionL->setText(QString("MP3 Diags ") + APP_VER);

    initText(m_pGplV2M, ":/licences/gplv2.txt");
    initText(m_pGplV3M, ":/licences/gplv3.txt");
    initText(m_pLgplV3M, ":/licences/lgplv3.txt");
    initText(m_pLgplV21M, ":/licences/lgpl-2.1.txt");
    initText(m_pBoostM, ":/licences/boost.txt");
    initText(m_pZlibM, ":/licences/zlib.txt");

    m_pMainTextM->setFocus();
    //{ QAction* p (new QAction(this)); p->setShortcut(QKeySequence("Ctrl+N")); connect(p, SIGNAL(triggered()), this, SLOT(accept())); addAction(p); }
}

//ttt0 https://apps.sourceforge.net/trac/sourceforge/ticket/631

void AboutDlgImpl::initText(QTextBrowser* p, const char* szFileName)
{
    QFile f (szFileName);
    //QFile::FileError err (f.error());
    //qDebug("file: %d", (int)err);
    //qDebug("size : %d", (int)f.size());
    //QByteArray b (f.readAll());
    f.open(QIODevice::ReadOnly);
    //qDebug("read size : %d", (int)b.size());
    p->setText(QString::fromUtf8(f.readAll()));
}


AboutDlgImpl::~AboutDlgImpl()
{
}





// exist: mp3 insight, mp3 doctor, mp3 butcher, mp3 toolbox, mp3 mechanic, mp3 workshop;
//ttt melt ? ice ? ? sorcerer ? exorcist ? healer ? ? MP3 Spy
//"mp3 workshop", "mp3 atelier"
//workshop synonyms:  foundry, laboratory, mill, plant, studio, works
// deep understanding










/*
Finds problems in MP3 files and helps the user to fix many of them using included tools. Looks at both the audio part (VBR info, quality, normalization) and the tags containing track information (ID3.) Also includes a tag editor and a file renamer.
*/





