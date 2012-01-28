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


#include  <string>

#include  <QProcess>
#include  <QScrollBar>
#include  <QMessageBox>
#include  <QCloseEvent>

#include  "ExternalToolDlgImpl.h"

#include  "Widgets.h"
#include  "StoredSettings.h"

#include  "CommonData.h"

////#include  <QFile> // ttt remove


using namespace std;

/*
ttt2 random 30-second freeze: norm completed but not detected; window was set to stay open; abort + close sort of worked, but the main window froze and the program had to be killed; on a second run it looked like the UI freeze is not permanent, but lasts 30 seconds (so waiting some more the first time might have unfrozen the app)

What seems to happen is this: QProcess loses contact with the actual process (or perhaps the program becomes a zombie or something similar, not really finished but not running anymore either); then 2 things happen: first, waitForFinished() doesn't return for 30 seconds (which is the default timeout, and can be made smaller); then, when closing the norm window there's another 30 seconds freeze, probably caused by the dialog destructor's attempt to destroy the QProcess member (which again tries to kill a dead process)

This might be fixed in newer versions of Qt.

Might be related to the comment "!!! needed because sometimes" below, which also suggests that the issue is in Qt rather than MP3 Diags


Note that this also happens during normal operation, even if "abort" is not pressed. The dialog might fail to detect that normalization is done. If that happens, the solution is to press Abort.

ttt2 - perhaps detect that no output is coming from the program, so just assume it's dead; still, the destructor would have to be detached and put on a secondary thread, or just leave a memory leak; (having a timer "clean" a vector with QProcess objects doesn't work, because it would freeze whatever object it's attached to)

ttt2 doc: might seem frozen at the end; just press abort and wait for at most a minute. i'm investigating the cause
same may happen after pressing abort while the normalization is running
*/

ExternalToolDlgImpl::ExternalToolDlgImpl(QWidget* pParent, bool bKeepOpen, SessionSettings& settings, const CommonData* pCommonData, const std::string& strCommandName, const char* szHelpFile) : QDialog(pParent, getDialogWndFlags()), Ui::ExternalToolDlg(), m_pProc(0), m_bFinished(false), m_settings(settings), m_pCommonData(pCommonData), m_strCommandName(strCommandName), m_szHelpFile(szHelpFile)
{
    setupUi(this);
    m_pKeepOpenCkM->setChecked(bKeepOpen);

    int nWidth, nHeight;
    m_settings.loadExternalToolSettings(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 300) { resize(nWidth, nHeight); }

    setWindowTitle(convStr(strCommandName));

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
}


ExternalToolDlgImpl::~ExternalToolDlgImpl()
{
    CursorOverrider crs;
    delete m_pProc;
}


void logTransformation(const string& strLogFile, const char* szActionName, const string& strMp3File);

