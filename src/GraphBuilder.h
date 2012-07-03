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
        class Callees &getFunctions(void) const;
        class Calls &getCalls(void) const;

    private:
        class GraphBuilderImpl *impl;
};
