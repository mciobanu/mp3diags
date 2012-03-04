#include <iostream>
#include <fstream>
#include <string>
//#include <cstring>
#include <cctype>

using namespace std;

char caseConv(char c, size_t pos) {
    if (pos % 4 == 2) {
        return toupper(c);
    }
    return tolower(c);
}

void convertTransl(string& s) {
    bool inEsc;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '&') {
            inEsc = true;
        }
        if (inEsc) {
            if (s[i] == ';') {
                inEsc = false;
            }
        } else {
            s[i] = caseConv(s[i], i);
        }
    }
}

int main(int, char** argv) {
    ifstream in (argv[1]);
    ofstream out ("/tmp/MiXeDcAsE");
    char a [10000];

    string SRC1 ("        <source>");
    size_t SRCLEN1 (SRC1.size());
    string SRC2 ("</source>");
    size_t SRCLEN2 (SRC2.size());

    string src;
    bool inEsc;
    size_t pos;

    while (in) {
        in.getline(a, 10000);
        string s (a);
        inEsc = false;
        if (s.find("        <translation ") == 0) {
            convertTransl(src);
            out << "        <translation>" << src << "</translation>" << endl;
            src.clear();
        } else {
            out << a << endl;
        }

        if (src.empty()) {
            pos = s.find(SRC1);
            if (pos == 0) {
                pos = s.find(SRC2);
                if (pos == string::npos) { // multi-line
                    src = s.substr(SRCLEN1, s.size() - SRCLEN1);
                    src += "\n";
                } else { // single-line
                    src = s.substr(SRCLEN1, s.size() - SRCLEN1 - SRCLEN2);
                }
            }
        } else {
            pos = s.find(SRC2);
            if (pos == string::npos) {
                if (s.find("        <comment>") == string::npos) {
                    src += s;
                    src += "\n";
                }
            } else {
                src += s.substr(0, s.size() - SRCLEN2);
            }
        }
    }
}


/*#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>

using namespace std;

char caseConv(char c, size_t pos) {
    if (pos % 4 == 2) {
        return toupper(c);
    }
    return tolower(c);
}


int main(int argc, char** argv) {
    ifstream in (argv[1]);
    ofstream out ("/tmp/MiXeDcAsE");
    char a [10000];

    const char* SRC1 ("        <source>");
    int SRCLEN1 (strlen(SRC1));
    const char* SRC2 ("</source>");
    int SRCLEN2 (strlen(SRC2));

    string src;
    bool inEsc;
    bool inSrc (false);
    while (in) {
        in.getline(a, 10000);
        string s (a);
        inEsc = false;
        if (s.find("        <translation ") == 0) {
            out << "        <translation>" << src << "</translation>" << endl;
        } else {
            out << a << endl;
        }

        if (inSrc) {
            size_t pos (s.find(SRC2));
            if (pos == string::npos) {
            } else {
            }
        } else {
            size_t pos (s.find(SRC));
            if (pos == 0) {
                src = s.substr(SRCLEN1, s.size() - SRCLEN1 - SRCLEN2);
                for (size_t i = 0; i < src.size(); ++i) {
                    if (src[i] == '&') {
                        inEsc = true;
                    }
                    if (inEsc) {
                        if (src[i] == ';') {
                            inEsc = false;
                        }
                    } else {
                        src[i] = caseConv(src[i], i);
                    }
                }
            }
        }
    }
}

*/

/*

int main(int, char** argv) {
    ifstream in (argv[1]);
    ofstream out ("/tmp/MiXeDcAsE");
    char a [10000];

    string SRC1 ("        <source>");
    size_t SRCLEN1 (SRC1.size());
    string SRC2 ("</source>");
    size_t SRCLEN2 (SRC2.size());

    string src;
    bool inEsc;
    size_t pos;

    while (in) {
        in.getline(a, 10000);
        string s (a);
        inEsc = false;
        if (s.find("        <translation ") == 0) {
            convertTransl(src);
            out << "        <translation>" << src << "</translation>" << endl;
            src.clear();
        } else {
            out << a << endl;
        }

        //if (s.find("        <comment ") != string::npos) {
            if (src.empty()) {
                pos = s.find(SRC1);
                if (pos == 0) {
                    pos = s.find(SRC2);
                    if (pos == string::npos) { // multi-line
                        src = s.substr(SRCLEN1, s.size() - SRCLEN1);
                        src += "\n";
                    } else { // single-line
                        src = s.substr(SRCLEN1, s.size() - SRCLEN1 - SRCLEN2);
                    }
                }
            } else {
                pos = s.find(SRC2);
                if (pos == string::npos) {
                    src += s;
                    src += "\n";
                } else {
                    src += s.substr(0, s.size() - SRCLEN2);
                }
            }
        //}
    }
}


*/
