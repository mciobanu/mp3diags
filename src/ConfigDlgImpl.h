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


#ifndef ConfigDlgImplH
#define ConfigDlgImplH

#include  <QDialog>

#include  "ui_Config.h"

#include  "NoteFilterDlgImpl.h" // for NoteListElem and NoteListPainter //ttt2 perhaps move them in their own file
#include  "Transformation.h"



//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------



class TransfConfig;
class CustomTransfListPainter;

class VisibleTransfPainter;

class QSettings;
class QStackedLayout;
class CommonData;

void initDefaultCustomTransf(int k, std::vector<std::vector<int> >& vv, CommonData* pCommonData);

void initDefaultVisibleTransf(std::vector<int>& v, CommonData* pCommonData);

class ConfigDlgImpl : public QDialog, private Ui::ConfigDlg, public NoteListPainterBase // ttt2 NoteListPainterBase is used for the ignored notes, while for custom transforms there is a separate CustomTransfListPainter; this is confusing //ttt2 perhaps create IgnoredNotesPainter, but it's not straightforward, because of the use of protected members in NoteListPainterBase
{
    Q_OBJECT

    TransfConfig& m_transfCfg;

    CommonData* m_pCommonData;
    bool m_bFull;

    void logState(const char* szPlace) const;

    /*override*/ std::string getTooltip(TooltipKey eTooltipKey) const;
    /*override*/ void reset();

    CustomTransfListPainter* m_pCustomTransfListPainter;
    DoubleList* m_pCustomTransfDoubleList;

    std::vector<std::vector<int> > m_vvnCustomTransf;
    void selectCustomTransf(int k); // 0 <= k <= CUSTOM_TRANSF_CNT
    int m_nCurrentTransf;
    void getTransfData();
    std::vector<QToolButton*> m_vpTransfButtons;
    std::vector<QTextEdit*> m_vpTransfLabels;
    void refreshTransfText(int k); // 0 <= k <= CUSTOM_TRANSF_CNT
    QPalette m_defaultPalette;
    QPalette m_wndPalette;
    std::vector<std::vector<int> > m_vvnDefaultCustomTransf;

    VisibleTransfPainter* m_pVisibleTransfPainter;
    DoubleList* m_pVisibleTransfDoubleList;

    std::vector<int> m_vnVisibleTransf;
    std::vector<int> m_vnDefaultVisibleTransf;

    void selectDir(QLineEdit*);
    QByteArray m_codepageTestText;

    QFont m_generalFont;
    QFont m_fixedFont;
    void setFontLabels();

    std::vector<QToolButton*> m_vpColButtons;
    std::vector<QColor> m_vNoteCategColors;

    void setBtnColor(int n);
    void onButtonClicked(int n);

    QStackedLayout* m_pFileSettingsLayout;
    TransfConfig::Options getOpt(); // has the correct m_bKeepOrigTime
    TransfConfig::Options getSimpleViewOpt(); // doesn't set m_bKeepOrigTime
    TransfConfig::Options getFullViewOpt(); // doesn't set m_bKeepOrigTime
    void setSimpleViewOpt(const TransfConfig::Options& opt); // m_bKeepOrigTime shouldn't be set
    void setFullViewOpt(const TransfConfig::Options& opt); // m_bKeepOrigTime is ignored

public:
    enum { SOME_TABS, ALL_TABS };
    ConfigDlgImpl(TransfConfig& transfCfg, CommonData* pCommonData, QWidget* pParent, bool bFull); // bFull determines if all the tabs should be visible
    ~ConfigDlgImpl();
    /*$PUBLIC_FUNCTIONS$*/

    bool run();

public slots:
    /*$PUBLIC_SLOTS$*/
    void on_m_pOkB_clicked();
    void on_m_pCancelB_clicked();

    void on_m_pCustomTransform1B_clicked() { selectCustomTransf(0); }
    void on_m_pCustomTransform2B_clicked() { selectCustomTransf(1); }
    void on_m_pCustomTransform3B_clicked() { selectCustomTransf(2); }
    void on_m_pCustomTransform4B_clicked() { selectCustomTransf(3); } // CUSTOM_TRANSF_CNT

    void onTransfDataChanged();

    void on_m_pSelectSrcDirB_clicked() { selectDir(m_pSrcDirE); }
    void on_m_pSelectTempDirB_clicked() { selectDir(m_pTempDestE); }
    void on_m_pSelectCompDirB_clicked() { selectDir(m_pCompDestE); }
    void on_m_pSelectOrigTransfDestDirB_clicked() { selectDir(m_pPODestE); }
    void on_m_pSelectOrigTransfDestDir2B_clicked() { selectDir(m_pPODest2E); }
    void on_m_pSelectOrigNotTransfDestDirB_clicked() { selectDir(m_pUODestE); }
    void on_m_pSelectTransfDestDirB_clicked() { selectDir(m_pProcDestE); }

    void on_m_pChangeGenFontB_clicked();
    void on_m_pChangeFixedFontB_clicked();

    void on_m_pLocaleCbB_currentIndexChanged(int);

    void on_m_pCol0B_clicked() { onButtonClicked(0); }
    void on_m_pCol1B_clicked() { onButtonClicked(1); }
    void on_m_pCol2B_clicked() { onButtonClicked(2); }
    void on_m_pCol3B_clicked() { onButtonClicked(3); }
    void on_m_pCol4B_clicked() { onButtonClicked(4); }
    void on_m_pCol5B_clicked() { onButtonClicked(5); }
    void on_m_pCol6B_clicked() { onButtonClicked(6); }
    void on_m_pCol7B_clicked() { onButtonClicked(7); }
    void on_m_pCol8B_clicked() { onButtonClicked(8); }
    void on_m_pCol9B_clicked() { onButtonClicked(9); }
    void on_m_pCol10B_clicked() { onButtonClicked(10); }
    void on_m_pCol11B_clicked() { onButtonClicked(11); }
    void on_m_pCol12B_clicked() { onButtonClicked(12); }
    void on_m_pCol13B_clicked() { onButtonClicked(13); }

    void on_m_pResetColorsB_clicked();

    void on_m_pFastSaveCkB_stateChanged();

    void on_m_pSimpleViewB_clicked();
    void on_m_pFullViewB_clicked();

    void onHelp();
};



#endif

