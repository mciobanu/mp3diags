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


#ifndef Mp3TransformThreadH
#define Mp3TransformThreadH

#include  <deque>
#include  <vector>
#include  <string>
#include  <ostream>

#include  <QStringList>


class Mp3Handler;
class QWidget;
class Transformation;
class CommonData;
class TransfConfig;

typedef QList<QString> StrList;

class Mp3Transformer
{
    CommonData* m_pCommonData;
    const TransfConfig& m_transfConfig;
    //bool m_bAll; // if to use all handlers or only the selected ones
    const std::deque<const Mp3Handler*>& m_vpHndlr;

    std::vector<const Mp3Handler*>& m_vpDel;
    std::vector<const Mp3Handler*>& m_vpAdd; // for proc files that are in the same directory as the source and have a different name

    std::vector<Transformation*>& m_vpTransf;

    std::string m_strErrorFile; // normally this is empty; if it's not, writing to the specified file failed
    std::string m_strErrorDir; // normally this is empty; if it's not, creating the specified backup file failed
    bool m_bWriteError;
    bool m_bFileChanged;
    std::ostream* m_pLog;

    virtual bool isAborted() = 0;
    virtual void checkPause() = 0;
    virtual void emitStepChanged(const StrList& v, int nStep) = 0;

public:
    Mp3Transformer(
        CommonData* pCommonData,
        const TransfConfig& transfConfig,
        const std::deque<const Mp3Handler*>& vpHndlr,
        std::vector<const Mp3Handler*>& vpDel,
        std::vector<const Mp3Handler*>& vpAdd,
        std::vector<Transformation*>& vpTransf,
        std::ostream* pLog) :

        m_pCommonData(pCommonData),
        m_transfConfig(transfConfig),
        m_vpHndlr(vpHndlr),
        m_vpDel(vpDel),
        m_vpAdd(vpAdd),
        m_vpTransf(vpTransf),
        m_bWriteError(true),
        m_bFileChanged(false),
        m_pLog(pLog)
    {
    }

    bool transform(); // returns on the first error

    std::string getError() const;
};



// if there are errors the user is notified;
// returns true iff there were no errors;
bool transform(const std::deque<const Mp3Handler*>& vpHndlr, std::vector<Transformation*>& vpTransf, const std::string& strTitle, QWidget* pParent, CommonData* pCommonData, TransfConfig& transfConfig);


#endif
