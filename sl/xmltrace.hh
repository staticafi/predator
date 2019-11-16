
// TODO: guard, comments

void XMLTraceBegin(const char *file, int line, std::string msg) ;

void XMLTraceNode(const char *file, int line, std::string msg) ;

void XMLTraceEnd() ;

#define XML_ERROR_MSG(a,b) do { \
    /*std::cout << "##E## " << (a)->file << ':' << (a)->line << "\n";*/ \
    std::string str;                                    \
    std::stringstream s(str);                                      \
    s << b;                                              \
    XMLTraceBegin((a)->file,(a)->line, str);      \
}while(0)

#define XML_NOTE_MSG(a,b) do { \
    /*std::cout << "##N## " << (a)->file << ':' << (a)->line << "\n";*/ \
    std::string str;                                    \
    std::stringstream s(str);                                      \
    s << b;                                              \
    XMLTraceNode((a)->file,(a)->line, str);      \
}while(0)

#define XML_ERROR_MEMLEAK_MSG(b) do { \
    std::string str;                                    \
    std::stringstream s(str);                                      \
    s << b;                                              \
    XMLTraceBegin(NULL,-1, str);      \
}while(0)
