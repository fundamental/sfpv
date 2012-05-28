namespace clang
{
    class ASTConsumer;
}

class GraphBuilder
{
    public:
        GraphBuilder(void);
        ~GraphBuilder(void);
        class clang::ASTConsumer *getConsumer(class TranslationUnit *tu);
        class FuncEntries &getFunctions(void) const;
        class FuncCalls &getCalls(void) const;

    private:
        class GraphBuilderImpl *impl;
};
