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


#include  <QCloseEvent>
#include  <QLocale>

#include  <ctime>

#include  "ThreadRunnerDlgImpl.h"

#include  "Helpers.h"


PausableThread::PausableThread(/*QObject* pParent = 0*/) : /*QThread(pParent),*/ m_bPaused(false), m_bAborted(false)
{
    static bool s_bRegistered (false);
    if (!s_bRegistered)
    {
        s_bRegistered = true;
        qRegisterMetaType<StrList>("StrList");
    }
}


/*virtual*/ PausableThread::~PausableThread()
{
//qDebug("thread destroyed");
}

void PausableThread::pause()
{
    m_bPaused = true; // !!! no synch needed
}


void PausableThread::resume()
{
    QMutexLocker lck(&m_mutex);
    m_bPaused = false;
    m_waitCondition.wakeAll();
}


void PausableThread::abort()
{
    QMutexLocker lck(&m_mutex);
    m_bAborted = true;
    m_waitCondition.wakeAll();
}


void PausableThread::checkPause() // if m_bPaused is set, it waits until resume() or abort() get called; otherwise it returns immediately
{
    if (!m_bPaused) { return; }

    QMutexLocker lck(&m_mutex);
    if (!m_bPaused) { return; } // !!! it was tested 2 lines above, but might have changed after that; now it's a different story, because it's protected by the mutex;
    if (m_bAborted) { return; }

    m_waitCondition.wait(&m_mutex);
}


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================




ThreadRunnerDlgImpl::ThreadRunnerDlgImpl(QWidget* pParent, Qt::WFlags flags, PausableThread* pThread, bool bShowCounter, TruncatePos eTruncatePos, bool bShowPauseAbort /*= true*/) :
    QDialog(pParent, flags),
    Ui::ThreadRunnerDlg(),

    m_pThread(pThread),
    m_nCounter(0),
    m_bShowCounter(bShowCounter),
    //m_nLastKey(0),

    m_tRealBegin(time(0)),
    m_tRunningBegin(time(0)),
    m_bShowPauseAbort(bShowPauseAbort),
    m_bFirstTime(true),
    m_eTruncatePos(eTruncatePos)
{
    setupUi(this);

    if (!bShowPauseAbort)
    {
        m_pPauseResumeB->hide();
        m_pAbortB->hide();
    }

    pThread->setParent(this);

    connect(m_pThread, SIGNAL(stepChanged(const StrList&, int)), this, SLOT(onStepChanged(const StrList&, int)));
    connect(m_pThread, SIGNAL(completed(bool)), this, SLOT(onThreadCompleted(bool)));
    connect(&m_closeTimer, SIGNAL(timeout()), this, SLOT(onCloseTimer()));
    connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimer()));

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence(Qt::Key_Escape)); connect(p, SIGNAL(triggered()), this, SLOT(on_m_pAbortB_clicked())); addAction(p); } // !!! make ESC call on_m_pAbortB_clicked() instead of closing the dialog

    m_pCurrentL->setText("");

    pThread->start();
    m_updateTimer.start(500); // 0.5 sec

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
}

ThreadRunnerDlgImpl::~ThreadRunnerDlgImpl()
{
}



void ThreadRunnerDlgImpl::onThreadCompleted(bool bSuccess)
{
    m_bSuccess = bSuccess;

    // !!! can't just exit; should wait for the thread's "run()" metod to finish
    m_pCurrentL->setText("Completed");
    m_pPauseResumeB->setEnabled(false);
    m_pAbortB->setEnabled(false);
    m_closeTimer.start(100); // 0.1 sec
    onCloseTimer(); // this may return immediately or not, depending on which thread gets executed after "PausableThread::notifComplete()"
}


void ThreadRunnerDlgImpl::on_m_pPauseResumeB_clicked()
{
    if (!m_pThread->m_bPaused)
    { // currently running
        m_pPauseResumeB->setText("&Resume");
        m_pThread->pause();

        m_tPauseBegin = time(0);
    }
    else
    { // currently paused
        m_pPauseResumeB->setText("&Pause");

        time_t t (time(0));
        m_tRunningBegin = m_tRunningBegin + t - m_tPauseBegin;
        m_pThread->resume();
    }
}


void ThreadRunnerDlgImpl::on_m_pAbortB_clicked()
{
    m_pPauseResumeB->setEnabled(false);
    m_pAbortB->setEnabled(false);
    m_pThread->abort();
}


