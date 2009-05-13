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


#ifndef SessionsDlgImplH
#define SessionsDlgImplH

#include  <QDialog>

#include  "ui_Sessions.h"


class QSettings;
////#include  <QSettings>
class CheckedDirModel;

class SessionsModel : public QAbstractTableModel
{
    Q_OBJECT

    //TagEditorDlgImpl* m_pTagEditorDlgImpl;
    //TagWriter* m_pTagWriter;
    //const CommonData* m_pCommonData;
    std::vector<std::string>& m_vstrSessions;

public:
    SessionsModel(std::vector<std::string>& vstrSessions);

    /*override*/ int rowCount(const QModelIndex&) const;
    /*override*/ int columnCount(const QModelIndex&) const { return 1; }
    /*override*/ QVariant data(const QModelIndex&, int nRole = Qt::DisplayRole) const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;

    void emitLayoutChanged() { emit layoutChanged(); }
};




class SessionsDlgImpl : public QDialog, private Ui::SessionsDlg
{
    Q_OBJECT

    std::vector<std::string> m_vstrSessions;
    //QSettings& m_settings;
    SessionsModel m_sessionsModel;
    //QSettings* m_pSettings;
    CheckedDirModel* m_pCheckedDirModel;
    std::string getCrtSession() const; // returns empty str if there's no session
    std::string getCrtSessionDir() const;
    //bool m_bOpenLast;
    void removeCrtSession();
    void selectSession(const std::string& strLast);
    void addSession(const std::string&);
public:
    SessionsDlgImpl(QWidget* pParent /*, QSettings& settings*/);
    ~SessionsDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    std::string run();

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/
    void onCrtSessChanged();

    void on_m_pNewB_clicked();
    void on_m_pEditB_clicked();
    void on_m_pEraseB_clicked();
    void on_m_pSaveAsB_clicked();
    void on_m_pHideB_clicked();
    void on_m_pLoadB_clicked();
    void on_m_pOpenB_clicked();
    void on_m_pCancelB_clicked();
    //void on_ _clicked();
    void onShow();

    void onSessDoubleClicked(const QModelIndex& index);
};


#endif

