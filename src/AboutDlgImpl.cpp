/***************************************************************************
 *   MP3 Insight - diagnosis, repairs and tag editing for MP3 files        *
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
        "Written by <a href=\"mailto:ciobi@inbox.com?subject=000 MP3 Diags\">Marian Ciobanu (Ciobi)</a>, 2008<p/>"
        "Using <a href=\"http://doc.trolltech.com/4/opensourceedition.html\">Qt 4 Open Source Edition</a><p/>"
        "Distributed under <a href=\"http://www.gnu.org/licenses/gpl-2.0.html#TOC1\">GPL V2</a><p/>"
        "Most icons are either copies or modified versions of icons from the <a href=\"http://www.oxygen-icons.org/\">Oxygen Project</a> for <a href=\"http://www.kde.org/\">KDE 4</a>. They are distributed under <a href=\"http://www.gnu.org/licenses/lgpl.html\">LGPL V3</a><p/>"
        "Home page: <a href=\"http://www.sourceforge.com/\">http://www.sourceforge.com</a><p/>"
        "Documentation: <a href=\"http://www.sourceforge.com/\">http://www.sourceforge.com</a>"); //ttt0

    m_pVersionL->setText("MP3 Diags 0.7.0.1");

    initText(m_pGplV2M, ":/licences/gplv2.txt");
    initText(m_pGplV3M, ":/licences/gplv3.txt");
    initText(m_pLgplV3M, ":/licences/lgplv3.txt");

    m_pMainTextM->setFocus();
    //{ QAction* p (new QAction(this)); p->setShortcut(QKeySequence("Ctrl+N")); connect(p, SIGNAL(triggered()), this, SLOT(accept())); addAction(p); }
}


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



//ttt0 doc: other reason to use file reader: up/low case
//ttt0 doc: you may want to use file reader to set genre, year, composer, which are more likely to be missing or wrong at MuscBrainz
//ttt0 document that m_eCaseForArtists is for composers too

//ttt0 about: zlib
//ttt0 about: discogs


// exist: mp3 insight, mp3 doctor, mp3 butcher, mp3 toolbox, mp3 mechanic, mp3 workshop;
//ttt0 melt ? ice ? ? sorcerer ? exorcist ? healer ? ? MP3 Spy
//"mp3 workshop", "mp3 atelier"
//workshop synonyms:  foundry, laboratory, mill, plant, studio, works
// deep understanding

/*
ttt0 documentation

//ttt0 usage suggestions: 1) don't create transf, keep orig, generate comp => look at comp 2) if ok, create transf in orig dir and move or erase proc orig //ttt0 this suggests a new option for rename: "move if file doesn't yet exist / remove if it exists"; this way only the real orig file is kept and various intermediaries are removed (remember that while one "transform batch" produces a single "proc", multiple edits can be performed, each with its own "orig" file

ttt0 handles mpeg2, mpeg1 layer2, ... but files must end with mp3

all ID3V2 that are created are 2.3.0

*/

//ttt0 doc: save in tag edt doesn't save everything, only as configured (regardless of automatic)

// ttt0 doc: tag edt removes POPM, doesn't use email
//ttt0 doc, after file renaming: can be used to have some meaningful rating (set rating => rename => ... )



//ttt0 doc bug described at the end of TagEditorDlgImpl::onFileSelSectionMoved

//ttt0 doc: lack of consistency
//ttt0 doc: duplicate images when they axist with non-"cover" type;
//ttt0 doc: a single "root"; ttt1 perhaps allow more dirs
//ttt0 doc UTF8
//ttt0 doc: boost
//ttt0 search for http and www, and check links in the code

//ttt0 self-ref using symlinks


