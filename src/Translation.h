#ifndef TranslationH
#define TranslationH


#include  <string>
#include  <vector>

#include  <QTranslator>

/*
class LocaleInfo
{
    std::string m_strCountry;
    std::string m_strLanguage;

public:
    LocaleInfo(std::string strFileName);
    const std::string& getCountry() const { return m_strCountry; }
    const std::string& getLanguage() const { return m_strLanguage; }
};*/

class TranslatorHandler
{
    std::vector<std::string> m_vstrLongTranslations;
    std::vector<std::string> m_vstrTranslations;
    QTranslator m_appTranslator;
    QTranslator m_systemTranslator;
    std::string m_strCurrentTranslation;
    QString m_qstrSystemTranslDir;
    void addTranslations(const std::string& strDir);
    std::string getLocale(std::string strTranslation);
    TranslatorHandler();
public:
    const std::vector<std::string>& getTranslations() { return m_vstrTranslations; }
    void setTranslation(const std::string& strTranslation);
    static TranslatorHandler& getGlobalTranslator();
    static std::string getLanguageInfo(std::string strFileName);
};



#endif // TranslationH
