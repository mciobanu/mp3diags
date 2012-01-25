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


#ifndef ExternalToolDlgImplH
#define ExternalToolDlgImplH

#include  <string>

#include  <QDialog>

#include  "ui_ExternalTool.h"

class QProcess;

class SessionSettings;
class CommonData;


class ExternalToolDlgImpl : public QDialog, private Ui::ExternalToolDlg
{
    Q_OBJECT

    QProcess* m_pProc;
    bool m_bFinished;
    void addText(QString);

    int m_nLastKey;

    //QString m_qstrLastLine;
    QString m_qstrText;
    SessionSettings& m_settings;
    const CommonData* m_pCommonData; // for logging

    const std::string m_strCommandName;
    const char* m_szHelpFile;

    /*override*/ void closeEvent(QCloseEvent* pEvent);
    // /*override*/ void keyPressEvent(QKeyEvent* pEvent);
    // /*override*/ void keyReleaseEvent(QKeyEvent* pEvent);

public:
    ExternalToolDlgImpl(QWidget* pParent, bool bKeepOpen, SessionSettings& settings, const CommonData* pCommonData, const std::string& strCommandName, const char* szHelpFile);
    ~ExternalToolDlgImpl();

    void run(const QString& qstrProg, const QStringList& lFiles); // it would make sense to return true if something was changed (or even exactly what changed), but that's not easy to do, so the caller should just assume that something changed
    static void prepareArgs(const QString& qstrCommand, const QStringList& lFiles, QString& qstrProg, QStringList& args);

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/
    void onFinished();
    void onOutputTxt();
    void onErrorTxt();
    void on_m_pCloseB_clicked();
    void on_m_pAbortB_clicked();

    void onHelp();
};

#endif

