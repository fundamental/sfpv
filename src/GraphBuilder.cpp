#include "GraphBuilder.h"
#include "Callees.h"
#include "Calls.h"
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <err.h>

#pragma clang diagnostic ignored "-Wc++11-extensions"

class GraphStmtHelper :public clang::StmtVisitor<GraphStmtHelper>
{
    GraphBuilderImpl *gbi;
    std::string caller;

    //Container for function local function pointers
    std::set<Callee*> local_ptr;

    public:
    GraphStmtHelper(GraphBuilderImpl *_gbi, std::string _caller)
        :gbi(_gbi), caller(_caller)
    {}

    ~GraphStmtHelper(void);

    void VisitStmt(clang::Stmt *s)
    {
        //s->dump();
        VisitChildren(s);
    }

    void VisitBinAssign(clang::BinaryOperator *eq);
    void VisitDeclStmt(clang::DeclStmt *ds);
    void VisitCallExpr(clang::CallExpr *ce);
    void VisitCXXNewExpr(clang::CXXNewExpr *ne);
    void VisitCXXDeleteExpr(clang::CXXDeleteExpr *de);

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
        Callees callees;
        Calls calls;
        class TranslationUnit *current_tu;
};

GraphStmtHelper::~GraphStmtHelper(void)
{
    for(Callee *c:local_ptr)
        gbi->callees.add(static_cast<FuncPtrCallee*>(c));
}

void GraphStmtHelper::VisitCXXNewExpr(clang::CXXNewExpr *ne)
{
    clang::FunctionDecl *fdecl = ne->getOperatorNew();

    //Alright, if you don't want to give me a fdecl you wont be parsed
    if(!fdecl) //TODO check why this case is possible
        return;

    std::string callee = fdecl->getQualifiedNameAsString();

    //add operator new to the list of known functions
    if(!gbi->callees.has(callee))
        gbi->callees.add(callee, gbi->current_tu, fdecl);

    gbi->calls.add(gbi->callees[caller], gbi->callees[callee], gbi->current_tu, ne);
}

void GraphStmtHelper::VisitCXXDeleteExpr(clang::CXXDeleteExpr *de)
{
    clang::FunctionDecl *fdecl = de->getOperatorDelete();

    //Alright, if you don't want to give me a fdecl you wont be parsed
    if(!fdecl) //TODO check why this case is possible
        return;

    std::string callee = fdecl->getQualifiedNameAsString();

    //add operator new to the list of known functions
    if(!gbi->callees.has(callee))
        gbi->callees.add(callee, gbi->current_tu, fdecl);

    gbi->calls.add(gbi->callees[caller], gbi->callees[callee], gbi->current_tu, de);
}

void GraphStmtHelper::VisitBinAssign(clang::BinaryOperator *eq)
{
    using clang::dyn_cast;
    using clang::DeclRefExpr;
    //Assume basic function pointer assignment
    //eq->getRHS()->dump();
    //eq->getLHS()->dump();
    if(dyn_cast<clang::CastExpr>(eq->getRHS()) &&
            dyn_cast<DeclRefExpr>(eq->getLHS()))
    {
        clang::DeclRefExpr *lhs = dyn_cast<DeclRefExpr>(eq->getLHS()),
                           *rhs = dyn_cast<DeclRefExpr>(
                                   dyn_cast<clang::CastExpr>(
                                       eq->getRHS())->getSubExpr());
        if(lhs && rhs) {
            //Get LHS node
            Callee *caller = NULL;
            for(Callee *c : local_ptr) {
                if(c->name == lhs->getDecl()->getNameAsString()) {
                    caller = c;
                    break;
                }
            }

            if(!caller) {
                warnx("Could not find local function pointer for assignment...");
                return;
            }

            if(gbi->callees.has(rhs->getDecl()->getNameAsString()))
                gbi->calls.add(caller, gbi->callees[rhs->getDecl()->getNameAsString()], gbi->current_tu, eq);
            else {
                //Find the local
                Callee *callee = NULL;
                for(Callee *c : local_ptr) {
                    if(c->name == rhs->getDecl()->getNameAsString()) {
                        callee = c;
                        break;
                    }
                }

                if(!callee) {
                    warnx("Could not find local function pointer for assignment...");
                    return;
                }
                gbi->calls.add(caller, callee, gbi->current_tu, eq);
            }


        }
    }
}


