

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


#include  <algorithm>

#include  <QMouseEvent>
#include  <QBoxLayout>
#include  <QPushButton>
#include  <QApplication>
#include  <QTextBrowser>
#include  <QCheckBox>

#include  "Widgets.h"

#include  "Helpers.h"


using namespace std;
using namespace pearl;



/*override*/ void ModifInfoMenu::mousePressEvent(QMouseEvent *pEvent)
{
    m_modif = pEvent->modifiers();
//int x (m_modif); qDebug("modif %d", x);
    if (pEvent->button() == Qt::RightButton)
    {
        m_modif |= Qt::ShiftModifier;
    }
    QMenu::mousePressEvent(pEvent);
}


ModifInfoToolButton::ModifInfoToolButton(QToolButton* pOldBtn) : QToolButton(pOldBtn->parentWidget())
{
    QWidget* pParent (pOldBtn->parentWidget());
    QWidgetList lpSiblings (pParent->findChildren<QWidget*>());

    setAutoRaise(pOldBtn->autoRaise());
    setIcon(pOldBtn->icon());
    setMinimumSize(pOldBtn->minimumSize());
    setMaximumSize(pOldBtn->maximumSize());
    setFocusPolicy(pOldBtn->focusPolicy());
    setIconSize(pOldBtn->iconSize());
    setToolTip(pOldBtn->toolTip());
    //ttt2 set other properties

    QBoxLayout* pLayout (dynamic_cast<QBoxLayout*>(pParent->layout())); //ttt2 see about other layouts
    CB_ASSERT (0 != pLayout);
    int nPos (pLayout->indexOf(pOldBtn));
    pLayout->insertWidget(nPos, this);

    //connect(this, SIGNAL(clicked()), that, SLOT(on_x_clicked())); //ttt2 would be nice to take over the signals, but doesn't seem possible

    delete pOldBtn;
}


/*override*/ void ModifInfoToolButton::mousePressEvent(QMouseEvent *pEvent)
{
    m_modif = pEvent->modifiers();
//int x (m_modif); qDebug("modif %d", x);
    QToolButton::mousePressEvent(pEvent);
}


/*override*/ void ModifInfoToolButton::keyPressEvent(QKeyEvent* pEvent)
{
    m_modif = pEvent->modifiers(); // ttt1 actually this never seems to get triggered; see if it's true (perhaps it matters if the button can get keyboard focus)
//int x (m_modif); qDebug("modif %d", x);
    QToolButton::keyPressEvent(pEvent);
}


void ModifInfoToolButton::contextMenuEvent(QContextMenuEvent* /*pEvent*/)
{
    //m_modif = pEvent->modifiers();
    m_modif = m_modif | Qt::ShiftModifier;
    //emit clicked();
}



int showMessage(QWidget* pParent, QMessageBox::Icon icon, int nDefault, int nEscape, const QString& qstrTitle, const QString& qstrMessage, const QString& qstrButton0, const QString& qstrButton1 /*= ""*/, const QString& qstrButton2 /*= ""*/, const QString& qstrButton3 /*= ""*/)
{
    QStringList l;
    l << qstrButton0;
    if (!qstrButton1.isEmpty())
    {
        l << qstrButton1;
        if (!qstrButton2.isEmpty())
        {
            l << qstrButton2;
            if (!qstrButton3.isEmpty())
            {
                l << qstrButton3;
            }
        }
    }
    QMessageBox msgBox (pParent);
    vector<QPushButton*> v;
    for (int i = 0; i < l.size(); ++i)
    {
        v.push_back(msgBox.addButton(l[i], QMessageBox::ActionRole)); // ActionRole is meaningless, but had to use something
        if (cSize(v) - 1 == nEscape)
        {
            msgBox.setEscapeButton(v.back());
        }
        if (cSize(v) - 1 == nDefault)
        {
            msgBox.setDefaultButton(v.back());
        }
    }

    msgBox.setText(qstrMessage);
    msgBox.setIcon(icon);
    msgBox.setTextFormat(Qt::PlainText);
    msgBox.setWindowTitle(qstrTitle);
    msgBox.exec();
    return find(v.begin(), v.end(), msgBox.clickedButton()) - v.begin();
}


