#include "Deductions.h"
#include "Errors.h"
#include "GraphBuilder.h"
#include "Calls.h"
#include "Callees.h"
#include "TranslationUnit.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <string>
#include <vector>
#include <cstdio>
#include <err.h>

#pragma clang diagnostic ignored "-Wc++11-extensions"

typedef Call Deduction;
class DeductionsImpl
{
    public:
        void print_path(Callee *callee, GraphBuilder *gb);
        int length(void) const;
        void print(void) const;
        bool rt(Callee *c) const;
        void deduce(Deduction *d);
        void perform_deductions(GraphBuilder *gb);
        void find_all(GraphBuilder *gb);
        int find_inconsistent(GraphBuilder *gb);
        std::vector<Deduction*> d_list;
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

void DeductionsImpl::print_path(Callee *callee, GraphBuilder *gb)
{
    for(Deduction *d: d_list) {
        if(d->second == callee) {
            clang::DiagnosticsEngine *diag = d->TU->getDiagEng();
            if(d->CE) {
                diag->Report(d->CE->getLocStart(), error_realtime_saftey_trace)
                    << d->first->getName();
            } else
                diag->Report(error_realtime_safety_class) << d->second << d->first;

            print_path(d->first, gb);
            return;
        }
    }

    clang::DiagnosticsEngine *diag = callee->TU->getDiagEng();
    diag->Report(callee->DECL->getLocation(), error_realtime_saftey_trace_end)
        << callee->getName();
}

int DeductionsImpl::length(void) const
{
    return d_list.size();
}

void DeductionsImpl::print(void) const
{
    for(Deduction *d: d_list)
        printf("%30s :=> %30s\n", d->first->getName().c_str(), d->second->getName().c_str());
}

bool DeductionsImpl::rt(Callee *c) const
{
    for(Deduction *d: d_list)
        if(d->second == c)
            return true;
    return false;
}

void DeductionsImpl::deduce(Deduction *d)
{
    d_list.push_back(d);
}

void DeductionsImpl::perform_deductions(GraphBuilder *gb)
{
    Callees &callees = gb->getFunctions();
    for(Callee *e:callees) {
        if(!e->realtime_p() && !rt(e))
            continue;

        //Deduce all children must be realtime
        for(Call *c:gb->getCalls()) {
            if(c->first == e && !c->second->realtime_p()
                    && !rt(c->second))
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
    for(Callee *e: gb->getFunctions()) {
        if(e->not_realtime_p() && rt(e)) {
            clang::DiagnosticsEngine *diag =  e->TU->getDiagEng();
            diag->Report(e->DECL->getLocation(), error_realtime_saftey_violation)
                << e->getName() << e->reason;

            print_path(e, gb);
            result |= 2;
        }
        else if(!e->defined_p() && !e->realtime_p() && rt(e)) {
            //Check for function calls [virtual methods]
            for(Call *call : gb->getCalls())
                if(call->caller==e)
                    goto next;

            clang::DiagnosticsEngine *diag = e->TU->getDiagEng();
            diag->Report(e->DECL->getLocation(), warnn_realtime_saftey_unknown)
                << e->getName();

            print_path(e, gb);
            result |= 1;
        }
next:
        ;
    }
    return result;
}

