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
#include  <sstream>

#include  <QFileDialog>
#include  <QMessageBox>
#include  <QHeaderView>

//#include  "SerSupport.h"

#include  "DebugDlgImpl.h"

#include  "CommonData.h"
#include  "DataStream.h"
#include  "Helpers.h"
#include  "StoredSettings.h"
#include  "LogModel.h"
//#include  "Serializable.h"

/*#include  <boost/archive/binary_oarchive.hpp>
#include  <boost/archive/binary_iarchive.hpp>
#include  <boost/serialization/vector.hpp>
*/

using namespace std;
using namespace pearl;



DebugDlgImpl::DebugDlgImpl(QWidget* pParent, CommonData* pCommonData) : QDialog(pParent, getDialogWndFlags()), Ui::DebugDlg(), m_pCommonData(pCommonData)
{
    setupUi(this);

    int nWidth, nHeight;
    m_pCommonData->m_settings.loadDebugSettings(nWidth, nHeight);
    if (nWidth > 400 && nHeight > 200)
    {
        resize(nWidth, nHeight);
    }
    else
    {
        defaultResize(*this);
    }

    {
        m_pEnableTracingCkB->setChecked(m_pCommonData->m_bTraceEnabled);
        m_pUseAllNotesCkB->setChecked(m_pCommonData->m_bUseAllNotes);
        m_pLogTransfCkB->setChecked(m_pCommonData->m_bLogTransf);
        m_pSaveDownloadedDataCkB->setChecked(m_pCommonData->m_bSaveDownloadedData);
    }

    {
        m_pLogModel = new LogModel(m_pCommonData, m_pLogG);
        m_pLogG->setModel(m_pLogModel);

        m_pLogG->verticalHeader()->setMinimumSectionSize(CELL_HEIGHT);
        m_pLogG->verticalHeader()->setDefaultSectionSize(CELL_HEIGHT);

        m_pLogG->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
        m_pLogG->verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);

        //m_pLogG->verticalHeader()->setResizeMode(QHeaderView::Fixed);
        m_pLogG->verticalHeader()->setResizeMode(QHeaderView::Interactive);
    }

    m_pUseAllNotesCkB->setToolTip(
            "If this is checked, ignored notes and trace notes\n"
            "are shown in the note list and exported, regardless\n"
            "of the \"Ignored\" settings.\n\n"
            "Note that if this is not checked, the trace notes\n"
            "are discarded during file scanning, so checking it\n"
            "later won't bring them back. A new scan is needed\n"
            "to see them.");

    m_pLogG->setFocus();

    { QAction* p (new QAction(this)); p->setShortcut(QKeySequence("F1")); connect(p, SIGNAL(triggered()), this, SLOT(onHelp())); addAction(p); }
}


void DebugDlgImpl::run()
{
    //if (QDialog::Accepted != exec()) { return false; }
    exec();

    m_pCommonData->m_settings.saveDebugSettings(width(), height());
    m_pCommonData->m_bTraceEnabled = m_pEnableTracingCkB->isChecked();
    m_pCommonData->m_bUseAllNotes = m_pUseAllNotesCkB->isChecked();
    m_pCommonData->m_bLogTransf = m_pLogTransfCkB->isChecked();
    m_pCommonData->m_bSaveDownloadedData = m_pSaveDownloadedDataCkB->isChecked();
    //return true;
}


DebugDlgImpl::~DebugDlgImpl()
{
}


void DebugDlgImpl::on_m_pCloseB_clicked()
{
    reject(); // !!! doesn't matter if it's accept()
}

void DebugDlgImpl::exportLog(const string& strFileName)
{
    const deque<std::string>& v (m_pCommonData->getLog());

    ofstream_utf8 out (strFileName.c_str());
    for (int i = 0, n = cSize(v); i < n; ++i)
    {
        out << v[i] << endl;
    }
}



