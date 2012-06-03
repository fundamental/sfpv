#include <utility>
#include <string>
#include <set>

namespace clang
{
    class Expr;
}

class CallPair : public std::pair<std::string,std::string>
{
    public:
        CallPair(void)
            :std::pair<std::string,std::string>("",""),TU(NULL), CE(NULL)
        {}

        CallPair(std::string a, std::string b, class TranslationUnit *tu, class clang::Expr *ce)
            :std::pair<std::string,std::string>(a,b),TU(tu), CE(ce)
        {}
        class TranslationUnit *TU;
        class clang::Expr *CE;
};

class FuncCalls
{
    public:
        FuncCalls(void);
        ~FuncCalls(void);

        void print(void) const;
        void add(std::string f1, std::string f2, class TranslationUnit *tu, class clang::Expr *ce);
        typedef std::set<CallPair>::iterator sitr;
        sitr begin(void);
        sitr end(void);
    private:
        struct FuncCallsImpl *impl;
};
