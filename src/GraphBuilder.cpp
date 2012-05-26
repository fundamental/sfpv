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
    //Note templates are tricky and thus support for them will be quite limited
    using clang::dyn_cast;
    clang::FunctionDecl *fdecl = ce->getDirectCallee();
    if(fdecl) {
        std::string callee = ce->getDirectCallee()->getQualifiedNameAsString();
        gbi->fc.add(caller,callee,gbi->current_tu,ce);
    }
    else if(dyn_cast<clang::UnresolvedLookupExpr>(ce->getCallee())) {
        //Template magic [found in cmath type lookup]
    }
    else if(dyn_cast<clang::CXXDependentScopeMemberExpr>(ce->getCallee())) {
        //More template magic [found in stl iterators]
    }
    else if(dyn_cast<clang::DependentScopeDeclRefExpr>(ce->getCallee())) {
        //Yet more templates [found in stl iter swap template]
    }
    else if(dyn_cast<clang::DeclRefExpr>(ce->getCallee())) {
        //Templates for everyone [this seems to handle possibly overloaded ops]
    }
    else if(dyn_cast<clang::CastExpr>(ce->getCallee())) {
        //Unwanted cast expressions
    }
    else if(dyn_cast<clang::CXXPseudoDestructorExpr>(ce->getCallee())) {
        //TODO Destructors should be checked properly
    }
    else if(dyn_cast<clang::CXXUnresolvedConstructExpr>(ce->getCallee())) {
        //TODO Constructors should be handled properly
    }
    else if(dyn_cast<clang::MemberExpr>(ce->getCallee())) {
        //Not too sure about this one
    }
    else if(dyn_cast<clang::UnresolvedMemberExpr>(ce->getCallee())) {
        //Not too sure about this one
    }
    else if(dyn_cast<clang::ParenExpr>(ce->getCallee())) {
        //This appears to deal with function pointers, this should be checked
    }
    else if(dyn_cast<clang::CallExpr>(ce->getCallee())) {
        //More Function pointer code
    }
    else {
        warnx("Odd, a call expression with unknown semantics was found");
    }
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

