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

#include  <deque>
#include  <memory>
#include  <string>
#include  <vector>

#include  <QDialog>
#include  <QItemDelegate>
#include  <QApplication> // for translation


#include  "ui_FileRenamer.h"

#include  "Helpers.h"

class CommonData;
class QSettings;
class FileRenamerDlgImpl;
class Mp3Handler;



class Renamer;


namespace FileRenamer {


// current album / filtered handlers;
class HndlrListModel : public QAbstractTableModel
{
    Q_OBJECT

    FileRenamerDlgImpl* m_pFileRenamerDlgImpl;
    const CommonData* m_pCommonData;

    const Renamer* m_pRenamer;

    bool m_bUseCurrentView;

    /*override*/ Qt::ItemFlags flags(const QModelIndex& index) const;
    /*override*/ bool setData(const QModelIndex& index, const QVariant& value, int nRole /* = Qt::EditRole*/);
public:
    HndlrListModel(CommonData* pCommonData, FileRenamerDlgImpl* pFileRenamerDlgImpl, bool bUseCurrentView);
    ~HndlrListModel();
    //HndlrListModel(TagEditorDlgImpl* pTagEditorDlgImpl);

    /*override*/ int rowCount(const QModelIndex&) const;
    /*override*/ int columnCount(const QModelIndex&) const;
    /*override*/ QVariant data(const QModelIndex&, int) const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;
    // /*override*/ Qt::ItemFlags flags(const QModelIndex& index) const;

    const Renamer* getRenamer() const { return m_pRenamer; }
    void setRenamer(const Renamer*);
    void setUnratedAsDuplicates(bool bUnratedAsDuplicate);

    const std::deque<const Mp3Handler*> getHandlerList() const; // returns either m_pCommonData->getCrtAlbum() or m_pCommonData->getViewHandlers(), based on m_bUseCurrentView

    void emitLayoutChanged() { emit layoutChanged(); }
};

class CurrentAlbumDelegate : public QItemDelegate
{
    Q_OBJECT
    const HndlrListModel* m_pHndlrListModel;

public:
    CurrentAlbumDelegate(QWidget* pParent, HndlrListModel* pHndlrListModel);

    /*override*/ void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    // /*override*/ QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};


struct SequencePattern;

class InvalidCharsReplacer;


} // namespace FileRenamer

class ModifInfoToolButton;

class FileRenamerDlgImpl : public QDialog, private Ui::FileRenamerDlg
{
    Q_OBJECT

    CommonData* m_pCommonData;

    FileRenamer::HndlrListModel* m_pHndlrListModel;

    void reloadTable();
    void resizeUi();

    std::vector<std::string> m_vstrPatterns;
    void loadPatterns();
    void savePatterns();
    void updateButtons();
    void createButtons();
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

    bool m_bUseCurrentView;

    //std::vector<QToolButton
    /*override*/ void resizeEvent(QResizeEvent* pEvent);
    /*override*/ bool eventFilter(QObject* pObj, QEvent* pEvent);
    QObject* m_pEditor;
    void closeEditor();

    void resizeIcons();

public:
    enum { DONT_USE_CRT_VIEW, USE_CRT_VIEW };
    FileRenamerDlgImpl(QWidget* pParent, CommonData* pCommonData, bool bUseCurrentView);
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

    void on_m_pMarkUnratedAsDuplicatesCkB_clicked();
    void on_m_pCloseB_clicked();

    void onHelp();
};


class Mp3Handler;

class Renamer
{
    Q_DECLARE_TR_FUNCTIONS(Renamer)
    std::string m_strPattern;
    FileRenamer::SequencePattern* m_pRoot;
    bool m_bSameDir;
    const CommonData* m_pCommonData;
    std::auto_ptr<FileRenamer::InvalidCharsReplacer> m_pInvalidCharsReplacer;
public:
    Renamer(const std::string& strPattern, const CommonData* pCommonData, bool bUnratedAsDuplicate);
    ~Renamer();
    const std::string& getPattern() const { return m_strPattern; }

    //const FileRenamer::SequencePattern* getSeq() const { return m_pRoot; }
    std::string getNewName(const Mp3Handler*) const;

    bool isSameDir() const { return m_bSameDir; }

    struct InvalidPattern
    {
        Q_DECLARE_TR_FUNCTIONS(InvalidPattern)
    public:
        const std::string m_strErr;
        InvalidPattern(const std::string& strPattern, const QString& strErr) : m_strErr(convStr(tr("Pattern \"%1\" is invalid. %2").arg(strPattern.c_str()).arg(strErr))) {}
    };

    mutable bool m_bUnratedAsDuplicate;
    mutable std::map<const Mp3Handler*, std::string> m_mValues; // only has entries for custom-modified values
};

#endif

