#include <string>

class TranslationUnit
{
    public:
        TranslationUnit(std::string str);
        ~TranslationUnit(void);
        void collect(class GraphBuilder *gb, int argc, const char **argv);
        void *getDiagEng(void);
    private:
        class TranslationUnitImpl *impl;
};