void DebugDlgImpl::on_m_pSaveLogB_clicked()
{
    QFileDialog dlg (this, "Choose destination file", "", "Text files (*.txt)");

    dlg.setFileMode(QFileDialog::AnyFile);
    if (QDialog::Accepted != dlg.exec()) { return; }

    QStringList fileNames (dlg.selectedFiles());
    if (1 != fileNames.size()) { return; } 

    QString s (fileNames.first());
    exportLog(convStr(s));
}


void DebugDlgImpl::on_m_pDecodeMpegFrameB_clicked()
{
    istringstream s (m_pFrameHdrE->text().toUtf8().constData());
    s >> hex;
    unsigned int n;
    s >> n;
    QMessageBox::information(this, "Decoded MPEG frame header", convStr(decodeMpegFrame(n, "\n")));
}




void DebugDlgImpl::onHelp()
{
    openHelp("310_advanced.html");
}


//========================================================================================================================================================
//========================================================================================================================================================
//========================================================================================================================================================


void tstFont();
void tstGenre();
void tstSer01();

void DebugDlgImpl::on_m_pTst01B_clicked()
{
    //tstSer01();
//m_pLogModel->selectTopLeft();

    /*TestThread01* p (new TestThread01());

    ThreadRunnerDlgImpl dlg (p, true, this);
    dlg.exec();*/

    /*bool b;
    QFont f (QFontDialog::getFont(&b, this));
    //cout << "font: " << QFontInfo(myOption.font).family().toStdString() << endl;
    //cout << "font from dlg: " << f.toString().toStdString() << endl;
    printFontInfo("font from dlg", f);

    for (int i = 6; i < 18; ++i)
    {
        QFont f ("B&H LucidaTypewriter", i);

        QFontInfo info (f);
        cout << i << ": " << info.family().toStdString() << ", exactMatch:" << info.exactMatch() << ", fixedPitch:" << info.fixedPitch() << ", italic:" << info.italic() <<
            ", pixelSize:" << info.pixelSize() << ", pointSize" << info.pointSize() << endl;
    }
*/
    //tstFont();

/*
    { const char* a ("1998-10-04"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998-10"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998-1-4"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998-1"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("98"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998-10-04T11:12:13"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998-10-04 11:12:13"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998-10-04T11:12"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998-10-04T11"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
    { const char* a ("1998-10-04T11:6"); QDateTime dt (QDateTime::fromString(a, Qt::ISODate)); if (!dt.isValid()) cout << a << " is invalid\n"; else cout << dt.toString(Qt::ISODate).toStdString() << endl; }
*/

    //m_transfConfig.testRemoveSuffix();

    //tstGenre();






    /*m_pFilesModel->notifyModelChanged();

    char a [100];
    sprintf(a, "%d %d", m_pFilesModel->rowCount(QModelIndex()), m_pFilesModel->columnCount(QModelIndex()));
    m_pStreamsM->append(a);*/

    //m_pContentM->setTextFormat(LogText);

/*
    QItemSelectionModel* pSelModel (m_pCommonData->m_pFilesG->selectionModel());
m_pCommonData->printFilesCrt();
    pSelModel->clear();
m_pCommonData->printFilesCrt();

if (m_pCommonData->getFilesGCrtRow() > 400000) exit(2);
return;
*/

    //resizeEvent(0);

    //m_pContentM->setReadOnly(TRUE);
#if 0
    static bool b1 (false);
    b1 = !b1;
    if (b1)
    {
        char a [100];
        for (int i = 0; i < 10000; ++ i)
        {
            sprintf(a, "%d                 //Mp3Handler* pMp3Handler (new Mp3Handler(fs.getName())); Mp3Handler", i);
            m_pContentM->append(a);
        }

        /*char a [100];
        string s;
        for (int i = 0; i < 1000; ++ i)
        {
            sprintf(a, "%d                 //Mp3Handler* pMp3Handler (new Mp3Handler(fs.getName())); Mp3Handler\n", i);
            s += a;
        }
        m_pContentM->append(s.c_str());*/
    }
    else
    {
        m_pContentM->setText("");
    }
#endif
}


//========================================================================================================================================================
//========================================================================================================================================================
//========================================================================================================================================================

#if 0

#include <iostream>