CursorOverrider::CursorOverrider(Qt::CursorShape crs /*= Qt::BusyCursor*/)
{
    QApplication::setOverrideCursor(QCursor(crs));
}


CursorOverrider::~CursorOverrider()
{
    QApplication::restoreOverrideCursor();
}



//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================



#if 0
struct ThreadLocalDlgList
{
    virtual void addDlg(QDialog* pDlg) = 0;
    virtual void removeDlg(QDialog* pDlg) = 0;
    virtual QDialog* getDlg() = 0;
};

ThreadLocalDlgList& getThreadLocalDlgList(); // a unique, global, object

struct DlgRef
{
    DlgRef(QDialog* pDlg) : m_pDlg(pDlg)
    {
        getThreadLocalDlgList().addDlg(pDlg);
    }

    ~DlgRef()
    {
        getThreadLocalDlgList().removeDlg(m_pDlg);
    }

private:
    QDialog* m_pDlg;
};
#endif





#if 0

struct DlgList
{
    void addDlg(QDialog* pDlg)
    {
        m_stpDlg.push(pDlg);
    }

    void removeDlg(QDialog* pDlg)
    {
        CB_ASSERT (!m_stpDlg.empty());
        CB_ASSERT (m_stpDlg.top() == pDlg);
        m_stpDlg.pop();
    }

    QDialog* getDlg() // may return 0
    {
        if (m_stpDlg.empty()) { return 0; }
        return m_stpDlg.top();
    }

    ~DlgList()
    {
        CB_ASSERT (m_stpDlg.empty()); //ttt 0 see if right, if called from ASSERT
    }
private:
    stack<QDialog*> m_stpDlg;
};



class ThreadLocalDlgListImpl : public ThreadLocalDlgList
{
    QThreadStorage<DlgList*> m_storage;
    void addStorageIfNeeded()
    {
        if (!m_storage.hasLocalData())
        {
            m_storage.setLocalData(new DlgList());
        }
    }

public:
    /*override*/ void addDlg(QDialog* pDlg)
    {
        addStorageIfNeeded();
        m_storage.localData()->addDlg(pDlg);
    }

    /*override*/ void removeDlg(QDialog* pDlg)
    {
        addStorageIfNeeded();
        m_storage.localData()->removeDlg(pDlg);
    }

    /*override*/ QDialog* getDlg()
    {
        addStorageIfNeeded();
        return m_storage.localData()->getDlg();
    }
};


ThreadLocalDlgList& getThreadLocalDlgList()
{
    static ThreadLocalDlgListImpl s_lst;
    return s_lst;
}

#endif



