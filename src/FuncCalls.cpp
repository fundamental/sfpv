#include "FuncCalls.h"
#include <cstdio>

#pragma clang diagnostic ignored "-Wc++11-extensions"

struct FuncCallsImpl
{
    std::set<CallPair> cpair;
};

FuncCalls::FuncCalls(void)
{
    impl = new FuncCallsImpl;
}

FuncCalls::~FuncCalls(void)
{
    delete impl;
}

void FuncCalls::add(std::string f1, std::string f2, class TranslationUnit *tu, class clang::CallExpr *ce)
{
    impl->cpair.insert(CallPair(f1,f2,tu,ce));
}

void FuncCalls::print(void) const
{
    for(CallPair c:impl->cpair)
        printf("%30s :=> %30s\n", c.first.c_str(), c.second.c_str());
}

typedef std::set<CallPair>::iterator sitr;

sitr FuncCalls::begin(void)
{
    return impl->cpair.begin();
}

sitr FuncCalls::end(void)
{
    return impl->cpair.end();
}
