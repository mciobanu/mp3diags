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


#ifndef WidgetsH
#define WidgetsH

#include  <QMenu>
#include  <QToolButton>
#include  <QMessageBox>


//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================


// A menu that makes available the "modifier" keys that were pressed when an item was selected. Currently only used to determine if SHIFT was pressed.
//
// To achieve the same result without using the keyboard, selecting with the right button will also tag SHIFT as pressed.
//
// (an alternative approch that would usually work is to query the keyboard status immediately after something is selected; however, this approach may lead to incorrect results if significant time passes between selection and query; besides, Qt doesn't seem to offer any means to query the keyboard and posts on the net suggest to use X or Windows-specific means (XQueryKeymap or GetAsyncKeyState), which is neither portable nor particularly obvious
class ModifInfoMenu : public QMenu
{
Q_OBJECT
    /*override*/ void mousePressEvent(QMouseEvent* pEvent);
    Qt::KeyboardModifiers m_modif;
public:
    ModifInfoMenu(QWidget* pParent = 0) : QMenu(pParent) {}
    Qt::KeyboardModifiers getModifiers() const { return m_modif; }
};



// A ToolButton that makes available the "modifier" keys that were pressed when it was clicked. Currently only used to determine if SHIFT or CTRL were pressed.
//
// To achieve the same result without using the keyboard, the following sequence will also tag SHIFT as pressed: left button down > right button down > left button up (it doesn't matter when the right button is released).
//
// On the constructor it replaces an existing QToolButton with itself, copying some of the properties, such as icon or size.
//
// ttt2 perhaps turn this class into something generic, by duplicating all the properties and handling other layouts
class ModifInfoToolButton : public QToolButton
{
Q_OBJECT
    /*override*/ void mousePressEvent(QMouseEvent* pEvent);
    /*override*/ void keyPressEvent(QKeyEvent* pEvent);
    Qt::KeyboardModifiers m_modif;
    void contextMenuEvent(QContextMenuEvent* pEvent);
public:
    ModifInfoToolButton(QToolButton* pOldBtn);
    Qt::KeyboardModifiers getModifiers() const { return m_modif; }
};




int showMessage(QWidget* pParent, QMessageBox::Icon icon, int nDefault, int nEscape, const QString& qstrTitle, const QString& qstrMessage, const QString& qstrButton1, const QString& qstrButton2 = "", const QString& qstrButton3 = "", const QString& qstrButton4 = "");


// switches the cursor to hourglass on the constructor and back to normal on the destructor
struct CursorOverrider
{
    CursorOverrider(Qt::CursorShape crs = Qt::BusyCursor);
    ~CursorOverrider();
};


#endif // #ifndef WidgetsH

