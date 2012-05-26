#include "GraphBuilder.h"
#include <clang/AST/StmtVisitor.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <cstdio>
#include <iostream>
#include <err.h>

#pragma clang diagnostic ignored "-Wc++11-extensions"
#define CHECK(x) do{if(!x) errx(1, "expected '%s'", #x);}while(0)

class GraphStmtHelper :public clang::StmtVisitor<GraphStmtHelper>
{
    GraphBuilderImpl *gbi;
    std::string caller;
    
    public:
    GraphStmtHelper(GraphBuilderImpl *_gbi, std::string _caller)
        :gbi(_gbi), caller(_caller)
    {}

    void VisitStmt(clang::Stmt *s)
    {
        VisitChildren(s);
    }

    void VisitCallExpr(clang::CallExpr *ce);

    void VisitChildren(clang::Stmt *S)
    {
        for(auto s : S->children())
            if(s)
                Visit(s);
    }

};

class GraphAstHelper :public clang::RecursiveASTVisitor<GraphAstHelper>
{
    public:
        GraphAstHelper(GraphBuilderImpl *_gbi)
            :gbi(_gbi)
        {}

        bool VisitFunctionDecl(clang::FunctionDecl *fdecl);
    private:
        GraphBuilderImpl *gbi;
};

class TopDeclConsumer : public clang::ASTConsumer
{
    public:
        TopDeclConsumer(GraphBuilderImpl *_gbi)
            :gbi(_gbi)
        {}
        bool HandleTopLevelDecl(clang::DeclGroupRef d);
    private:
        class GraphBuilderImpl *gbi;
};

class GraphBuilderImpl
{
    public:
        GraphBuilderImpl(void)
            :consume(this)
        {}

        void annotation_check(std::string fname, clang::Decl *decl);

        TopDeclConsumer consume;
        FuncEntries fe;
        Fcalls fc;
        class TranslationUnit *current_tu;
};

void GraphStmtHelper::VisitCallExpr(clang::CallExpr *ce)
{
    clang::FunctionDecl *fdecl = ce->getDirectCallee();
    if(fdecl) {
        std::string callee = ce->getDirectCallee()->getQualifiedNameAsString();
        gbi->fc.add(caller,callee,gbi->current_tu,ce);
    }
    else
        warnx("odd, we found a fdecl without a callee, TODO fix this edge case");
}

bool TopDeclConsumer::HandleTopLevelDecl(clang::DeclGroupRef d) {
    typedef clang::DeclGroupRef::iterator iter;
    for (iter b = d.begin(), e = d.end(); b != e; ++b) {
        GraphAstHelper GAH(gbi);
        GAH.TraverseDecl(*b);
    }

    return true; // keep going
}

GraphBuilder::GraphBuilder(void)
{
    impl = new GraphBuilderImpl;
}

GraphBuilder::~GraphBuilder(void)
{
    delete impl;
}

clang::ASTConsumer *GraphBuilder::getConsumer(class TranslationUnit *tu)
{
    impl->current_tu = tu;
    return &impl->consume;
}

FuncEntries &GraphBuilder::getFunctions(void) const
{
    return impl->fe;
}

Fcalls &GraphBuilder::getCalls(void) const
{
    return impl->fc;
}

void GraphBuilderImpl::annotation_check(std::string fname, clang::Decl *decl)
{
    using clang::dyn_cast;
    using clang::AnnotateAttr;
    if(!decl->hasAttrs())
        return;

    for(auto attr : decl->getAttrs()) {
        if(attr->getKind() == clang::attr::Annotate) {
            AnnotateAttr *annote = dyn_cast<AnnotateAttr>(attr);
            const std::string a_string = annote->getAnnotation().str();
            if(a_string=="realtime")
                fe[fname].realtime();
            if(a_string=="!realtime")
                fe[fname].not_realtime();
        }
    }
}

bool GraphAstHelper::VisitFunctionDecl(clang::FunctionDecl *fdecl)
{

    using clang::dyn_cast;
    clang::CXXMethodDecl *method = dyn_cast<clang::CXXMethodDecl>(fdecl);
    if(method && method->size_overridden_methods()) {
        for(auto itr = method->begin_overridden_methods();
                itr != method->end_overridden_methods(); ++itr)
        {
            //FIXME
            //This only covers the basic cases of virtual methods and it may
            //give incorrect behavior for nonvirtual methods, further testing
            //required
            gbi->fc.add((*itr)->getQualifiedNameAsString(),
                    method->getQualifiedNameAsString(), gbi->current_tu, NULL);
        }
    }

    std::string fname = fdecl->getQualifiedNameAsString();
    gbi->fe.add(fname, gbi->current_tu, fdecl);
    if(fdecl->isThisDeclarationADefinition()) {
        gbi->fe[fname].define();

        //Find Calls within function
        GraphStmtHelper gsh(gbi, fname);
        gsh.Visit(fdecl->getBody());
    }
    gbi->annotation_check(fname, fdecl);

    return true;
}

