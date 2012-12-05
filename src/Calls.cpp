#include "Calls.h"
#include <cstdio>

#pragma clang diagnostic ignored "-Wc++11-extensions"

struct CallsImpl
{
    ~CallsImpl(void);
    std::set<Call*> cpair;
};

CallsImpl::~CallsImpl(void)
{
    for(auto a:cpair)
        delete a;
}

Calls::Calls(void)
{
    impl = new CallsImpl;
}

Calls::~Calls(void)
{
    delete impl;
}

void Calls::add(Callee *a, Callee *b, class TranslationUnit *tu, class clang::Expr *ce)
{
    impl->cpair.insert(new Call(a,b,tu,ce));
}

void Calls::print(void) const
{
    for(Call *c:impl->cpair)
        printf("%30s :=> %30s\n", c->first->getName().c_str(), c->second->getName().c_str());
}

typedef std::set<Call*>::iterator sitr;

sitr Calls::begin(void)
{
    return impl->cpair.begin();
}

sitr Calls::end(void)
{
    return impl->cpair.end();
}