struct Base
{
    ciobi_ser::SerHelper<Base> m_serHelper;
    Base(ciobi_ser::Univ* pUniv, int n1) : m_serHelper(*this, pUniv), m_n1(n1) {}
    //template <class T> Base(T& obj, ciobi_ser::Univ* pUniv, int n1) : m_serHelper(obj, pUniv), m_n1(n1) {}
    Base(ciobi_ser::ForSerOnly*) : m_serHelper(*this, 0) {}
    virtual ~Base()
    {
    }

    int m_n1;

    virtual void print(ostream& out) const
    {
        out << "Base: " << m_n1 << endl;
    }

    virtual void save(const ciobi_ser::Univ& univ)
    {
        //univ.saveCPE(m_vpMasterDirs);
        univ.save(m_n1);
        cout << "saved base: " << m_n1 << endl;
    }

    virtual void load(ciobi_ser::Univ& univ)
    {
        //univ.loadCPE(m_vpMasterDirs);
        univ.load(m_n1);
        cout << "loaded base: " << m_n1 << endl;
    }

    virtual const char* getClassName() const { return ciobi_ser::ClassName<Base>::szClassName; }
};


struct Der : public Base
{
    //ciobi_ser::SerHelper<Der> m_serHelper;
    //Der(ciobi_ser::Univ* pUniv, int n1, int n2) : Base(pUniv, n1), m_serHelper(*this, pUniv), m_n2(n2) {}
    //Der(ciobi_ser::Univ* pUniv, int n1, int n2) : Base(*this, pUniv, n1), m_n2(n2) {}
    Der(ciobi_ser::Univ* pUniv, int n1, int n2) : Base(pUniv, n1), m_n2(n2) {}
    Der(ciobi_ser::ForSerOnly* p) : Base(p) {}

    int m_n2;

    /*override*/ void print(ostream& out) const
    {
        out << "Der: " << m_n1 << ", " << m_n2 << endl;
    }

    /*override*/ void save(const ciobi_ser::Univ& univ)
    {
        Base::save(univ);
        univ.save(m_n2);
        cout << "saved der: " << m_n1 << ", " << m_n2 << endl;
    }

    /*override*/ void load(ciobi_ser::Univ& univ)
    {
        Base::load(univ);
        univ.load(m_n2);
        cout << "loaded der: " << m_n1 << ", " << m_n2 << endl;
    }

    /*override*/ const char* getClassName() const { return ciobi_ser::ClassName<Der>::szClassName; }
};


template<> /*static*/ const char* ciobi_ser::ClassName<Base>::szClassName ("Base");
template<> /*static*/ const char* ciobi_ser::ClassName<Der>::szClassName ("Der");


// explicit instantiation
template class ciobi_ser::SerHelper<Der>;


ostream& operator<<(ostream& out, const Base& o)
{
    o.print(out);
    return out;
}



//=================================================================================================================

struct Container
{
    ciobi_ser::SerHelper<Container> m_serHelper;
    Container(ciobi_ser::Univ* pUniv, int n) : m_serHelper(*this, pUniv)
    {
        int k (1);
        for (int i = 0; i < n; ++i, k *= 2)
        {
            Base* p (i % 2 ? new Base(pUniv, k) : new Der(pUniv, k, k + 1));
            //Base* p (new Base(pUniv, k));
            m_v.push_back(p);
        }
    }

    Container(ciobi_ser::ForSerOnly*) : m_serHelper(*this, 0) {}
    virtual ~Container()
    {
    }

    vector<Base*> m_v;

    void save(const ciobi_ser::Univ& univ)
    {
        //univ.saveCPE(m_vpMasterDirs);
        univ.saveCP(m_v);
        cout << "saved container: " << endl;
    }

    void load(ciobi_ser::Univ& univ)
    {
        //univ.loadCPE(m_vpMasterDirs);
        univ.loadCP(m_v);
        cout << "loaded container, size " << cSize(m_v) << endl;
        for (int i = 0; i < cSize(m_v); ++i)
        {
            cout << *m_v[i];
        }
    }

    const char* getClassName() const { return ciobi_ser::ClassName<Container>::szClassName; }
};

