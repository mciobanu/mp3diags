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

#include  "Helpers.h"
#include  "Version.h"


using namespace Version;

/*
" + QString(::AboutDlgImpl::tr("QQQ")).arg("QQQ").arg("QQQ").arg("QQQ") + "
" + QString(::AboutDlgImpl::tr("QQQ")).arg("QQQ").arg("QQQ") + "
" + QString(::AboutDlgImpl::tr("QQQ")).arg("QQQ") + "

" + ::AboutDlgImpl::tr("QQQ").arg("QQQ").arg("QQQ").arg("QQQ") + "
" + ::AboutDlgImpl::tr("QQQ").arg("QQQ").arg("QQQ") + "
" + ::AboutDlgImpl::tr("QQQ").arg("QQQ") + "

*/

AboutDlgImpl::AboutDlgImpl(QWidget* pParent /* = 0*/) : QDialog(pParent, getDialogWndFlags()), Ui::AboutDlg()
{
    setupUi(this);

    QPalette pal (m_pMainTextM->palette());
    pal.setColor(QPalette::Base, pal.color(QPalette::Disabled, QPalette::Window));

    m_pMainTextM->setPalette(pal);
/*
<a href=\"DDDDDDDDDDDDD\">NNNNNNNNNNNNNN</a>
*/
    m_pMainTextM->setHtml(
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Written by %1, %2").arg("<a href=\"mailto:mp3diags@gmail.com?subject=000 MP3 Diags\">Marian Ciobanu (Ciobi)</a>").arg("2008 - 2012") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Command-line mode by %1, %2").arg("Michael Elsd&#xf6;rfer").arg("2011") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("%1 translation by %2, %3").arg(AboutDlgImpl::tr("Czech")).arg("<a href=\"http://fripohled.blogspot.com/\">Pavel Fric</a>").arg("2012") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("%1 translation by %2, %3").arg(AboutDlgImpl::tr("German")).arg("<a href=\"mailto:2711271+translate@gmail.com?subject=MP3 Diags translation\">Marco Krause</a>").arg("2012") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Distributed under %1").arg("<a href=\"http://www.gnu.org/licenses/gpl-2.0.html#TOC1\">GPL V2</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Using %1, released under %2").arg("<a href=\"http://qt.nokia.com/products/qt-sdk\">Qt SDK</a>").arg("<a href=\"http://www.gnu.org/licenses/lgpl-2.1.html\">LGPL 2.1</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Using %1, released under the %2zlib License%3").arg("<a href=\"http://www.zlib.net/\">zlib</a>").arg("<a href=\"http://www.zlib.net/zlib_license.html\">").arg("</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Using %1, distributed under the %2Boost Software License%3").arg("<a href=\"http://www.boost.org/doc/libs/1_39_0/libs/serialization/doc/index.html\">Boost Serialization</a>").arg("<a href=\"http://www.boost.org/users/license.html\">").arg("</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Using original and modified icons from the %1 for %2, distributed under %3LGPL V3%4").arg("<a href=\"http://www.oxygen-icons.org/\">Oxygen Project</a>").arg("<a href=\"http://www.kde.org/\">KDE 4</a>").arg("<a href=\"http://www.gnu.org/licenses/lgpl.html\">").arg("</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Using web services provided by %1 to retrieve album data").arg("<a href=\"http://www.discogs.com/\">Discogs</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Using web services provided by %1 to retrieve album data").arg("<a href=\"http://musicbrainz.org/\">MusicBrainz</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Home page and documentation: %1").arg("<a href=\"http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/\">http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Feedback and support: %1 or %2 at SourceForge").arg("<a href=\"http://sourceforge.net/forum/forum.php?forum_id=947206\">Open Discussion Forum</a>").arg("<a href=\"http://sourceforge.net/forum/forum.php?forum_id=947207\">Help Forum</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Bug reports and feature requests: %1 at SourceForge").arg("<a href=\"http://sourceforge.net/apps/mantisbt/mp3diags/\">MantisBT Issue Tracker</a>") + "</p>"
        "<p style=\"margin-bottom:8px; margin-top:1px; \">" + AboutDlgImpl::tr("Change log for the latest version: %1").arg("<a href=\"http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/015_changelog.html\">http://mp3diags.sourceforge.net" + QString(getWebBranch()) + "/015_changelog.html</a>") + "</p>"
        );

    m_pVersionL->setText(QString(getAppName()) + " " + getSimpleAppVer()); //ttt1 write "unstable" in red

    initText(m_pGplV2M, ":/licences/gplv2.txt");
    initText(m_pGplV3M, ":/licences/gplv3.txt");
    initText(m_pLgplV3M, ":/licences/lgplv3.txt");
    initText(m_pLgplV21M, ":/licences/lgpl-2.1.txt");
    initText(m_pBoostM, ":/licences/boost.txt");
    initText(m_pZlibM, ":/licences/zlib.txt");

    m_pSysInfoM->setText(getSystemInfo());

    m_pMainTextM->setFocus();
    //{ QAction* p (new QAction(this)); p->setShortcut(QKeySequence("Ctrl+N")); connect(p, SIGNAL(triggered()), this, SLOT(accept())); addAction(p); }

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
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