/*static*/ void ExternalToolDlgImpl::prepareArgs(const QString& qstrCommand, const QStringList& lFiles, QString& qstrProg, QStringList& lArgs)
{
    if (qstrCommand.contains('"'))
    {
        bool bInsideQuotes (false);
        qstrProg.clear();
        QString qstrCrt;
        for (int i = 0; i < qstrCommand.size(); ++i)
        {
            QChar c (qstrCommand[i]);
            if (' ' == c)
            {
                if (bInsideQuotes)
                {
                    qstrCrt += c;
                }
                else
                {
                    if (!qstrCrt.isEmpty())
                    {
                        if (qstrProg.isEmpty())
                        {
                            qstrProg = qstrCrt;
                        }
                        else
                        {
                            lArgs << qstrCrt;
                        }
                        qstrCrt.clear();
                    }
                }
            }
            else if ('"' == c)
            {
                bInsideQuotes = !bInsideQuotes;
                if (!bInsideQuotes && qstrCrt.isEmpty())
                {
                    if (i == qstrCommand.size() - 1 || qstrCommand[i + 1] == ' ')
                    { // add an empty param
                        lArgs << qstrCrt;
                    }
                }
            }
            else
            {
                qstrCrt += c;
            }
        }

        if (!qstrCrt.isEmpty())
        {
            if (qstrProg.isEmpty())
            {
                qstrProg = qstrCrt;
            }
            else
            {
                lArgs << qstrCrt;
            }
            qstrCrt.clear();
        }
        //ttt2 maybe: trigger some error if bInsideQuotes is "false" at the end
        //ttt3 maybe allow <<ab "" cd>> to be interpreted as 3 params, with the second one empty; currently it is interpreted as 2 non-empty params
    }
    else
    {
        int k (1);
        for (; k < qstrCommand.size() && (qstrCommand[k - 1] != ' '  || (qstrCommand[k] != '-' && qstrCommand[k] != '/')); ++k) {} //ttt2 perhaps better: look for spaces from the end and stop when a dir exists from the beginning of the name till the current space
        qstrProg = qstrCommand.left(k).trimmed();
        QString qstrArg (qstrCommand.right(qstrCommand.size() - k).trimmed());
        //qDebug("prg <%s>  arg <%s>", qstrProg.toUtf8().constData(), qstrArg.toUtf8().constData());

        lArgs = qstrArg.split(" ", QString::SkipEmptyParts); // ttt2 perhaps accomodate params that contain spaces, but mp3gain doesn't seem to need them;
        //QString qstrName (l.front());
        //l.removeFirst();
    }
    lArgs << lFiles;
}


void ExternalToolDlgImpl::run(const QString& qstrProg1, const QStringList& lFiles) //ttt2 in Windows MP3Gain doesn't seem to care about Unicode (well, the GUI version does, but that doesn't help). aacgain doesn't work either; see if there's a good way to deal with this; doc about using short filenames
{
    m_pProc = new QProcess(this);
    //m_pProc = new QProcess(); // !!! m_pProc is not owned; it will be destroyed
    connect(m_pProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished()));
    connect(m_pProc, SIGNAL(readyReadStandardOutput()), this, SLOT(onOutputTxt()));
    connect(m_pProc, SIGNAL(readyReadStandardError()), this, SLOT(onErrorTxt()));

    if (m_pCommonData->m_bLogTransf)
    {
        for (int i = 0; i < lFiles.size(); ++i)
        {
            logTransformation(m_pCommonData->m_strTransfLog, m_strCommandName.c_str(), convStr(lFiles[i]));
        }
    }

    QString qstrProg;
    QStringList l;
    prepareArgs(qstrProg1, lFiles, qstrProg, l);

    m_pProc->start(qstrProg, l);

    if (!m_pProc->waitForStarted(5000))
    {
        QMessageBox::critical(this, "Error", "Cannot start process. Check that the executable name and the parameters are correct.");
        return;
    }

    {
        QAction* p (new QAction(this));
        p->setShortcut(QKeySequence(Qt::Key_Escape));
        connect(p, SIGNAL(triggered()), this, SLOT(on_m_pAbortB_clicked()));
        addAction(p);
    }

    exec();

    m_settings.saveExternalToolSettings(width(), height());
}


void ExternalToolDlgImpl::onOutputTxt()
{
    addText(m_pProc->readAllStandardOutput()); //ttt2 perhaps use different colors for std and err, or use 2 separate text boxes
}


void ExternalToolDlgImpl::onErrorTxt()
{
    //addText("####" + m_pProc->readAllStandardError());
    QString s (m_pProc->readAllStandardError().trimmed());
    if (s.isEmpty()) { return; }
//qDebug("err %s", s.toUtf8().constData());
    //inspect(s.toUtf8().constData(), s.size());
    int n (s.lastIndexOf("\r"));
    if (n > 0)
    {
        s.remove(0, n + 1);
    }

    n = s.lastIndexOf("\n");
    if (n > 0)
    {
        s.remove(0, n + 1);
    }
    //inspect(s.toUtf8().constData(), s.size());
    while (!s.isEmpty() && s[0] == ' ') { s.remove(0, 1); }

    m_pDetailE->setText(s);
}


