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


#ifndef FileRenamerDlgImplH
#define FileRenamerDlgImplH

#include  <QDialog>
#include  <QItemDelegate>

#include  "ui_FileRenamer.h"


class CommonData;
class QSettings;
class FileRenamerDlgImpl;

/*
namespace RenamerPatterns
{
}*/


class Renamer;


namespace FileRenamer {


// current album
class CurrentAlbumModel : public QAbstractTableModel
{
    Q_OBJECT

    FileRenamerDlgImpl* m_pFileRenamerDlgImpl;
    const CommonData* m_pCommonData;

    const Renamer* m_pRenamer;

public:
    CurrentAlbumModel(CommonData* pCommonData, FileRenamerDlgImpl* pFileRenamerDlgImpl);
    ~CurrentAlbumModel();
    //CurrentAlbumModel(TagEditorDlgImpl* pTagEditorDlgImpl);

    /*override*/ int rowCount(const QModelIndex&) const;
    /*override*/ int columnCount(const QModelIndex&) const;
    /*override*/ QVariant data(const QModelIndex&, int) const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;
    // /*override*/ Qt::ItemFlags flags(const QModelIndex& index) const;

    const Renamer* getRenamer() const { return m_pRenamer; }
    void setRenamer(const Renamer*);

    void emitLayoutChanged() { emit layoutChanged(); }
};

class CurrentAlbumDelegate : public QItemDelegate
{
    Q_OBJECT
    const CommonData* m_pCommonData;

public:
    CurrentAlbumDelegate(QWidget* pParent, CommonData* pCommonData);

    /*override*/ void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    // /*override*/ QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};


struct SequencePattern;


} // namespace FileRenamer

class ModifInfoToolButton;

class FileRenamerDlgImpl : public QDialog, private Ui::FileRenamerDlg
{
    Q_OBJECT

    CommonData* m_pCommonData;

    FileRenamer::CurrentAlbumModel* m_pCurrentAlbumModel;

    void reloadTable();
    void resizeUi();

    std::vector<std::string> m_vstrPatterns;
    void loadPatterns();
    void savePatterns();
    void updateButtons();
    void createButtons(QWidget* pWidget);
    void selectPattern(); // selects the appropriate pattern for a new album, based on whether it's VA or SA; sets m_nCrtPattern and checks the right button; assumes m_eState is properly set up;

    int m_nVaButton, m_nSaButton;
    int m_nBtnId;
    int m_nCrtPattern;
    enum State { SINGLE, SINGLE_ERR, VARIOUS, VARIOUS_ERR, ERR }; // ERR is for when all songs lack ID3V2
    State m_eState;
    bool isSingleArtist() const { return SINGLE == m_eState || SINGLE_ERR == m_eState; }
    //bool m_bSingleArtist; // if there are a
    QButtonGroup* m_pButtonGroup;

    ModifInfoToolButton* m_pModifRenameB;

    //std::vector<QToolButton
    /*override*/ void resizeEvent(QResizeEvent* pEvent);

    void resizeIcons();

public:
    FileRenamerDlgImpl(QWidget* pParent, CommonData* pCommonData);
    ~FileRenamerDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    std::string run();

    using Ui::FileRenamerDlg::m_pCurrentAlbumG;

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pNextB_clicked();
    void on_m_pPrevB_clicked();

    void on_m_pRenameB_clicked();

    void on_m_pEditPatternsB_clicked();

    void onShow() { reloadTable(); }

    void onPatternClicked();

    //void onPatternChanged();
    //void onAlbumChanged();
};


class Mp3Handler;

class Renamer
{
    std::string m_strPattern;
    FileRenamer::SequencePattern* m_pRoot;
public:
    Renamer(const std::string& strPattern);
    ~Renamer();
    const std::string& getPattern() const { return m_strPattern; }

    //const FileRenamer::SequencePattern* getSeq() const { return m_pRoot; }
    std::string getNewName(const Mp3Handler*) const;

    struct InvalidPattern
    {
        const std::string m_strErr;
        InvalidPattern(const std::string& strErr) : m_strErr(strErr) {}
    };
};

#endif

