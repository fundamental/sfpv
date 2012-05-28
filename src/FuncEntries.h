#ifndef TYPES_H
#define TYPES_H
#include <string>
#include <map>
#include <set>

namespace clang {
    class FunctionDecl;
}

class FuncEntry
{
    public:
        FuncEntry(void)
            :name("I_HAVE_A_BUG...@" __FILE__)
        {
        }
        FuncEntry(std::string str, class TranslationUnit *tu, class clang::FunctionDecl *fdecl)
            :name(str),TU(tu),FDECL(fdecl),RT(false),ext_RT(false),nRT(false), defined(false)
        {}

        void display(void) const;
        void realtime(void) { RT = true; }
        void ext_realtime(void) {ext_RT = true;}
        void not_realtime(void) { nRT = true; }
        bool realtime_p(void) {return RT|ext_RT;}
        bool not_realtime_p(void) {return nRT;}
        void define(void) {defined = true;}
        bool defined_p(void) {return defined;}

        std::string name;
        class TranslationUnit *TU;
        class clang::FunctionDecl *FDECL;
    private:
        bool RT;
        bool ext_RT;
        bool nRT;
        bool defined;
};

class FuncEntries
{
    public:
        FuncEntries(void);
        ~FuncEntries(void);

        void print(void) const;
        bool has(std::string fname);
        void add(std::string fname, class TranslationUnit *tu, class clang::FunctionDecl *fdecl);
        FuncEntry &operator[](std::string fname);

        typedef std::map<std::string, FuncEntry>::iterator iterator;
        iterator begin(void);
        iterator end(void);
    private:
        class FuncEntriesImpl *impl;
};

#endif
