#include "FuncEntries.h"
#include <cstdio>
#include <map>

//C++11 extentions are useful, but setting the standard to be C++11 breaks
//compilation, so lets just sweep these errors under a rug for now
#pragma clang diagnostic ignored "-Wc++11-extensions"

void FuncEntry::display(void) const
{
    printf("%40s\t%d\t%d\t%d\n", name.c_str(), RT|ext_RT, nRT, defined);
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
    if(impl->fmap.find(fname) == impl->fmap.end())
        impl->fmap.insert(std::pair<std::string, FuncEntry>(fname,FuncEntry(fname, tu, fdecl)));
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
