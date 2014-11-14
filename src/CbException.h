#ifndef CBEXCEPTION_H
#define CBEXCEPTION_H


#include  <string>

#ifndef Q_MOC_RUN  // See: https://bugreports.qt-project.org/browse/QTBUG-22829
#include  <boost/lexical_cast.hpp>
#endif


void logToGlobalFile(const std::string& s);


class CbException : public std::exception {

    std::string m_strMsg;

public:
    CbException(std::string strMsg, const char* szFile, int nLine);

    CbException(std::string strMsg, const char* szFile, int nLine, const CbException& cause);

    /*override*/ ~CbException() throw() {}

    /*override*/ const char* what() const throw() {
        return m_strMsg.c_str();
    }
};

#define DEFINE_CB_EXCP(CLASS_NAME) \
struct CLASS_NAME : public CbException \
{ \
    CLASS_NAME(const char* szFile, int nLine) : CbException(#CLASS_NAME, szFile, nLine) {} \
    CLASS_NAME(const std::string& strMsg, const char* szFile, int nLine) : CbException(#CLASS_NAME + (": " + strMsg), szFile, nLine) {} \
    CLASS_NAME(const std::string& strMsg, const char* szFile, int nLine, const CbException& cause) : CbException(#CLASS_NAME + (": " + strMsg), szFile, nLine, cause) {} \
}

#define DEFINE_CB_EXCP1(CLASS_NAME, TYPE, FIELD) \
struct CLASS_NAME : public CbException \
{ \
    TYPE FIELD; \
    CLASS_NAME(const TYPE& FIELD, const char* szFile, int nLine) : CbException(std::string(#CLASS_NAME) + ": " + boost::lexical_cast<std::string>(FIELD), szFile, nLine), FIELD(FIELD) {} \
    /*override*/ ~CLASS_NAME() throw() {} \
}

#define DEFINE_CB_EXCP2(CLASS_NAME, TYPE1, FIELD1, TYPE2, FIELD2) \
struct CLASS_NAME : public CbException \
{ \
    TYPE1 FIELD1; \
    TYPE2 FIELD2; \
    CLASS_NAME(const TYPE1& FIELD1, const TYPE2& FIELD2, const char* szFile, int nLine) : CbException(std::string(#CLASS_NAME) + ": " + boost::lexical_cast<std::string>(FIELD1) + ", " + boost::lexical_cast<std::string>(FIELD2), szFile, nLine), FIELD1(FIELD1), FIELD2(FIELD2) {} \
    /*override*/ ~CLASS_NAME() throw() {} \
}



#define CB_THROW(CLASS_NAME) throw CLASS_NAME(__FILE__, __LINE__)
#define CB_THROW1(CLASS_NAME, PARAM) throw CLASS_NAME(PARAM, __FILE__, __LINE__)
#define CB_THROW2(CLASS_NAME, PARAM1, PARAM2) throw CLASS_NAME(PARAM1, PARAM2, __FILE__, __LINE__)


//#define CB_CHECK1(COND, EXCP) { if (!(COND)) { ::trace(#EXCP); throw EXCP; } }
#define CB_CHECK(COND, EXCP) { if (!(COND)) { ::trace(#EXCP); throw EXCP(#EXCP, __FILE__, __LINE__); } } //ttt2 see how to force calling code to end with ";", like CB_THROW
#define CB_CHECK_MSG(COND, EXCP, MSG) { if (!(COND)) { ::trace(std::string(#EXCP) + ": " + MSG); throw EXCP(MSG, __FILE__, __LINE__); } }
#define CB_CHECK1(COND, EXCP, PARAM) { if (!(COND)) { ::trace(std::string(#EXCP) + ": " + boost::lexical_cast<std::string>(PARAM)); throw EXCP(PARAM, __FILE__, __LINE__); } }
#define CB_CHECK2(COND, EXCP, PARAM1, PARAM2) { if (!(COND)) { ::trace(#EXCP); throw EXCP(PARAM1, PARAM2, __FILE__, __LINE__); } }

#define CB_EXCP(EXCP) EXCP(__FILE__, __LINE__)
#define CB_EXCP1(EXCP, PARAM) EXCP(PARAM, __FILE__, __LINE__)
#define CB_EXCP2(EXCP, PARAM1, PARAM2) EXCP(PARAM1, PARAM2, __FILE__, __LINE__)


//#define CB_TRACE_AND_THROW(MSG) { throw std::runtime_error(MSG); }
//#define CB_TRACE_AND_THROW1(EXCP) { ::trace(#EXCP); throw EXCP; }
#define CB_TRACE_AND_THROW(EXCP) { ::trace(#EXCP); throw EXCP(#EXCP, __FILE__, __LINE__); }
#define CB_TRACE_AND_THROW1(EXCP, MSG) { ::trace(#EXCP); throw EXCP(std::string(#EXCP) + ": " + MSG, __FILE__, __LINE__); }

//#define CB_ASSERT(COND) { if (!(COND)) { ::trace("assert"); CB_THROW1(CbRuntimeError, "assertion failure"); } }
#define CB_ASSERT(COND) { if (!(COND)) { assertBreakpoint(); ::trace("assert"); logAssert(__FILE__, __LINE__, #COND); ::exit(1); } }
#define CB_ASSERT1(COND, MSG) { if (!(COND)) { assertBreakpoint(); ::trace("assert"); logAssert(__FILE__, __LINE__, #COND, MSG); ::exit(1); } }

DEFINE_CB_EXCP(CbRuntimeError);
DEFINE_CB_EXCP(CbInvalidArgument);


#endif // CBEXCEPTION_H
