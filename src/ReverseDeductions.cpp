#include "ReverseDeductions.h"
#include "Errors.h"
#include "GraphBuilder.h"
#include "FuncCalls.h"
#include "FuncEntries.h"
#include "TranslationUnit.h"

#include <string>
#include <vector>
#include <cstdio>

#pragma clang diagnostic ignored "-Wc++11-extensions"

typedef CallPair Deduction;
class ReverseDeductionsImpl
{
    public:
        int length(void) const;
        void print(void) const;
        void print_whitelist(void) const;
        bool rt_p(std::string fname) const;
        bool nrt_p(std::string fname) const;
        void deduce_rt(std::string d);
        void deduce_nrt(Deduction d);
        void perform_deductions(GraphBuilder *gb);
        void find_all(GraphBuilder *gb);
        bool known(FuncEntry e) const;

        GraphBuilder *gb;
    private:
        std::vector<std::string> rt_deductions;
        std::vector<Deduction> nrt_deductions;
};

ReverseDeductions::ReverseDeductions(GraphBuilder *gb)
    :impl(new ReverseDeductionsImpl)
{
    impl->gb = gb;
    impl->find_all(gb);
}

ReverseDeductions::~ReverseDeductions(void)
{
    delete impl;
}

void ReverseDeductions::print(void)
{
    impl->print();
}

void ReverseDeductions::print_whitelist(void)
{
    impl->print_whitelist();
}

int ReverseDeductionsImpl::length(void) const
{
    return rt_deductions.size() + nrt_deductions.size();
}

void ReverseDeductionsImpl::print(void) const
{
    printf("Deduced Safe Functions\n");
    for(std::string s: rt_deductions)
        printf("%30s\n", s.c_str());

    printf("Deduced Unsafe Functions\n");
    for(Deduction d: nrt_deductions)
        printf("%30s :=> %30s\n", d.first.c_str(), d.second.c_str());
}

void ReverseDeductionsImpl::print_whitelist(void) const
{
    for(std::string s: rt_deductions)
        printf("%s\n", s.c_str());
}

bool ReverseDeductionsImpl::rt_p(std::string fname) const
{
    FuncEntries &fe = gb->getFunctions();
    if(fe.has(fname) && fe[fname].realtime_p())
        return true;

    for(std::string d: rt_deductions)
        if(d == fname)
            return true;
    return false;
}

bool ReverseDeductionsImpl::nrt_p(std::string fname) const
{
    FuncEntries &fe = gb->getFunctions();
    if(fe.has(fname) && fe[fname].not_realtime_p())
        return true;

    for(auto d: nrt_deductions)
        if(d.first == fname)
            return true;
    return false;
}

void ReverseDeductionsImpl::deduce_nrt(Deduction d)
{
    nrt_deductions.push_back(d);
}
void ReverseDeductionsImpl::deduce_rt(std::string d)
{
    rt_deductions.push_back(d);
}

void ReverseDeductionsImpl::perform_deductions(GraphBuilder *gb)
{
    FuncEntries &entries = gb->getFunctions();
    for(auto pair:entries) {
        FuncEntry e = pair.second;

        //Don't bother with functions with deduced safety
        if(known(e) || !e.defined_p())
            continue;

        //Try to deduce non-realtime
        //If any call is to an unsafe function, this function is unsafe
        for(CallPair c:gb->getCalls()) {
            if(c.first == e.name && nrt_p(c.second)) {
                deduce_nrt(c);
                goto next;
            }
        }


        //Try to deduce realtime
        //If all calls are to safe functions, this function is safe
        //FIXME to postpone complexity recursion is hardcoded
        for(CallPair c:gb->getCalls()) {
            if(c.first == e.name && c.first != c.second &&!rt_p(c.second)) {
                goto next;
            }
        }
        deduce_rt(e.name);
next:
        ;
    }
}

void ReverseDeductionsImpl::find_all(GraphBuilder *gb)
{
    int len = 0;
    do {
        len = length();
        perform_deductions(gb);
    } while(len != length());
}

bool ReverseDeductionsImpl::known(FuncEntry e) const
{
    return rt_p(e.name)||nrt_p(e.name);
}

