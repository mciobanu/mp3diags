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


#ifndef TagEditorDlgImplH
#define TagEditorDlgImplH

#include  <set>

#include  <QDialog>
#include  <QItemDelegate>

#include  "ui_TagEditor.h"


class CommonData;
class QSettings;
class QScrollArea;
class TagWriter;
class TagEditorDlgImpl;
class TransfConfig;

namespace TagEditor {


class CurrentAlbumModel : public QAbstractTableModel
{
    Q_OBJECT

    TagEditorDlgImpl* m_pTagEditorDlgImpl;
    TagWriter* m_pTagWriter;
    const CommonData* m_pCommonData;

public:
    //CurrentAlbumModel(CommonData* pCommonData);
    CurrentAlbumModel(TagEditorDlgImpl* pTagEditorDlgImpl);

    /*override*/ int rowCount(const QModelIndex&) const;
    /*override*/ int columnCount(const QModelIndex&) const;
    /*override*/ QVariant data(const QModelIndex&, int) const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;
    /*override*/ Qt::ItemFlags flags(const QModelIndex& index) const;
    /*override*/ bool setData(const QModelIndex& index, const QVariant& value, int nRole /*= Qt::EditRole*/);

    void emitLayoutChanged() { emit layoutChanged(); }
};



class CurrentFileModel : public QAbstractTableModel
{
    Q_OBJECT

    const TagEditorDlgImpl* m_pTagEditorDlgImpl;
    const TagWriter* m_pTagWriter;
    const CommonData* m_pCommonData;

public:
    CurrentFileModel(const TagEditorDlgImpl* pTagEditorDlgImpl);

    /*override*/ int rowCount(const QModelIndex&) const;
    /*override*/ int columnCount(const QModelIndex&) const;
    /*override*/ QVariant data(const QModelIndex&, int) const;

    /*override*/ QVariant headerData(int nSection, Qt::Orientation eOrientation, int nRole = Qt::DisplayRole) const;

    void emitLayoutChanged() { emit layoutChanged(); }
};


#if 0
class CurrentAlbumGDelegate : public QAbstractItemDelegate
//class CurrentAlbumGDelegate : public QItemDelegate
//class CurrentAlbumGDelegate : public MultiLineTvDelegate
{
    Q_OBJECT

public:
    //NotesGDelegate(CommonData* pCommonData, QObject* pParent) : QItemDelegate(pParent), m_pCommonData(pCommonData) {}
    NotesGDelegate(CommonData* pCommonData);

    /*override*/ void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    CommonData* m_pCommonData;
};
#endif



class CurrentFileDelegate : public QItemDelegate
{
    Q_OBJECT
protected:
    QTableView* m_pTableView;
public:
    CurrentFileDelegate(QTableView* pTableView);

    /*override*/ void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    /*override*/ QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};


class CurrentAlbumDelegate : public QItemDelegate
{
    Q_OBJECT
protected:
    QTableView* m_pTableView;
    const TagEditorDlgImpl* m_pTagEditorDlgImpl;
    const TagWriter* m_pTagWriter;
    //const CommonData* m_pCommonData;

    mutable std::set<QObject*> m_spEditors;
public:
    CurrentAlbumDelegate(QTableView* pTableView, TagEditorDlgImpl* pTagEditorDlgImpl);

    /*override*/ void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    // /*override*/ QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

    /*override*/ QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem&, const QModelIndex& index) const;
    bool closeEditor(); // closes the editor opened with F2, saving the data; returns false if there was some error and it couldn't close
private slots:
    void onEditorDestroyed(QObject*);
};

} // namespace TagEditor

class AssgnBtnWrp;

class TagEditorDlgImpl : public QDialog, private Ui::TagEditorDlg
{
    Q_OBJECT

    CommonData* m_pCommonData;

    void resizeTagEditor(); // resizes the widgets in the tag editor area: album table, image list; for file table and album table resizes the columns
    void resizeFile(); // resizes the "current file" grid; called by resizeTagEditor() and by onFileChanged();
    QScrollArea* m_pImgScrollArea;
    TagWriter* m_pTagWriter;
    /*override*/ void resizeEvent(QResizeEvent* pEvent);
    /*override*/ void closeEvent(QCloseEvent* pEvent);

