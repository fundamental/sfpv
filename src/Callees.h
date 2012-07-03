#ifndef CALLEE_H
#define CALLEE_H
#include <string>
#include <map>
#include <set>

namespace clang {
    class Decl;
}

//The class of objects, which can be called
class Callee
{
    public:
        Callee(std::string str, class TranslationUnit *tu=NULL, class clang::Decl *decl=NULL)
            :name(str),TU(tu),DECL(decl),RT(false),ext_RT(false),nRT(false), ext_nRT(false), defined(false)
        {}

        virtual void display(void) const;
        void realtime(void) { RT = true; }
        void ext_realtime(void) {ext_RT = true;}
        void ext_not_realtime(std::string _reason) {ext_nRT = true; reason = "is an function interacting with " + _reason;}
        void not_realtime(void) { nRT = true; reason = "was annotated non-realtime"; }
        bool realtime_p(void) {return RT|ext_RT;}
        bool not_realtime_p(void) {return nRT|ext_nRT;}
        void define(void) {defined = true;}
        bool defined_p(void) {return defined;}

        std::string reason;
        std::string name;
        class TranslationUnit *TU;
        class clang::Decl *DECL;

        virtual std::string getName(void) const;
    private:
        bool RT;
        bool ext_RT;
        bool nRT;
        bool ext_nRT;
        bool defined;
};

class FuncCallee : public Callee
{
    public:
        void display(void) const;
};

class FuncPtrCallee : public Callee
{
    public:
        FuncPtrCallee(std::string name, class TranslationUnit *tu, class clang::Decl *decl)
            :Callee(name,tu,decl)
        {
            define();
        }

        std::string getName(void) const;
        //void display(void) const;
};

class Callees
{
    public:
        Callees(void);
        ~Callees(void);

        void print(void) const;
        bool has(std::string fname);
        void add(std::string fname, class TranslationUnit *tu, class clang::Decl *fdecl);
        void add(FuncPtrCallee *fpc);
        Callee *operator[](std::string fname);

        typedef std::set<Callee*>::iterator iterator;
        iterator begin(void);
        iterator end(void);
    private:
        class CalleesImpl *impl;
};

#endif
