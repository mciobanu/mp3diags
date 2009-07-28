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


#include  <QMessageBox>

#include  "TagEdtPatternsDlgImpl.h"

#include  "Helpers.h"
#include  "SongInfoParser.h"
#include  "StoredSettings.h"


using namespace std;


TagEdtPatternsDlgImpl::TagEdtPatternsDlgImpl(QWidget* pParent, SessionSettings& settings, const vector<string>& vstrPredef) : QDialog(pParent, getDialogWndFlags()), Ui::PatternsDlg(), m_settings(settings), m_vstrPredef(vstrPredef), m_nCrtLine(-1), m_nCrtCol(-1)
{
    setupUi(this);

    QPalette grayPalette (m_infoM->palette());

    grayPalette.setColor(QPalette::Base, grayPalette.color(QPalette::Disabled, QPalette::Window));

    m_infoM->setPalette(grayPalette);
    m_infoM->setTabStopWidth(fontMetrics().width("%ww"));

#ifndef WIN32
    QString qsSep (getPathSep());
#else
    QString qsSep ("\\");
#endif

    m_infoM->setText("%n\ttrack number\n%a\tartist\n%t\ttitle\n%b\talbum\n%y\tyear\n%g\tgenre\n%r\trating (a lowercase letter)\n%c\tcomposer\n%i\tignored"
            "\n\nTo include the special characters \"%\", \"[\", \"]\" and \"" + qsSep + "\", preced them by a \"%\": \"%%\", \"%[\", \"%]\" and \"%" + qsSep + "\""
            "\n\nFor a pattern to be considered a \"file pattern\" (as opposed to a \"table pattern\"), it must contain at least a \"" + qsSep + "\", even if you don't care about what's in the file's parent directory (see the fourth predefined pattern for an example.)"
            "\n\nLeading and trailing spaces are removed automatically, so \"-[ ]%t\" is equivalent to \"-%t\"");

    int nWidth, nHeight;
    m_settings.loadTagEdtPatternsSettings(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 300) { resize(nWidth, nHeight); }

    connect(m_pTextM, SIGNAL(cursorPositionChanged()), this, SLOT(onCrtPosChanged()));

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
}




TagEdtPatternsDlgImpl::~TagEdtPatternsDlgImpl()
{
}

/*$SPECIALIZATION$*/

void TagEdtPatternsDlgImpl::on_m_pCancelB_clicked()
{
    reject();
}


void TagEdtPatternsDlgImpl::on_m_pOkB_clicked()
{
    m_vPatterns.clear();
    string s (convStr(m_pTextM->toPlainText()));
    const char* p (s.c_str());
    if (0 == *p) { accept(); return; }
    for (; '\n' == *p; ++p) {}

    const char* q (p);
    for (;;)
    {
        if ('\n' == *p || 0 == *p) // ttt1 see if this works on Windows with mingw
        {
            string s1 (q, p - q);
            s1 = fromNativeSeparators(s1);
            string strCheck (SongInfoParser::testPattern(s1));
            if (!strCheck.empty())
            {
                QMessageBox::critical(this, "Error", convStr(strCheck));
                return;
            }

            m_vPatterns.push_back(s1);

            for (; '\n' == *p; ++p) {}
            if (0 == *p) { break; }
            q = p;
        }
        ++p;
    }

    m_settings.saveTagEdtPatternsSettings(width(), height());

    accept();
}




bool TagEdtPatternsDlgImpl::run(vector<pair<string, int> >& v)
{
    string s;
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        if (!s.empty()) { s += "\n"; }
        s += toNativeSeparators(v[i].first);
    }
    m_pTextM->setText(convStr(s));
    if (QDialog::Accepted != exec()) { return false; }

    set<int> sPos;

    vector<pair<string, int> > v1;
    for (int i = 0, n = cSize(m_vPatterns); i < n; ++i)
    {
        int j (0);
        int m (cSize(v));
        for (; j < m; ++j)
        {
            if (m_vPatterns[i] == v[j].first && sPos.end() == sPos.find(j))
            {
                sPos.insert(j);
                break;
            }
        }
        if (m == j) { j = -1; }
        v1.push_back(make_pair(m_vPatterns[i], j));
    }

    //v.clear();
    v.swap(v1);

    return true;
}


void TagEdtPatternsDlgImpl::on_m_pAddPredefB_clicked()
{
    string s (convStr(m_pTextM->toPlainText()));
    for (unsigned i = 0; i < m_vstrPredef.size(); ++i)
    {
        if (!s.empty() && !endsWith(s, "\n"))
        {
            s += "\n";
        }

        s += toNativeSeparators(m_vstrPredef[i]);
    }

    m_pTextM->setText(convStr(s));
}


void TagEdtPatternsDlgImpl::onHelp()
{
    openHelp("220_tag_editor_patterns.html");
}


void TagEdtPatternsDlgImpl::onCrtPosChanged()
{
    QTextCursor crs (m_pTextM->textCursor());
    m_nCrtLine = crs.blockNumber();
    m_nCrtCol = crs.columnNumber();
    m_pCrtPosL->setText(QString("Line %1, Col %2").arg(m_nCrtLine + 1).arg(m_nCrtCol + 1));
}