template<> /*static*/ const char* ciobi_ser::ClassName<Container>::szClassName ("Container");


void tstVec01(bool bSave)
{
    ciobi_ser::Univ univ;
    univ.setDestroyOption(ciobi_ser::Univ::DO_NOTHING); // DELETE_SORTED_OBJECTS
    const char* szFile ("savePolymVec.tser");

    if (bSave)
    {
        Container c (&univ, 4);

        ciobi_ser::SaveOptions so(ciobi_ser::SaveOptions::TEXT, 1, 1, ciobi_ser::SaveOptions::USE_OBJ_INFO, ciobi_ser::SaveOptions::DONT_USE_SEP, ciobi_ser::SaveOptions::USE_COMMENTS);
        univ.saveToFile(&c, szFile, "tst polym univ name", so);
        return;
    }

    Container* p;
    univ.loadFromFile(p, szFile);
}

//=================================================================================================================


void tstBase01(bool bSave)
{
    ciobi_ser::Univ univ;
    univ.setDestroyOption(ciobi_ser::Univ::DO_NOTHING); // DELETE_SORTED_OBJECTS
    const char* szFile ("saveBase01.tser");

    if (bSave)
    {
        Base b1 (&univ, 3);

        ciobi_ser::SaveOptions so(ciobi_ser::SaveOptions::TEXT, 1, 1/*, ciobi_ser::SaveOptions::DONT_USE_OBJ_INFO, ciobi_ser::SaveOptions::DONT_USE_SEP, ciobi_ser::SaveOptions::DONT_USE_COMMENTS*/);
//cout << 1 << endl;
        univ.saveToFile(&b1, szFile, "tst base 01 univ name", so);
//cout << 2 << endl;
        return;
    }

    Base* pb1;
    univ.loadFromFile(pb1, szFile);
}


void tstDer01(bool bSave)
{
    ciobi_ser::Univ univ;
    univ.setDestroyOption(ciobi_ser::Univ::DO_NOTHING); // DELETE_SORTED_OBJECTS
    const char* szFile ("saveDer01.tser");

    if (bSave)
    {
        Der d (&univ, 4, 6);

        ciobi_ser::SaveOptions so(ciobi_ser::SaveOptions::TEXT, 1, 1);
        univ.saveToFile(&d, szFile, "tst der 01 univ name", so);
        return;
    }

    Der* p;
    univ.loadFromFile(p, szFile);
}


//=================================================================================================================
//=================================================================================================================


struct Emb
{
    //ciobi_ser::SerHelper<Base> m_serHelper;
    Emb(int n1) : /*m_serHelper(*this, pUniv),*/ m_n1(n1) {}
    Emb(ciobi_ser::ForSerOnly*) /*m_serHelper(*this, 0)*/ {}

    int m_n1;

    void save(const ciobi_ser::Univ& univ)
    {
        univ.save(m_n1);
    }

    void load(ciobi_ser::Univ& univ)
    {
        univ.load(m_n1);
    }

    void print(ostream& out) const
    {
        out << "Emb: " << m_n1 << endl;
    }
};

ostream& operator<<(ostream& out, const Emb& emb)
{
    emb.print(out);
    return out;
}


struct Encl
{
    ciobi_ser::SerHelper<Encl> m_serHelper;
    Encl(ciobi_ser::Univ* pUniv, int n) : m_serHelper(*this, pUniv)
    {
        int k (1);
        for (int i = 0; i <= n; ++i)
        {
            Emb* p (new Emb(k));
            m_v.push_back(p);
            k *= 2;
        }
    }

    Encl(ciobi_ser::ForSerOnly*) : m_serHelper(*this, 0) {}

    vector<Emb*> m_v;

    void save(const ciobi_ser::Univ& univ)
    {
        univ.saveCPE(m_v);
        cout << "saved encl\n";
    }

    void load(ciobi_ser::Univ& univ)
    {
        univ.loadCPE(m_v);
        cout << "loaded encl:";
        int n (cSize(m_v));
        for (int i = 0; i < n; ++i)
        {
            cout << *m_v[i];
        }
        cout << endl;
    }

