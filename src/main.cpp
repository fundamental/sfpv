#define __STDC_CONSTANT_MACROS 1
#define __STDC_FORMAT_MACROS 1
#define __STDC_LIMIT_MACROS 1
#include <iostream>

#include <llvm/Support/raw_ostream.h>

#include <clang/AST/StmtVisitor.h>

#include "TranslationUnit.h"
#include "Types.h"
#include "GraphBuilder.h"
#include "Errors.h"

#include <string>
#include <cstdio>
#include <err.h>

//C++11 extentions are useful, but setting the standard to be C++11 breaks
//compilation, so lets just sweep these errors under a rug for now
#pragma clang diagnostic ignored "-Wc++11-extensions"
#define CHECK(x) do{if(!x) errx(1, "expected '%s'", #x);}while(0)

typedef CallPair Deduction;
std::vector<Deduction> d_list;

///////////////////////////////
//Deduction List Functions   //
///////////////////////////////

void dlist_print_path(std::string func, GraphBuilder *gb)
{
    for(Deduction d: d_list) {
        if(d.second == func) {
            clang::DiagnosticsEngine *diag = (clang::DiagnosticsEngine *) d.TU->getDiagEng();
            std::string str = d.CE->getDirectCallee()->getQualifiedNameAsString();
            diag->Report(d.CE->getLocStart(), error_realtime_saftey_trace) << str;

            dlist_print_path(d.first, gb);
            return;
        }
    }

    FuncEntry f = gb->getFunctions()[func];
    clang::DiagnosticsEngine *diag = (clang::DiagnosticsEngine *) f.TU->getDiagEng();
    diag->Report(f.FDECL->getLocation(), error_realtime_saftey_trace_end) << f.FDECL->getQualifiedNameAsString();
}

int dlist_length(void)
{
    return d_list.size();
}

void dlist_print(void)
{
    for(Deduction d: d_list)
        printf("%30s :=> %30s\n", d.first.c_str(), d.second.c_str());
}

bool dlist_rt(std::string fname)
{
    for(Deduction d: d_list)
        if(d.second == fname)
            return true;
    return false;
}

void dlist_add(CallPair c)
{
    d_list.push_back(c);
}

void dlist_deduce(GraphBuilder *gb)
{
    FuncEntries &entries = gb->getFunctions();
    for(auto pair:entries) {
        FuncEntry e = pair.second;
        if(!e.realtime_p() && !dlist_rt(e.name))
            continue;

        //Deduce all children must be realtime
        for(CallPair c:gb->getCalls()) {
            if(c.first == e.name && !entries[c.second].realtime_p()
                    && !dlist_rt(c.second))
                dlist_add(c);//e.name,c.second);
        }
    }
}

void dlist_find_all(GraphBuilder *gb)
{
    int len = 0;
    do {
        len = dlist_length();
        dlist_deduce(gb);
    } while(len != dlist_length());
}


int find_inconsistent(GraphBuilder *gb)
{
    int result = 0; //aka all is safe
    for(auto pair: gb->getFunctions()) {
        FuncEntry e = pair.second;
        if(e.not_realtime_p() && dlist_rt(e.name)) {
            clang::DiagnosticsEngine *diag = (clang::DiagnosticsEngine *) e.TU->getDiagEng();
            diag->Report(e.FDECL->getLocation(), error_realtime_saftey_violation)
                << e.FDECL->getQualifiedNameAsString();

            dlist_print_path(e.name, gb);
            result |= 2;
        }
        if(!e.defined_p() && !e.realtime_p() && dlist_rt(e.name)) {

            clang::DiagnosticsEngine *diag = (clang::DiagnosticsEngine *) e.TU->getDiagEng();
            diag->Report(e.FDECL->getLocation(), warnn_realtime_saftey_unknown)
                << e.FDECL->getQualifiedNameAsString();

            dlist_print_path(e.name, gb);
            result |= 1;
        }
    }
    return result;
}

void info(const char *str)
{
    printf("[INFO] %s...\n", str);
}

bool file_exists(const char * filename)
{
    if (FILE * file = fopen(filename, "r"))
    {
        fclose(file);
        return true;
    }
    return false;
}

int main(int argc, const char **argv)
{
    if(argc == 1) {
        printf("usage: %s SOURCE-FILES\n", argv[0]);
        return 255;
    }

    info("Creating GraphBuilder");
    GraphBuilder gb;

    info("Setting up CompilerInstance for TranslationUnit");
    TranslationUnit **tus = new TranslationUnit*[argc-1];
    unsigned units = 0;
    for(int i=1; i<argc; ++i) {
        if(file_exists(argv[i])) {
            printf("Adding %s...\n", argv[i]);
            tus[units++] = new TranslationUnit(argv[i]);
        } else
            printf("Skipping invalid file %s...\n", argv[i]);
    }

    info("Collecting Information from TranslationUnit");
    for(int i=0; i<units; ++i)
        tus[i]->collect(&gb, argc, argv);

    info("Performing Deductions");
    dlist_find_all(&gb);

    info("Printing Generated Information");

    //Show overall state
    info("Function Table");
    gb.getFunctions().print();
    info("Call Graph");
    gb.getCalls().print();
    info("Deductions");
    dlist_print();
    puts("");
    info("Found Results");
    int result = find_inconsistent(&gb);

    //Cleanup
    for(int i=0; i<argc-1; ++i)
        delete tus[i];
    delete[] tus;

    return result;
}
