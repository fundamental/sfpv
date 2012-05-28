#include <string>

namespace clang
{
    class DiagnosticsEngine;
}

class TranslationUnit
{
    public:
        TranslationUnit(std::string str);
        ~TranslationUnit(void);
        void collect(class GraphBuilder *gb, char *clang_options);
        clang::DiagnosticsEngine *getDiagEng(void);
    private:
        class TranslationUnitImpl *impl;
};