    const char* getClassName() const { return ciobi_ser::ClassName<Encl>::szClassName; }
};

template<> /*static*/ const char* ciobi_ser::ClassName<Encl>::szClassName ("Encl");



void tstEmbVec01(bool bSave)
{
    ciobi_ser::Univ univ;
    univ.setDestroyOption(ciobi_ser::Univ::DO_NOTHING); // DELETE_SORTED_OBJECTS
    const char* szFile ("saveEncl01.tser");

    if (bSave)
    {
        Encl e (&univ, 8);

        ciobi_ser::SaveOptions so(ciobi_ser::SaveOptions::TEXT, 1, 1);
        univ.saveToFile(&e, szFile, "tst encl univ name", so);
        return;
    }

    Encl* p;
    univ.loadFromFile(p, szFile);
}


#endif

//========================================================================================================================================================
//========================================================================================================================================================
//========================================================================================================================================================

#if 0

#include <iostream>

struct BoostTstBase
{
    BoostTstBase(int n1) : m_n1(n1) {}
    virtual ~BoostTstBase()
    {
    }

    int m_n1;

    virtual void print(ostream& out) const
    {
        out << "BoostTstBase: " << m_n1 << endl;
    }

protected:
    BoostTstBase() {}
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & m_n1;
    }
};


struct BoostTstDer : public BoostTstBase
{
    BoostTstDer(int n1, int n2) : BoostTstBase(n1), m_n2(n2) {}

    int m_n2;

    /*override*/ void print(ostream& out) const
    {
        out << "BoostTstDer: " << m_n1 << ", " << m_n2 << endl;
    }

private:
    friend class boost::serialization::access;
    BoostTstDer() {}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & boost::serialization::base_object<BoostTstBase>(*this);
        ar & m_n2;
    }
};




ostream& operator<<(ostream& out, const BoostTstBase& o)
{
    o.print(out);
    return out;
}



//=================================================================================================================

struct BoostContainer
{
    BoostContainer(int n)
    {
        int k (1);
        for (int i = 0; i < n; ++i, k *= 2)
        {
            BoostTstBase* p (i % 2 ? new BoostTstBase(k) : new BoostTstDer(k, k + 1));
            m_v.push_back(p);
        }
    }

    virtual ~BoostContainer()
    {
    }

    vector<BoostTstBase*> m_v;

    /*void save()
    {
        //univ.saveCPE(m_vpMasterDirs);
        univ.saveCP(m_v);
        cout << "saved container: " << endl;
    }

    void load()
    {
        //univ.loadCPE(m_vpMasterDirs);
        univ.loadCP(m_v);
    }*/
private:
    friend class boost::serialization::access;
    BoostContainer() {}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*nVersion*/)
    {
        ar & m_v;
    }
};



void tstBoostVec01(bool bSave)
{
    const char* szFile ("savePolymVec.boost.ser");

    if (bSave)
    {
        BoostContainer* p (new BoostContainer(4));

        ofstream_utf8 out (szFile);
        //boost::archive::text_oarchive oar (out);
        boost::archive::binary_oarchive oar (out);

        oar.register_type<BoostTstDer>();
        oar.register_type<BoostTstBase>();
        oar.register_type<BoostContainer>();

        //BoostContainer* q (p);
        //BoostContainer* const q (p);
        //const BoostContainer* const q2 (p);

        //oar << const_cast<const BoostContainer const*>(p);
        //oar << q;
        oar << (BoostContainer* const)p;
        //oar << q;
        return;
    }

    BoostContainer* p;
    ifstream_utf8 in (szFile);
    //boost::archive::text_iarchive iar (in);
    boost::archive::binary_iarchive iar (in);
    iar.register_type<BoostTstDer>();
    iar.register_type<BoostTstBase>();
    iar.register_type<BoostContainer>();

    iar >> p;

    cout << "loaded container, size " << cSize(p->m_v) << endl;
    for (int i = 0; i < cSize(p->m_v); ++i)
    {
        cout << *p->m_v[i];
    }
}

