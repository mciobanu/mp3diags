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

#include  "RenamerPatternsDlgImpl.h"

#include  "Helpers.h"
#include  "FileRenamerDlgImpl.h"
#include  "StoredSettings.h"


using namespace std;



RenamerPatternsDlgImpl::RenamerPatternsDlgImpl(QWidget* pParent, SessionSettings& settings) : QDialog(pParent, getDialogWndFlags()), Ui::PatternsDlg(), m_settings(settings)
{
    setupUi(this);
    m_pAddPredefB->hide();

    QPalette grayPalette (m_infoM->palette());

    grayPalette.setColor(QPalette::Base, grayPalette.color(QPalette::Disabled, QPalette::Window));

    m_infoM->setPalette(grayPalette);
    m_infoM->setTabStopWidth(fontMetrics().width("%ww"));
    QString qsSep (getPathSep());
    m_infoM->setText("%n\ttrack number\n%a\tartist\n%t\ttitle\n%b\talbum\n%y\tyear\n%g\tgenre\n%r\trating (a lowercase letter)\n%c\tcomposer"
            "\n\nTo include the special characters \"%\", \"[\" and \"]\", precede them by a \"%\": \"%%\", \"%[\" and \"%]\"\n\nThe path should be a full path, starting with a \"/\"");

    int nWidth, nHeight;
    m_settings.loadRenamerPatternsSettings(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 300) { resize(nWidth, nHeight); }
}


RenamerPatternsDlgImpl::~RenamerPatternsDlgImpl()
{
}

/*$SPECIALIZATION$*/

void RenamerPatternsDlgImpl::on_m_pCancelB_clicked()
{
    reject();
}


void RenamerPatternsDlgImpl::on_m_pOkB_clicked()
{
    m_vstrPatterns.clear();
    string s (convStr(m_pTextM->toPlainText()));
    const char* p (s.c_str());
    if (0 == *p) { accept(); return; }

    const char* q (p);
    for (;;)
    {
        if ('\n' == *p || 0 == *p) // ttt1 see if this works on Windows with mingw
        {
            string s1 (q, p - q);
            string strErr;
            try
            {
                Renamer r (s1);
            }
            catch (const Renamer::InvalidPattern& ex)
            {
                strErr = ex.m_strErr;
            }

            if (!strErr.empty())
            {
                QMessageBox::critical(this, "Error", convStr(strErr));
                return;
            }

            m_vstrPatterns.push_back(s1);

            for (; '\n' == *p; ++p) {}
            if (0 == *p) { break; }
            q = p;
        }
        ++p;
    }

    m_settings.saveRenamerPatternsSettings(width(), height());

    accept();
}



bool RenamerPatternsDlgImpl::run(vector<string>& v)
{
    string s;
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        if (!s.empty()) { s += "\n"; }
        s += v[i];
    }
    m_pTextM->setText(convStr(s));
    if (QDialog::Accepted != exec()) { return false; }

    v = m_vstrPatterns;

/*    set<int> sPos;

    vector<pair<string, int> > v1;
    for (int i = 0, n = cSize(m_vPatterns); i < n; ++i)
    {
        int j (0);
        int m (cSize(v));
        for (; j < m; ++j)
        {
            if (m_vPatterns[i] == v[j] && sPos.end() == sPos.find(j))
            {
                sPos.insert(j);
                break;
            }
        }
        if (m == j) { j = -1; }
        v1.push_back(make_pair(m_vPatterns[i], j));
    }

    //v.clear();
    v.swap(v1);*/

    return true;
}