QString ThreadRunnerDlgImpl::truncateLarge(const QString& s, int nKeepFirst /*= 0*/) // truncates strings that are too wide to display without resizing
{
    QFontMetrics fontMetrics (m_pCurrentL->font());
    const int MARGIN (8); // normally this should be 0 but in other cases Qt missed a few pixels when estimating how much space it needed, so it seems better to lose some pixels // ttt2 2009.04.30 - actually this is probably related to spacing; if this is true, the hard-coded value should be replaced by some query to QApplication::style()->pixelMetric()

    if (fontMetrics.width(s) < m_pCurrentL->width() - MARGIN)
    {
        return s;
    }

    int nSize (s.size() - 1);
    QString res (s);

    switch (m_eTruncatePos)
    {
    case TRUNCATE_BEGIN:
        {
            res.insert (nKeepFirst, "... ");
            nKeepFirst += 4; // size of "... "
            nSize -= nKeepFirst;
            while (nSize > 0 && fontMetrics.width(res) >= m_pCurrentL->width() - MARGIN)
            {
                res.remove(nKeepFirst, 1);
                --nSize;
            }
            return res;
        }

    case TRUNCATE_END:
        {
            while (nSize > 0 && fontMetrics.width(res + " ...") >= m_pCurrentL->width() - MARGIN)
            {
                res.truncate(nSize);
                --nSize;
            }
            return res + " ...";
        }

    default:
        //CB_ASSERT (false); //ttt2 add support for TRUNCATE_MIDDLE
        throw false;
    }
}


//void ThreadRunnerDlgImpl::onStepChanged(const QString& qstrLabel1, const QString& qstrLabel2 /*= ""*/, const QString& qstrLabel3 /*= ""*/, const QString& qstrLabel4 /*= ""*/)
void ThreadRunnerDlgImpl::onStepChanged(const StrList& v, int nStep)
{
//qDebug("step %s", qstrLabel.toStdString().c_str());
    if (-1 == nStep)
    {
        ++m_nCounter;
    }
    else
    {
        m_nCounter = nStep;
    }

    m_vStepInfo = v;
    if (m_bFirstTime)
    {
        QTimer::singleShot(1, this, SLOT(onUpdateTimer()));
        m_bFirstTime = false;
    }
}


static QString getTimeFmt(int n) // n in seconds
{
    int h (n / 3600); n -= h*3600;
    int m (n / 60); n -= m*60;
    QString q;
    q.sprintf("%d:%02d:%02d", h, m, n);
    return q;
}


void ThreadRunnerDlgImpl::onCloseTimer()
{
//qDebug("ThreadRunnerDlgImpl::onTimer()");
    if (!m_pThread->isFinished())
    {
//qDebug("waithing for thread");
        return;
    }

    m_closeTimer.stop();
    m_updateTimer.stop();

    if (m_bSuccess)
    {
        accept();
    }
    else
    {
        reject();
    }
}

void ThreadRunnerDlgImpl::onUpdateTimer()
{
    QString s;
    if (m_bShowCounter)
    {
        static QLocale loc ("C");
        s = loc.toString(m_nCounter) + ". ";
    }

    int n ((int)m_vStepInfo.size());
    if (n >= 1)
    {
        s = truncateLarge(s + m_vStepInfo[0], s.size());

        for (int i = 1; i < n; ++i)
        {
            s += "\n" + truncateLarge(m_vStepInfo[i]);
        }
    }

    time_t t (time(0));
    if (m_bShowPauseAbort)
    {
        s += "\n\nTotal time: " + getTimeFmt(t - m_tRealBegin);
        s += "\nRunning time: " + getTimeFmt((m_pThread->m_bPaused ? m_tPauseBegin : t) - m_tRunningBegin);
    }
    else
    {
        s += "\n\nTime: " + getTimeFmt(t - m_tRealBegin);
    }

    m_pCurrentL->setText(s);
}


/*override*/ void ThreadRunnerDlgImpl::closeEvent(QCloseEvent* pEvent)
{
    pEvent->ignore();
    on_m_pAbortB_clicked();
}


void ThreadRunnerDlgImpl::onHelp()
{
    // openHelp("index.html"); //ttt2 see if anything more specific can be done
}



#if 0

/*
Not sure if this should work: from the doc for QDialog:

Escape Key
If the user presses the Esc key in a dialog, QDialog::reject() will be called. This will cause the window to close: The close event cannot be ignored.

ttt2 see Qt::Key_Escape in MainFormDlgImpl for a different approach, decide which is better
*/

/*override*/ void ThreadRunnerDlgImpl::keyPressEvent(QKeyEvent* pEvent)
{
//qDebug("key prs %d", pEvent->key());

    m_nLastKey = pEvent->key();

    pEvent->ignore();
}


/*override*/ void ThreadRunnerDlgImpl::keyReleaseEvent(QKeyEvent* pEvent)
{
//qDebug("key rel %d", pEvent->key());
    if (Qt::Key_Escape == pEvent->key())
    {
        on_m_pAbortB_clicked();
    }
    pEvent->ignore(); // ttt2 not sure this is the way to do it, but the point is to disable the ESC key
}

#endif