/*static*/ int HtmlMsg::msg(QWidget* pParent, int nDefault, int nEscape, bool* pbGotTheMessage, int nFlags, const QString& qstrTitle, const QString& qstrMessage, int nWidth, int nHeight, const QString& qstrButton0, const QString& qstrButton1 /*= ""*/, const QString& qstrButton2 /*= ""*/, const QString& qstrButton3 /*= ""*/)
{
    LAST_STEP("HtmlMsg::msg()");

    //QDialog dlg (pParent, Qt::Dialog | getNoResizeWndFlags() | (bStayOnTop ? Qt::WindowStaysOnTopHint : Qt::WindowFlags(0)));
    QDialog dlg (pParent, getDialogWndFlags() | (nFlags & STAY_ON_TOP ? Qt::WindowStaysOnTopHint : Qt::WindowFlags(0)));

    dlg.setWindowTitle(qstrTitle);
    dlg.setWindowIcon(QIcon(":/images/logo.svg"));
    QVBoxLayout* pLayout (new QVBoxLayout(&dlg));

    QTextBrowser* pContent (new QTextBrowser(&dlg));

    if (nFlags & CRITICAL)
    {
        QPalette pal (pContent->palette());
        pal.setColor(QPalette::Base, QColor(192, 0, 0));
        pal.setColor(QPalette::Text, QColor(255, 255, 0));
        pContent->setPalette(pal);

        QFont fnt (pContent->font());
        fnt.setBold(true);
        pContent->setFont(fnt);
    }

    pContent->setOpenExternalLinks(true);
    pContent->setHtml(qstrMessage + (nFlags & SHOW_SYS_INFO ? "<hr/><p style=\"margin-bottom:1px; margin-top:8px; \">" + getSystemInfo() + "</p>" : "")); //ttt1 perhaps use CSS
    pLayout->addWidget(pContent);

    QHBoxLayout btnLayout;
    QCheckBox* pCheck (0);
    if (0 != pbGotTheMessage)
    {
        pCheck = new QCheckBox("I got the message; don't show this again", &dlg);
        btnLayout.addWidget(pCheck);
    }

    HtmlMsg msg (&dlg, nEscape);

    btnLayout.addStretch(0);

    QPushButton* pBtn0 (0); QPushButton* pBtn1 (0); QPushButton* pBtn2 (0); QPushButton* pBtn3 (0);

    pBtn0 = new QPushButton(qstrButton0, &dlg);
    (nFlags & VERT_BUTTONS ? (QLayout&)(*pLayout) : (QLayout&)btnLayout).addWidget(pBtn0);
    QObject::connect(pBtn0, SIGNAL(clicked()), &msg, SLOT(onClick0()));
    if (0 == nDefault) { pBtn0->setDefault(true); }

    if (!qstrButton1.isEmpty())
    {
        pBtn1 = new QPushButton(qstrButton1, &dlg);
        (nFlags & VERT_BUTTONS ? (QLayout&)(*pLayout) : (QLayout&)btnLayout).addWidget(pBtn1);
        QObject::connect(pBtn1, SIGNAL(clicked()), &msg, SLOT(onClick1()));
        if (1 == nDefault) { pBtn1->setDefault(true); }

        if (!qstrButton2.isEmpty())
        {
            pBtn2 = new QPushButton(qstrButton2, &dlg);
            (nFlags & VERT_BUTTONS ? (QLayout&)(*pLayout) : (QLayout&)btnLayout).addWidget(pBtn2);
            QObject::connect(pBtn2, SIGNAL(clicked()), &msg, SLOT(onClick2()));
            if (2 == nDefault) { pBtn2->setDefault(true); }

            if (!qstrButton3.isEmpty())
            {
                pBtn3 = new QPushButton(qstrButton3, &dlg);
                (nFlags & VERT_BUTTONS ? (QLayout&)(*pLayout) : (QLayout&)btnLayout).addWidget(pBtn3);
                QObject::connect(pBtn3, SIGNAL(clicked()), &msg, SLOT(onClick3()));
                if (3 == nDefault) { pBtn3->setDefault(true); }
            }
        }
    }

    pLayout->addLayout(&btnLayout);

    dlg.resize(nWidth, nHeight);
    dlg.setSizeGripEnabled(true);

    dlg.exec();

    LAST_STEP1("HtmlMsg::msg 2()", 1);

    if (0 != pbGotTheMessage)
    {
        *pbGotTheMessage = pCheck->isChecked();
    }

    return msg.m_nBtn;
}

void HtmlMsg::onClick0()
{
    m_nBtn = 0;
    m_pDlg->accept();
}


void HtmlMsg::onClick1()
{
    m_nBtn = 1;
    m_pDlg->accept();
}

void HtmlMsg::onClick2()
{
    m_nBtn = 2;
    m_pDlg->accept();
}

void HtmlMsg::onClick3()
{
    m_nBtn = 3;
    m_pDlg->accept();
}





