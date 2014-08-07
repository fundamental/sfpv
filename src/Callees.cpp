#include "Callees.h"
#include <clang/AST/Decl.h>
#include <cstdio>
#include <map>
#include <err.h>

//C++11 extentions are useful, but setting the standard to be C++11 breaks
//compilation, so lets just sweep these errors under a rug for now
#pragma clang diagnostic ignored "-Wc++11-extensions"

void Callee::display(void) const
{
    printf("%40s\t%d\t%d\t%d\n", name.c_str(), RT|ext_RT, nRT|ext_nRT, defined);
}

std::string Callee::getName(void) const
{
    using clang::FunctionDecl;
    using clang::dyn_cast_or_null;

    //Assuming strange builtin is being viewed
    if(!DECL && (name[0] == '_' || name.substr(0,5) == "std::" || strstr(name.c_str(), "::~")))
        return name;

    FunctionDecl *fdecl = dyn_cast_or_null<FunctionDecl>(DECL);
    if(fdecl)
        return fdecl->getQualifiedNameAsString();
    warnx("could not get function name... (%p) (%s)", DECL, name.c_str());
    if(DECL)
        DECL->dumpColor();
    else
        warnx("A null DECL, that is odd...");
    return name;//"NULL";
}

std::string FuncPtrCallee::getName(void) const
{
    using clang::VarDecl;
    using clang::dyn_cast;
    VarDecl *vdecl = dyn_cast<VarDecl>(DECL);
    if(vdecl)
        return "(" + vdecl->getQualifiedNameAsString() + ")";
    warnx("could not get function pointer name...");
    return "NULL";
}

class CalleesImpl
{
    public:
        std::map<std::string, Callee*> fmap;
        std::set<Callee*> callees;

        ~CalleesImpl(void)
        {
            for(Callee *c:callees)
                delete c;
        }
};

Callees::Callees(void)
    :impl(new CalleesImpl)
{}
Callees::~Callees(void)
{
    delete impl;
}

void Callees::print(void) const
{
    printf("%40s\t%s\t%s\t%s\n", "Fn. Name", "RT", "nRT", "defined");
    for(auto callee : impl->callees)
        callee->display();
}

bool Callees::has(std::string fname)
{
    return impl->fmap.find(fname)!=impl->fmap.end();
}

void Callees::add(std::string fname, class TranslationUnit *tu, class clang::Decl *fdecl)
{
    //warnx("Callee added (%s)", fname.c_str());
    if(impl->fmap.find(fname) == impl->fmap.end()) {
        Callee *ce = new Callee(fname, tu, fdecl);
        impl->fmap.insert(std::pair<std::string, Callee*>(fname, ce));
        impl->callees.insert(ce);
    } else {
        //There must have been a forward reference
        impl->fmap[fname]->TU = tu;
        impl->fmap[fname]->DECL = fdecl;
    }
}

void Callees::add(FuncPtrCallee *fpc)
{
    impl->callees.insert(fpc);
}

Callee *Callees::operator[](std::string fname)
{
    if(!impl->fmap[fname]) {
        //warnx("could not find (%s)", fname.c_str());
        //warnx("Making a sloppy forward reference");
        //warnx("Callee added (%s)...", fname.c_str());
        Callee *ce = new Callee(fname);
        impl->fmap[fname] = ce;
        impl->callees.insert(ce);
    }
    return impl->fmap[fname];
}

typedef std::set<Callee*>::iterator miterator;

miterator Callees::begin(void)
{
    return impl->callees.begin();
}

miterator Callees::end(void)
{
    return impl->callees.end();
}