void ExternalToolDlgImpl::addText(QString s)
{
    s = s.trimmed();
    if (s.isEmpty()) { return; }

    for (;;)
    {
        int n (s.indexOf("\n\n"));
        if (-1 == n) { break; }
        s.remove(n, 1);
    }

    m_qstrText = (m_qstrText.isEmpty() ? s : m_qstrText + "\n" + s);

    m_pOutM->setText(m_qstrText);

    QScrollBar* p (m_pOutM->verticalScrollBar());
    if (p->isVisible())
    {
        p->setValue(p->maximum());
    }
}


void ExternalToolDlgImpl::onFinished()
{
    if (m_bFinished) { return; } // !!! needed because sometimes terminating with kill() triggers onFinished() and sometimes it doesn't
    m_bFinished = true;
    // !!! doesn't need to destroy m_pProc and QAction, because they will be destroyed anyway when the dialog will be destroyed, which is going to be pretty soon
    if (m_pKeepOpenCkM->isChecked()) //ttt2 perhaps save in config
    {
        addText("==================================\nFinished");
        m_pDetailE->setText("");
    }
    else
    {
        reject();
    }
}

void ExternalToolDlgImpl::on_m_pCloseB_clicked()
{
    if (!m_bFinished)
    {
        QMessageBox::warning(this, "Warning", convStr("Cannot close while \"" + m_strCommandName + "\" is running."));
        return;
    }
    accept();
}


void ExternalToolDlgImpl::on_m_pAbortB_clicked()
{
qDebug("proc state %d", int(m_pProc->state()));
    if (m_bFinished)
    {
        on_m_pCloseB_clicked();
        return;
    }

    if (0 == showMessage(this, QMessageBox::Warning, 1, 1, "Confirm", convStr("Stopping \"" + m_strCommandName + "\" may leave the files in an inconsistent state or may prevent temporary files from being deleted. Are you sure you want to abort " + m_strCommandName + "?"), "Yes, abort", "Don't abort"))
    {
        CursorOverrider crs;
        m_pProc->kill();
        m_pProc->waitForFinished(5000);
        onFinished();
    }
}


// !!! Can't allow the top-right close button or the ESC key to close the dialog, because that would kill the thread too and leave everything in an inconsistent state. So the corresponding events are intercepted and "ignore()"d and abort() is called instead

/*override*/ void ExternalToolDlgImpl::closeEvent(QCloseEvent* pEvent)
{
    /*
    Not sure if this should work: from the doc for QDialog:

    Escape Key
    If the user presses the Esc key in a dialog, QDialog::reject() will be called. This will cause the window to close: The close event cannot be ignored.

    ttt2 see Qt::Key_Escape in MainFormDlgImpl for a different approach, decide which is better
    */
    pEvent->ignore();
    on_m_pCloseB_clicked();
//    on_m_pAbortB_clicked();
}

#if 0
/*override*/ void ExternalToolDlgImpl::keyPressEvent(QKeyEvent* pEvent)
{
//qDebug("key prs %d", pEvent->key());

    m_nLastKey = pEvent->key();

    pEvent->ignore(); // ttt2 not sure this is the way to do it, but the point is to disable the ESC key
}


/*override*/ void ExternalToolDlgImpl::keyReleaseEvent(QKeyEvent* pEvent)
{
//qDebug("key rel %d", pEvent->key());
    if (Qt::Key_Escape == pEvent->key())
    {
        on_m_pAbortB_clicked();
    }
    pEvent->ignore(); // ttt2 not sure this is the way to do it, but the point is to disable the ESC key
}
#endif


void ExternalToolDlgImpl::onHelp()
{
    openHelp(m_szHelpFile);
}

//ttt2 timer in normalizer
//ttt2 look at normalized loudness in tracks, maybe warn

//ttt1 non-ASCII characters are not shown correctly

