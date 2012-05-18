#include <clang/AST/ASTConsumer.h>
#include "Types.h"

class GraphBuilder
{
    public:
        GraphBuilder(void);
        ~GraphBuilder(void);
        clang::ASTConsumer *getConsumer(class TranslationUnit *tu);
        FuncEntries &getFunctions(void) const;
        Fcalls &getCalls(void) const;

    private:
        class GraphBuilderImpl *impl;
};
