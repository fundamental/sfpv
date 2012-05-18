#include "Types.h"
#include <cstdio>
#include <map>
#include <set>

void FuncEntry::display(void) const
{
    printf("%40s\t%d\t%d\t%d\n", name.c_str(), RT, nRT, defined);
}

class FuncEntriesImpl
{
    public:
        std::map<std::string, FuncEntry> fmap;
};

FuncEntries::FuncEntries(void)
    :impl(new FuncEntriesImpl)
{}
FuncEntries::~FuncEntries(void)
{
    delete impl;
}

void FuncEntries::print(void) const
{
    printf("%40s\t%s\t%s\t%s\n", "Fn. Name", "RT", "nRT", "defined");
    for(auto pair : impl->fmap)
        pair.second.display();
}

bool FuncEntries::has(std::string fname)
{
    return impl->fmap.find(fname)!=impl->fmap.end();
}

void FuncEntries::add(std::string fname, class TranslationUnit *tu, class clang::FunctionDecl *fdecl)
{
    impl->fmap[fname] = FuncEntry(fname, tu, fdecl);
}

FuncEntry &FuncEntries::operator[](std::string fname)
{
    return impl->fmap[fname];
}

typedef std::map<std::string, FuncEntry>::iterator miterator;

miterator FuncEntries::begin(void)
{
    return impl->fmap.begin();
}

miterator FuncEntries::end(void)
{
    return impl->fmap.end();
}

struct FcallsImpl
{
    std::set<CallPair> cpair;
};

Fcalls::Fcalls(void)
{
    impl = new FcallsImpl;
}

Fcalls::~Fcalls(void)
{
    delete impl;
}

void Fcalls::add(std::string f1, std::string f2, class TranslationUnit *tu, class clang::CallExpr *ce)
{
    impl->cpair.insert(CallPair(f1,f2,tu,ce));
}

void Fcalls::print(void) const
{
    for(CallPair c:impl->cpair)
        printf("%30s :=> %30s\n", c.first.c_str(), c.second.c_str());
}

typedef std::set<CallPair>::iterator sitr;

sitr Fcalls::begin(void)
{
    return impl->cpair.begin();
}

sitr Fcalls::end(void)
{
    return impl->cpair.end();
}

