#include  "CbException.h"

#include  <vector>
#include  <cstdio>
#include  <cstring>

using namespace std;


CbException::CbException(std::string strMsg, const char* szFile, int nLine) {
    vector<char> a (strMsg.size() + strlen(szFile) + 20);
    sprintf(&a[0], "%s [%s/%d]", strMsg.c_str(), szFile, nLine);
    m_strMsg = &a[0];
}

CbException::CbException(std::string strMsg, const char* szFile, int nLine, const CbException& cause) {
    vector<char> a (strMsg.size() + strlen(szFile) + cause.m_strMsg.length() + 40);
    sprintf(&a[0], "%s [%s/%d] / Caused by: %s", cause.m_strMsg.c_str(), szFile, nLine, cause.what());
    m_strMsg = &a[0];
}


