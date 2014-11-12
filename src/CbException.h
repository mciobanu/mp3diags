#ifndef CBEXCEPTION_H
#define CBEXCEPTION_H


#include  <string>

void logToGlobalFile(const std::string& s);


class CbException : public std::exception {

    std::string m_strMsg;

public:
    CbException(std::string strMsg, const char* szFile, int nLine);

    CbException(std::string strMsg, const char* szFile, int nLine, const CbException& cause);

    ~CbException() throw() {}

    /*override*/ const char* what() const throw() {
        return m_strMsg.c_str();
    }
};

#define DEFINE_CB_EXCP(CLASS_NAME) \
struct CLASS_NAME : public CbException \
{ \
    CLASS_NAME(const char* szFile, int nLine) : CbException(#CLASS_NAME, szFile, nLine) {} \
    CLASS_NAME(std::string strMsg, const char* szFile, int nLine) : CbException(strMsg, szFile, nLine) {} \
    CLASS_NAME(std::string strMsg, const char* szFile, int nLine, const CbException& cause) : CbException(strMsg, szFile, nLine, cause) {} \
}

#define CB_THROW(CLASS_NAME) throw CLASS_NAME(__FILE__, __LINE__)


//#define CB_CHECK1(COND, EXCP) { if (!(COND)) { ::trace(#EXCP); throw EXCP; } }
#define CB_CHECK(COND, EXCP) { if (!(COND)) { ::trace(#EXCP); throw EXCP(#EXCP, __FILE__, __LINE__); } } //ttt2 see how to force calling code to end with ";", like CB_THROW
#define CB_CHECK_MSG(COND, EXCP, MSG) { if (!(COND)) { std::string msg (std::string(#EXCP) + ": " + MSG); ::trace(msg); throw EXCP(msg, __FILE__, __LINE__); } }
#define CB_CHECK_PARAM(COND, EXCP, PARAM) { if (!(COND)) { std::string msg (std::string(#EXCP) + ": " + PARAM); ::trace(msg); throw EXCP(PARAM, __FILE__, __LINE__); } } //ttt2 not quite right; assumes PARAM is string



//#define CB_TRACE_AND_THROW(MSG) { throw std::runtime_error(MSG); }
//#define CB_TRACE_AND_THROW1(EXCP) { ::trace(#EXCP); throw EXCP; }
#define CB_TRACE_AND_THROW(EXCP) { ::trace(#EXCP); throw EXCP(#EXCP, __FILE__, __LINE__); }
#define CB_TRACE_AND_THROW_MSG(EXCP, MSG) { ::trace(#EXCP); throw EXCP((std::string(#EXCP) + ": " + MSG, __FILE__, __LINE__); }

//#define CB_ASSERT(COND) { if (!(COND)) { ::trace("assert"); throw std::runtime_error("assertion failure"); } }
#define CB_ASSERT(COND) { if (!(COND)) { assertBreakpoint(); ::trace("assert"); logAssert(__FILE__, __LINE__, #COND); ::exit(1); } }
#define CB_ASSERT_MSG(COND, MSG) { if (!(COND)) { assertBreakpoint(); ::trace("assert"); logAssert(__FILE__, __LINE__, #COND, MSG); ::exit(1); } }


#endif // CBEXCEPTION_H
