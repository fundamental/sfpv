#include <string>

class TranslationUnit
{
    public:
        TranslationUnit(std::string str);
        ~TranslationUnit(void);
        void collect(class GraphBuilder *gb, const char *clang_options);
        void *getDiagEng(void);
    private:
        class TranslationUnitImpl *impl;
};
