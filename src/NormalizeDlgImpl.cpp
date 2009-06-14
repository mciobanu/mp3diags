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

#include  "NormalizeDlgImpl.h"

#include  "Widgets.h"
#include  "StoredSettings.h"

#include  "CommonData.h"

////#include  <QFile> // ttt remove


using namespace std;



NormalizeDlgImpl::NormalizeDlgImpl(QWidget* pParent, bool bKeepOpen, SessionSettings& settings, const CommonData* pCommonData) : QDialog(pParent, getDialogWndFlags()), Ui::NormalizeDlg(), m_bFinished(false), m_settings(settings), m_pCommonData(pCommonData)
{
    setupUi(this);
    m_pKeepOpenCkM->setChecked(bKeepOpen);

    int nWidth, nHeight;
    m_settings.loadNormalizeSettings(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 300) { resize(nWidth, nHeight); }
}


NormalizeDlgImpl::~NormalizeDlgImpl()
{
}


void logTransformation(const string& strLogFile, const char* szActionName, const string& strMp3File);


void NormalizeDlgImpl::normalize(const QString& qstrProg, const QStringList& lFiles)
{
    m_pProc = new QProcess(this);
    connect(m_pProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished()));
    connect(m_pProc, SIGNAL(readyReadStandardOutput()), this, SLOT(onOutputTxt()));
    connect(m_pProc, SIGNAL(readyReadStandardError()), this, SLOT(onErrorTxt()));

    if (m_pCommonData->m_bLogTransf)
    {
        for (int i = 0; i < lFiles.size(); ++i)
        {
            logTransformation(m_pCommonData->m_strTransfLog, "Normalize", convStr(lFiles[i]));
        }
    }

    QStringList l (qstrProg.split(" ", QString::SkipEmptyParts)); // ttt2 perhaps accomodate params that contain spaces, but mp3gain doesn't seem to need them;
    QString qstrName (l.front());
    l.removeFirst();
    l << lFiles;
    m_pProc->start(qstrName, l);

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

    m_settings.saveNormalizeSettings(width(), height());
}


void NormalizeDlgImpl::onOutputTxt()
{
    addText(m_pProc->readAllStandardOutput()); //ttt2 perhaps use different colors for std and err, or use 2 separate text boxes
}


void NormalizeDlgImpl::onErrorTxt()
{
    //addText("####" + m_pProc->readAllStandardError());
    QString s (m_pProc->readAllStandardError().trimmed());
    if (s.isEmpty()) { return; }
    m_pDetailE->setText(s);
}


void NormalizeDlgImpl::addText(QString s)
{
/*QFile f ("abc.txt");
f.open(QIODevice::Append);
f.write(s.toLatin1());*/

    s = s.trimmed();
    if (s.isEmpty()) { return; }

    for (;;)
    {
        int n (s.indexOf("\n\n"));
        if (-1 == n) { break; }
        s.remove(n, 1);
    }

    //QString q (m_qstrText.isEmpty() ? s : m_qstrText + "\n" + s);
    m_qstrText = (m_qstrText.isEmpty() ? s : m_qstrText + "\n" + s);

    m_pOutM->setText(m_qstrText);

    /*if (!s.startsWith("[")) // ttt1 mp3gain specific, but should work ok with others too
    {
        m_qstrText = q;
    }*/

#if 0
    QString q (m_pOutM->toPlainText());
    if (!q.isEmpty()) { q += "\n"; }
    /*{
        //time_t t (time(0));
        tm t;
        timeval tv;
        gettimeofday(&tv, 0);
        localtime_r(&tv.tv_sec, &t);
        char a [15];
        sprintf(a, "%02d:%02d:%02d.%03d ", t.tm_hour, t.tm_min, t.tm_sec, int(tv.tv_usec/1000));
        q += a;
    }*/
    q += s;
    while (q.endsWith("\n"))
    {
        q.remove(q.size() - 1, 1);
    }
    m_pOutM->setText(q);
#endif

    QScrollBar* p (m_pOutM->verticalScrollBar());
    if (p->isVisible())
    {
        p->setValue(p->maximum());
    }
}


void NormalizeDlgImpl::onFinished()
{
    if (m_bFinished) { return; } // !!! needed because sometimes terminating with kill() triggers onFinished() and sometimes it doesn't
    m_bFinished = true;
    // !!! doesn't need to destroy m_pProc and QAction, because they will be destroyed anyway when the dialog will be destroyed, which is going to be pretty soon
    if (m_pKeepOpenCkM->isChecked())
    {
        addText("==================================\nFinished");
        m_pDetailE->setText("");
    }
    else
    {
        reject();
    }
}

void NormalizeDlgImpl::on_m_pCloseB_clicked()
{
    if (!m_bFinished)
    {
        QMessageBox::warning(this, "Warning", "Cannot close while the normalization is running.");
        return;
    }
    accept();
}


void NormalizeDlgImpl::on_m_pAbortB_clicked()
{
    if (m_bFinished)
    {
        on_m_pCloseB_clicked();
        return;
    }

    if (0 == showMessage(this, QMessageBox::Warning, 1, 1, "Confirm", "Stopping normalization may leave the files in an inconsistent state or may prevent temporary files from being deleted. Are you sure you want to abort the normalization?", "Yes, abort", "Don't abort"))
    {
        m_pProc->kill();
        m_pProc->waitForFinished();
        onFinished();
    }
}


// !!! Can't allow the top-right close button or the ESC key to close the dialog, because that would kill the thread too and leave everything in an inconsistent state. So the corresponding events are intercepted and "ignore()"d and abort() is called instead

/*override*/ void NormalizeDlgImpl::closeEvent(QCloseEvent* pEvent)
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
/*override*/ void NormalizeDlgImpl::keyPressEvent(QKeyEvent* pEvent)
{
//qDebug("key prs %d", pEvent->key());

    m_nLastKey = pEvent->key();

    pEvent->ignore(); // ttt2 not sure this is the way to do it, but the point is to disable the ESC key
}


/*override*/ void NormalizeDlgImpl::keyReleaseEvent(QKeyEvent* pEvent)
{
//qDebug("key rel %d", pEvent->key());
    if (Qt::Key_Escape == pEvent->key())
    {
        on_m_pAbortB_clicked();
    }
    pEvent->ignore(); // ttt2 not sure this is the way to do it, but the point is to disable the ESC key
}
#endif