//=================================================================================================================
#if 0

void tstBase01(bool bSave)
{
    const char* szFile ("saveBase01.tser");

    if (bSave)
    {
        BoostTstBase b1 (&univ, 3);

        univ.saveToFile(&b1, szFile, "tst base 01 univ name", so);
//cout << 2 << endl;
        return;
    }

    BoostTstBase* pb1;
    univ.loadFromFile(pb1, szFile);
}


void tstDer01(bool bSave)
{
    const char* szFile ("saveDer01.tser");

    if (bSave)
    {
        BoostTstDer d (4, 6);

        univ.saveToFile(&d, szFile, "tst der 01 univ name", so);
        return;
    }

    BoostTstDer* p;
    univ.loadFromFile(p, szFile);
}

//=================================================================================================================




//=================================================================================================================
//=================================================================================================================


struct Emb
{
    Emb(int n1) : /*m_serHelper(*this, pUniv),*/ m_n1(n1) {}

    int m_n1;

    void save()
    {
        univ.save(m_n1);
    }

    void load()
    {
        univ.load(m_n1);
    }

    void print(ostream& out) const
    {
        out << "Emb: " << m_n1 << endl;
    }
};

ostream& operator<<(ostream& out, const Emb& emb)
{
    emb.print(out);
    return out;
}


struct Encl
{
    Encl(int n)
    {
        int k (1);
        for (int i = 0; i <= n; ++i)
        {
            Emb* p (new Emb(k));
            m_v.push_back(p);
            k *= 2;
        }
    }

    vector<Emb*> m_v;

    void save()
    {
        univ.saveCPE(m_v);
        cout << "saved encl\n";
    }

    void load()
    {
        univ.loadCPE(m_v);
        cout << "loaded encl:";
        int n (cSize(m_v));
        for (int i = 0; i < n; ++i)
        {
            cout << *m_v[i];
        }
        cout << endl;
    }
};



void tstEmbVec01(bool bSave)
{
    const char* szFile ("saveEncl01.tser");

    if (bSave)
    {
        Encl e (&univ, 8);

        univ.saveToFile(&e, szFile, "tst encl univ name", so);
        return;
    }

    Encl* p;
    univ.loadFromFile(p, szFile);
}
#endif
#endif

//=================================================================================================================

#if 0
#include "fstream_unicode.h"

// include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

/////////////////////////////////////////////////////////////
// gps coordinate
//
// illustrates serialization for a simple type
//
class gps_position
{
private:
    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & degrees;
        ar & minutes;
        ar & seconds;
    }
    int degrees;
    int minutes;
    float seconds;
public:
    gps_position(){};
    gps_position(int d, int m, float s) :
        degrees(d), minutes(m), seconds(s)
    {}
};

int tutorial() {
    // create and open a character archive for output
    ofstream_utf8 ofs("filename");

    // create class instance
    const gps_position g(35, 59, 24.567f);

    // save data to archive
    {
        boost::archive::text_oarchive oa(ofs);
        // write class instance to archive
        oa << g;
        // archive and stream closed when destructors are called
    }

    // ... some time later restore the class instance to its orginal state
    gps_position newg;
    {
        // create and open an archive for input
        ifstream_utf8 ifs("filename", std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        // read class state from archive
        ia >> newg;
        // archive and stream closed when destructors are called
    }
    return 0;
}
#endif

#if 0
void tstSer01()
{
    //tstBase01(1 == argc);
    //tstDer01(1 == argc);
    //tstVec01(true);
    //tstVec01(false);
    //tstEmbVec01(1 == argc);

    tstBoostVec01(true);
    tstBoostVec01(false);

    //tutorial();
}
#endif

/*struct TstSer
{
    TstSer()
    {
        //tstBase01(1 == argc);
        //tstDer01(1 == argc);
        tstVec01(true);
        //tstVec01(false);
        //tstEmbVec01(1 == argc);

        ::exit(0);
    }
};

TstSer tstSer01;
*/




// -lboost_serialization-mt-1_37

//ttt2 if "use all notes" is checked, it keeps rescanning at startup

