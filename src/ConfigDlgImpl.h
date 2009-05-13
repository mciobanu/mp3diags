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



//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------



class TransfConfig;
class TransfListPainter;

class QSettings;
class CommonData;

void initDefaultCustomTransf(int k, std::vector<std::vector<int> >& vv, CommonData* pCommonData);


class ConfigDlgImpl : public QDialog, private Ui::ConfigDlg, public NoteListPainter
{
    Q_OBJECT

    TransfConfig& m_transfCfg;

    CommonData* m_pCommonData;

    void logState(const char* szPlace) const;

    /*override*/ std::string getTooltip(TooltipKey eTooltipKey) const;
    /*override*/ void reset();

    TransfListPainter* m_pTransfListPainter;
    DoubleList* m_pTransfDoubleList;
    std::vector<std::vector<int> > m_vvCustomTransf;
    void selectCustomTransf(int k); // 0 <= k <= CUSTOM_TRANSF_CNT
    int m_nCurrentTransf;
    void getTransfData();
    std::vector<QToolButton*> m_vpTransfButtons;
    std::vector<QTextEdit*> m_vpTransfLabels;
    void refreshTransfText(int k); // 0 <= k <= CUSTOM_TRANSF_CNT
    QPalette m_defaultPalette;
    QPalette m_wndPalette;
    std::vector<std::vector<int> > m_vvDefaultTransf;

    void selectDir(QLineEdit*);
    QByteArray m_codepageTestText;

    QFont m_generalFont;
    QFont m_fixedFont;
    void setFontLabels();

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
    void on_m_pSelectOrigNotTransfDestDirB_clicked() { selectDir(m_pUODestE); }
    void on_m_pSelectTransfDestDirB_clicked() { selectDir(m_pProcDestE); }

    void on_m_pChangeGenFontB_clicked();
    void on_m_pChangeFixedFontB_clicked();

    void on_m_pLocaleCbB_currentIndexChanged(int);
};

#endif