void AboutDlgImpl::onHelp()
{
    openHelp("index.html");
}




// exist: mp3 insight, mp3 doctor, mp3 butcher, mp3 toolbox, mp3 mechanic, mp3 workshop;
//ttt melt ? ice ? ? sorcerer ? exorcist ? healer ? ? MP3 Spy
//"mp3 workshop", "mp3 atelier"
//workshop synonyms:  foundry, laboratory, mill, plant, studio, works
// deep understanding




/*


//bjam --toolset=gcc
//PATH=D:\Qt\2009.02\mingw\bin;%PATH%
bjam serialization toolset=gcc


bjam toolset=gcc serialization threading=multi release

http://stackoverflow.com/questions/718447/adding-external-library-into-qt-creator-project
http://stackoverflow.com/questions/199092/compiling-a-qt-program-in-windows-xp-with-mingws-g
*/

/*
Finds problems in MP3 files and helps the user to fix many of them using included tools. Looks at both the audio part (VBR info, quality, normalization) and the tags containing track information (ID3.) Also includes a tag editor and a file renamer.
*/


//PATH=D:\Qt\2009.02\qt\bin;%PATH%







//ttt2 perhaps "Scan images in the current folder", checked by default
//ttt2 perhaps something to remove image files after assigning them, or at least show them in a different color; it was suggested to add a "-" button to remove images, below the "v" for "assigning them", but not sure it's such great idea; perhaps some option to delete local images that were assigned (but perhaps the unassigned CD scan should go as well); // perhaps "-" works, though; should be enabled/visible only for local files









//ttt2 some standard means to log only uncaught exceptions




// backport jaunty 9.04 : https://bugs.launchpad.net/jaunty-backports/+bug/423560






/*
//ttt2 w7 8.3 names

C:\Windows\system32>fsutil behavior query disable8dot3 c:
The volume state for Disable8dot3 is 0 (8dot3 name creation is enabled).
The registry state of NtfsDisable8dot3NameCreation is 2, the default (Volume level setting).
Based on the above two settings, 8dot3 name creation is enabled on c:.
*/


//ttt2 Perhaps linking Boost statically and Qt dynamically would solve most dependency issues - http://pages.cs.wisc.edu/~thomas/X/static-linking.html : "surround the libraries you wish to link statically with -static and -dynamic (in that order)"; OTOH "-dynamic" is not in man, and that page is from 1997; things have changed, and -static seems to be a global option, so it doesn't matter if you put it first or last; what should work is specifying the file name: -l:libboost_serialization.a rather than -lboost_serialization (!!! note the ":")



//ttt2 mutt rips: https://sourceforge.net/projects/mp3diags/forums/forum/947206/topic/3441516 ; also, check for missing tracks and other album-related issues;


//ttt0 might have to remove the program before switching packages; - in the fake mp3diags

//ttt1 01 - The Privateer.mp3 - go to tag editor, change track, save; new image doesn't show, but there is the original "other" bmp and the new "cover" jpg

//ttt2 maybe support for saving images to .directory files - point 7 at https://sourceforge.net/projects/mp3diags/forums/forum/947206/topic/3389395
