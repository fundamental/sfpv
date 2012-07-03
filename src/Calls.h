#include <utility>
#include <string>
#include <set>
#include <err.h>

#include "Callees.h"

namespace clang
{
    class Expr;
}

enum CallType
{
    DirectCall,
    IndirectCall,
    VirtualCall
};

class Call : public std::pair<Callee*,Callee*>
{
    public:
        Call(Callee *a, Callee *b, class TranslationUnit *tu, class clang::Expr *ce)
            :std::pair<Callee*,Callee*>(a,b),TU(tu), caller(a), callee(b), CE(ce)
        {}
        //The translation unit for which the call is defined
        class TranslationUnit *TU;
        Callee *caller, *callee;
        CallType ct;

        //Expression of call (when appliciable)
        class clang::Expr *CE;
};

class Calls
{
    public:
        Calls(void);
        ~Calls(void);

        void print(void) const;
        void add(Callee *a, Callee *b, class TranslationUnit *tu, class clang::Expr *ce);
        typedef std::set<Call*>::iterator sitr;
        sitr begin(void);
        sitr end(void);
    private:
        struct CallsImpl *impl;
};
