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


#ifndef SimpleSaxHandlerH
#define SimpleSaxHandlerH

#include  <QXmlDefaultHandler>

template<class T>
struct SimpleSaxHandler : public QXmlDefaultHandler
{
    typedef void (T::* Start)(const QXmlAttributes&);
    typedef void (T::* End)();
    typedef void (T::* Char)(const std::string&);

    class Node
    {
        NoDefaults k;
        friend class SimpleSaxHandler<T>;
        int m_nLevel;

        Node(const std::string& strName) : k(0), m_nLevel(0), m_pParent(0), m_strName(strName), onStart(0), onEnd(0), onChar(0)
        {
        }

        Node(Node& parent, const std::string& strName) : k(0), m_nLevel(parent.m_nLevel + 1), m_pParent(&parent), m_strName(strName), onStart(0), onEnd(0), onChar(0)
        {
            parent.add(this);
        }

        std::vector<Node*> m_vpChildren;
        Node* m_pParent;

        std::string m_strName;
        void add(Node* p)
        {
            if (0 != getChild(p->m_strName)) { throw 1; } // ttt1 throw something else
            m_vpChildren.push_back(p);
        }

        Node* getChild(const std::string& strName)
        {
            for (int i = 0, n = cSize(m_vpChildren); i < n; ++i)
            {
                if (strName == m_vpChildren[i]->m_strName)
                {
                    return m_vpChildren[i];
                }
            }
            return 0;
        }

    public:
        Start onStart;
        End onEnd;
        Char onChar;
    };

    Node& getRoot()
    {
        //return *m_lpNodes.front();
        typename std::list<Node*>::iterator it (m_lpNodes.begin());
        ++it;
        return **it;
    }

    Node& makeNode(Node& node, const std::string& strName)
    {
        m_lpNodes.push_back(new Node(node, strName));
        return *m_lpNodes.back();
    }

    SimpleSaxHandler(const std::string& strName)
    {
        m_lpNodes.push_back(new Node("qqq")); // to avoid comparisons to 0
        m_lpNodes.push_back(new Node(*m_lpNodes.back(), strName)); // the "root" node has a level of 1, because the artificial first elem has level 0

        m_pCrtNode = m_lpNodes.front(); m_nCrtLevel = 0; // ttt1 reset these if an error occurs and then the object is used for another parsing;
    }

    /*override*/ ~SimpleSaxHandler()
    {
        clearPtrContainer(m_lpNodes);
    }

private:
    std::list<Node*> m_lpNodes;

    Node* m_pCrtNode;
    int m_nCrtLevel;

    /*override*/ bool startElement(const QString& /*qstrNamespaceUri*/, const QString& qstrLocalName, const QString& /*qstrName*/, const QXmlAttributes& attrs)
    {
        ++m_nCrtLevel;
        if (m_nCrtLevel == m_pCrtNode->m_nLevel + 1)
        {
            std::string s (qstrLocalName.toUtf8());
            Node* pChild (m_pCrtNode->getChild(s));
            if (0 != pChild)
            {
                m_pCrtNode = pChild;
                if (0 != m_pCrtNode->onStart)
                {
                    T* p (dynamic_cast<T*>(this));
                    (p->*(m_pCrtNode->onStart))(attrs);
                }
            }
        }
        return true;
    }

    /*override*/ bool endElement(const QString& /*qstrNamespaceUri*/, const QString& qstrLocalName, const QString& /*qstrName*/)
    {
        --m_nCrtLevel;
        if (m_nCrtLevel == m_pCrtNode->m_nLevel - 1)
        {
            std::string s (qstrLocalName.toUtf8());
            CB_ASSERT (s == m_pCrtNode->m_strName);
            if (0 != m_pCrtNode->onEnd)
            {
                T* p (dynamic_cast<T*>(this));
                (p->*(m_pCrtNode->onEnd))();
            }
            m_pCrtNode = m_pCrtNode->m_pParent;
        }
        return true;
    }

    /*override*/ bool characters(const QString& qstrCh)
    {
        std::string s (qstrCh.toUtf8());
        if (m_nCrtLevel == m_pCrtNode->m_nLevel)
        {
            if (0 != m_pCrtNode->onChar)
            {
                T* p (dynamic_cast<T*>(this));
                (p->*(m_pCrtNode->onChar))(s);
            }
        }
        return true;
    }
};

#endif // #ifndef SimpleSaxHandlerH
