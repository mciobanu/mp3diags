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


#ifndef ThreadRunnerDlgImplH
#define ThreadRunnerDlgImplH

#include <QDialog>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>

#include <vector>

#include "ui_ThreadRunner.h"



typedef QList<QString> StrList;

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

/*
This class is supposed to be derived from to create the actual thread class, then instantiated and passed to a ThreadRunnerDlgImpl, which will handle the thread.

Usage:

A) The user thread calls "pause()", "resume()" or "abort()" as needed.

B) The class derived from PausableThread must declare a CompleteNotif at the beginning of its "run()" function.

C) The class derived from PausableThread should do these things periodically in run():
    1) have something like this: "if (isAborted()) return;"
    2) call "checkPause()"; this call blocks if the user called "pause()"
    3) emit "stepChanged()", to allow progress to be displayed in the associated dialog

D) The class derived from PausableThread must call "setSuccess(true)" on the CompleteNotif variable just before exiting, if the execution was successful.


Notes:
- the user thread shouldn't make any assumptions about how long it would take for a step in PausableThread, but this should not be long (less than 0.1 s)
- in order to help performance it is possible that one more step in PausableThread will be executed after sending pause(); (functionally this shouldn't matter, because it's the same as if pause() was called just after PausableThread checked if it should pause)

*/
class PausableThread : public QThread
{
    Q_OBJECT

    QMutex m_mutex;
    QWaitCondition m_waitCondition;

    bool m_bPaused;  // set by pause() / resume() ; derived classes should check it periodically by calling isPaused()
    bool m_bAborted; // set by abort() ; derived classes should check it periodically by calling isAborted()

protected:
    //bool isPaused() const { return m_bPaused; } // !!! no synch needed
    class CompleteNotif
    {
        bool m_bSuccess;
        PausableThread* m_pThread;
    public:
        CompleteNotif(PausableThread* pThread) : m_bSuccess(false), m_pThread(pThread) {}
        ~CompleteNotif() { m_pThread->notifComplete(m_bSuccess); }
        void setSuccess(bool bSuccess) { m_bSuccess = bSuccess; }
    };

public:
    PausableThread(/*QObject* pParent = 0*/);
    virtual ~PausableThread();

    bool isAborted() const { return m_bAborted; } // !!! no synch needed
    void checkPause(); // if m_bPaused is set, it waits until resume() or abort() get called; otherwise it returns immediately
    //void emitStepChanged(const QString& qstrLabel1, const QString& qstrLabel2 = "", const QString& qstrLabel3 = "", const QString& qstrLabel4 = "") { emit stepChanged(qstrLabel1, qstrLabel2, qstrLabel3, qstrLabel4); }
    void emitStepChanged(const StrList& v, int nStep = -1) { emit stepChanged(v, nStep); }

//public slots:
//private slots:
    void pause();
    void resume();

    void abort();

private:
    friend class ThreadRunnerDlgImpl;
    friend class PausableThread::CompleteNotif;
    void notifComplete(bool bSuccess) { emit completed(bSuccess); }

signals:
    //void stepChanged(const QString& qstrLabel1, const QString& qstrLabel2, const QString& qstrLabel3, const QString& qstrLabel4);
    void stepChanged(const StrList& v, int nStep); // normally nStep should be -1, which causes the steps to be handled automatically;
    void completed(bool bSuccess);
};


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


/*
Takes a PausableThread* on the constructor and sets itself as the owner (so the thread should be created with "new()")

The thread must not be already started. It starts the thread and has buttons for pause/resume and abort.

Closes itself when the thread completes.

----------

There are 2 events that can't be allow to work as usual:
    1) clicking the top-right "Close" button
    2) pressing ESC (which normally triggers the closing)

Allowing the window ro close would kill the thread too and leave everything in an inconsistent state. So on_m_pAbortB_clicked() is called instead.

To deal with Close, closeEvent() is overridden.

To prevent ESC from closing the dialog, Qt::Key_Escape is set as a shortcut for on_m_pAbortB_clicked(). (Alternatively, keyPressEvent and keyReleaseEvent could be used, but it's more complicated.)


*/
class ThreadRunnerDlgImpl : public QDialog, private Ui::ThreadRunnerDlg
{
    Q_OBJECT

    PausableThread* m_pThread;
    bool m_bSuccess;
    QTimer m_closeTimer;
    QTimer m_updateTimer;
    int m_nCounter;
    bool m_bShowCounter;

    time_t m_tRealBegin;
    time_t m_tRunningBegin;
    time_t m_tPauseBegin;

    StrList m_vStepInfo;
    bool m_bShowPauseAbort;
    bool m_bFirstTime;

public:
    enum TruncatePos { TRUNCATE_BEGIN, TRUNCATE_MIDDLE, TRUNCATE_END };

private:
    /*override*/ void closeEvent(QCloseEvent* pEvent); // needed to sort of disable the close button; the event gets ignored and on_m_pAbortB_clicked() gets called instead
#if 0
    int m_nLastKey;
    /*override*/ void keyPressEvent(QKeyEvent* pEvent);
    /*override*/ void keyReleaseEvent(QKeyEvent* pEvent);
#endif
    TruncatePos m_eTruncatePos;

    QString truncateLarge(const QString& s, int nKeepFirst = 0); // truncates strings that are too wide to display without resizing; uses m_eTruncatePos to determine where to truncate
public:
    enum { HIDE_COUNTER, SHOW_COUNTER };
    enum { HIDE_PAUSE_ABORT, SHOW_PAUSE_ABORT };
    //ThreadRunnerDlgImpl(PausableThread* pThread, bool bShowCounter, QWidget* pParent = 0, Qt::WFlags fl = 0);
    ThreadRunnerDlgImpl(QWidget* pParent, Qt::WFlags flags, PausableThread* pThread, bool bShowCounter, TruncatePos eTruncatePos, bool bShowPauseAbort = true);
    ~ThreadRunnerDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    //void startThread(); // !!! start automatically
    /*void pauseThread();
    void resumeThread();
    void abortThread();*/

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

private slots:
    void onThreadCompleted(bool bSuccess);
    void on_m_pPauseResumeB_clicked();
    void on_m_pAbortB_clicked();
    //void onStepChanged(const QString& qstrLabel1, const QString& qstrLabel2 = "", const QString& qstrLabel3 = "", const QString& qstrLabel4 = "");
    void onStepChanged(const StrList& v, int nStep);
    void onCloseTimer();
    void onUpdateTimer();

    void onHelp();
};

#endif // #ifndef ThreadRunnerDlgImplH