    void loadTagWriterInf();
    void saveTagWriterInf();

    AssgnBtnWrp* m_pAssgnBtnWrp;

    TagEditor::CurrentAlbumModel* m_pCurrentAlbumModel;
    TagEditor::CurrentFileModel* m_pCurrentFileModel;

    TagEditor::CurrentAlbumDelegate* m_pAlbumDel;
    bool m_bSectionMovedLock;

    void clearSelection();
    void resizeIcons();
    void selectMainCrt(); // selects the song that is current in the main window (to be called on the constructor);

    TransfConfig& m_transfConfig;

    enum SaveOpt { SAVED, DISCARDED, CANCELLED, PARTIALLY_SAVED };
    enum { IMPLICIT, EXPLICIT };
    SaveOpt save(bool bExplicitCall); // based on configuration, either just saves the tags or asks the user for confirmation; returns true iff all tags have been saved or if none needed saving; it should be followed by a reload(), either for the current or for the next/prev album; if bExplicitCall is true, the "ASK" option is turned into "SAVE";

    bool closeEditor(/*const std::string& strAction*/) { return m_pAlbumDel->closeEditor(); } // closes the editor opened with F2, saving the data; returns false if there was some error and it couldn't close //ttt1 perhaps use strAction for a more personalize message

    /*override*/ bool eventFilter(QObject* pObj, QEvent* pEvent);
    std::vector<std::pair<int, int> > getSelFields() const; // returns the selceted fields, with the first elem as the song number and the second as the field index in TagReader::Feature (it converts columns to fields using TagReader::FEATURE_ON_POS); first column (file name) is ignored
    void eraseSelFields(); // erases the values in the selected fields

public:
    TagEditorDlgImpl(QWidget* pParent, CommonData* pCommonData, TransfConfig& transfConfig); // transfConfig is needed both to be able to instantiate the config dialog and for saving ID3V2
    ~TagEditorDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    std::string run();

    TagWriter* getTagWriter() const { return m_pTagWriter; }
    const CommonData* getCommonData() const { return m_pCommonData; }

    using Ui::TagEditorDlg::m_pCurrentAlbumG;
    using Ui::TagEditorDlg::m_pCurrentFileG;
    void updateAssigned(); // updates the state of "assigned" fields and the assign button

    static QColor ALBFILE_NORM_COLOR;
    static QColor ALB_NONID3V2_COLOR;
    static QColor ALB_ASSIGNED_COLOR;

    static QColor FILE_TAG_MISSING_COLOR;
    static QColor FILE_NA_COLOR;
    static QColor FILE_NO_DATA_COLOR;

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/

    void on_m_pNextB_clicked();
    void on_m_pPrevB_clicked();

    void on_m_pQueryDiscogsB_clicked();
    void on_m_pQueryMusicBrainzB_clicked();
    void on_m_pEditPatternsB_clicked();
    void on_m_pPaletteB_clicked();

    void on_m_pAssignedB_clicked();
    void on_m_pReloadB_clicked();
    void on_m_pCopyFirstB_clicked();
    void on_m_pSaveB_clicked();
    void on_m_pPasteB_clicked();
    void on_m_bSortB_clicked();
    void on_m_pConfigB_clicked();

    void onAlbSelChanged();
    void onFileSelSectionMoved(int nLogicalIndex, int nOldVisualIndex, int nNewVisualIndex);
    void onShow() { resizeTagEditor(); }

    void onAlbumChanged(/*bool bContentOnly*/); // the param was meant to determine if the selection should be kept (bContentOnly is true) or cleared (false); no longer needed, because the clearing the selection is done separately; //ttt1 perhaps put back, after restructuring the tag editor signals
    void onFileChanged();
    void onImagesChanged(); // adds new ImageInfoPanelWdgImpl instances, connects assign button and calls resizeTagEditor()
};

#endif