void GraphStmtHelper::VisitDeclStmt(clang::DeclStmt *ds)
{
    using clang::dyn_cast;
    //ds->dump();
    for(auto itr = ds->decl_begin(); itr != ds->decl_end(); ++itr) {
        if(dyn_cast<clang::VarDecl>(*itr)) {
            clang::VarDecl *vdecl = dyn_cast<clang::VarDecl>(*itr);
            //vdecl->dumpXML();
            if(vdecl->getType()->isFunctionPointerType()) {
                FuncPtrCallee *callee =
                    new FuncPtrCallee(vdecl->getNameAsString(), gbi->current_tu, (*itr));
                local_ptr.insert(callee);

                //Add initial state if one is present
                if(vdecl->getInit())
                {
                    if(dyn_cast<clang::CastExpr>(vdecl->getInit())) {
                        clang::DeclRefExpr *expr =
                            dyn_cast<clang::DeclRefExpr>(dyn_cast<clang::CastExpr>(vdecl->getInit())->getSubExpr());
                        if(expr)
                            std::string tmp = expr->getDecl()->getNameAsString();
                    }
                }
            }
        }
    }
    clang::StmtVisitor<GraphStmtHelper>::VisitDeclStmt(ds);
}

void GraphStmtHelper::VisitCallExpr(clang::CallExpr *ce)
{
    //Note templates are tricky and thus support for them will be quite limited
    using clang::dyn_cast;
    clang::FunctionDecl *fdecl = ce->getDirectCallee();
    if(fdecl) {
        std::string callee = ce->getDirectCallee()->getQualifiedNameAsString();
        gbi->calls.add(gbi->callees[caller],gbi->callees[callee],gbi->current_tu,ce);
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
        //ce->dump();
        if(dyn_cast<clang::CastExpr>(ce->getCallee())) {
            clang::DeclRefExpr *expr =
                dyn_cast<clang::DeclRefExpr>(dyn_cast<clang::CastExpr>(ce->getCallee())->getSubExpr());
            if(expr) {
                std::string tmp = expr->getDecl()->getNameAsString();

                //Find the local
                Callee *callee = NULL;
                for(Callee *c : local_ptr) {
                    if(c->name == tmp) {
                        callee = c;
                        break;
                    }
                }

                if(callee)
                    gbi->calls.add(gbi->callees[caller],callee,gbi->current_tu,ce);
                else
                    warnx("could not find callee %s\n", tmp.c_str());
            }
        }
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
        //(*b)->dumpXML();
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

Callees &GraphBuilder::getFunctions(void) const
{
    return impl->callees;
}

Calls &GraphBuilder::getCalls(void) const
{
    return impl->calls;
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
                callees[fname]->realtime();
            if(a_string=="!realtime")
                callees[fname]->not_realtime();
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
            Callee *super = gbi->callees[(*itr)->getQualifiedNameAsString()];
            Callee *sub   = gbi->callees[method->getQualifiedNameAsString()];
            //FIXME
            //This only covers the basic cases of virtual methods and it may
            //give incorrect behavior for nonvirtual methods, further testing
            //required
            gbi->calls.add(super, sub, gbi->current_tu, NULL);
        }
    }

    std::string fname = fdecl->getQualifiedNameAsString();
    gbi->callees.add(fname, gbi->current_tu, fdecl);
    if(fdecl->isThisDeclarationADefinition()) {
        gbi->callees[fname]->define();

        //Find Calls within function
        GraphStmtHelper gsh(gbi, fname);
        gsh.Visit(fdecl->getBody());
    }
    gbi->annotation_check(fname, fdecl);

    return true;
}

