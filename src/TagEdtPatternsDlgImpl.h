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


#ifndef TagEdtPatternsDlgImplH
#define TagEdtPatternsDlgImplH

#include  <QDialog>

#include  "ui_Patterns.h"

#include  <vector>
#include  <string>

class SessionSettings;

class TagEdtPatternsDlgImpl : public QDialog, private Ui::PatternsDlg
{
    Q_OBJECT

    std::vector<std::string> m_vPatterns;
    SessionSettings& m_settings;

    const std::vector<std::string>& m_vstrPredef;

public:
    TagEdtPatternsDlgImpl(QWidget* pParent, SessionSettings& settings, const std::vector<std::string>& vstrPredef);
    ~TagEdtPatternsDlgImpl();

    bool run(std::vector<std::pair<std::string, int> >&); // the int is ignored for input; for output, it tells which position a given pattern occupied in the input, or -1 if it's a new string
protected slots:

    void on_m_pOkB_clicked();
    void on_m_pCancelB_clicked();
    void on_m_pAddPredefB_clicked();

    void onHelp();
};

#endif // #ifndef TagEdtPatternsDlgImplH

