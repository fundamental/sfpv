#include "Deductions.h"
#include "Errors.h"
#include "GraphBuilder.h"
#include "FuncCalls.h"
#include "FuncEntries.h"
#include "TranslationUnit.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <string>
#include <vector>
#include <cstdio>

#pragma clang diagnostic ignored "-Wc++11-extensions"

typedef CallPair Deduction;
class DeductionsImpl
{
    public:
        void print_path(std::string func, GraphBuilder *gb);
        int length(void) const;
        void print(void) const;
        bool rt(std::string fname) const;
        void deduce(Deduction d);
        void perform_deductions(GraphBuilder *gb);
        void find_all(GraphBuilder *gb);
        int find_inconsistent(GraphBuilder *gb);
        std::vector<Deduction> d_list;
};

Deductions::Deductions(GraphBuilder *gb)
    :impl(new DeductionsImpl)
{
    impl->find_all(gb);
}

Deductions::~Deductions(void)
{
    delete impl;
}

void Deductions::print(void)
{
    impl->print();
}

int Deductions::find_inconsistent(GraphBuilder *gb)
{
    return impl->find_inconsistent(gb);
}

void DeductionsImpl::print_path(std::string func, GraphBuilder *gb)
{
    for(Deduction d: d_list) {
        if(d.second == func) {
            clang::DiagnosticsEngine *diag = d.TU->getDiagEng();
            if(d.CE) {
                std::string str = d.CE->getDirectCallee()->getQualifiedNameAsString();
                diag->Report(d.CE->getLocStart(), error_realtime_saftey_trace) << str;
            } else
                diag->Report(error_realtime_safety_class) << d.second << d.first;

            print_path(d.first, gb);
            return;
        }
    }

    FuncEntry f = gb->getFunctions()[func];
    clang::DiagnosticsEngine *diag = f.TU->getDiagEng();
    diag->Report(f.FDECL->getLocation(), error_realtime_saftey_trace_end) << f.FDECL->getQualifiedNameAsString();
}

int DeductionsImpl::length(void) const
{
    return d_list.size();
}

void DeductionsImpl::print(void) const
{
    for(Deduction d: d_list)
        printf("%30s :=> %30s\n", d.first.c_str(), d.second.c_str());
}

bool DeductionsImpl::rt(std::string fname) const
{
    for(Deduction d: d_list)
        if(d.second == fname)
            return true;
    return false;
}

void DeductionsImpl::deduce(Deduction d)
{
    d_list.push_back(d);
}

void DeductionsImpl::perform_deductions(GraphBuilder *gb)
{
    FuncEntries &entries = gb->getFunctions();
    for(auto pair:entries) {
        FuncEntry e = pair.second;
        if(!e.realtime_p() && !rt(e.name))
            continue;

        //Deduce all children must be realtime
        for(CallPair c:gb->getCalls()) {
            if(c.first == e.name && !entries[c.second].realtime_p()
                    && !rt(c.second))
                deduce(c);
        }
    }
}

void DeductionsImpl::find_all(GraphBuilder *gb)
{
    int len = 0;
    do {
        len = length();
        perform_deductions(gb);
    } while(len != length());
}

int DeductionsImpl::find_inconsistent(GraphBuilder *gb)
{
    int result = 0; //aka all is safe
    for(auto pair: gb->getFunctions()) {
        FuncEntry e = pair.second;
        if(e.not_realtime_p() && rt(e.name)) {
            clang::DiagnosticsEngine *diag =  e.TU->getDiagEng();
            diag->Report(e.FDECL->getLocation(), error_realtime_saftey_violation)
                << e.FDECL->getQualifiedNameAsString();

            print_path(e.name, gb);
            result |= 2;
        }
        if(!e.defined_p() && !e.realtime_p() && rt(e.name)) {
            //Check for function calls [virtual methods]
            for(auto call : gb->getCalls())
                if(call.first==e.name)
                    goto next;

            clang::DiagnosticsEngine *diag = e.TU->getDiagEng();
            diag->Report(e.FDECL->getLocation(), warnn_realtime_saftey_unknown)
                << e.FDECL->getQualifiedNameAsString();

            print_path(e.name, gb);
            result |= 1;
        }
next:
        ;
    }
    return result;
}

