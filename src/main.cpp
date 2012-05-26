#include <iostream>

#include <llvm/Support/raw_ostream.h>

#include <clang/AST/StmtVisitor.h>

#include "TranslationUnit.h"
#include "Types.h"
#include "GraphBuilder.h"
#include "Errors.h"

#include <string>
#include <cstdio>
#include <fstream>
#include <err.h>
#include <unistd.h>

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
            if(d.CE) {
                std::string str = d.CE->getDirectCallee()->getQualifiedNameAsString();
                diag->Report(d.CE->getLocStart(), error_realtime_saftey_trace) << str;
            } else
                diag->Report(error_realtime_safety_class) << d.second << d.first;

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
            //Check for function calls [virtual methods]
            for(auto call : gb->getCalls())
                if(call.first==e.name)
                    goto next;

            clang::DiagnosticsEngine *diag = (clang::DiagnosticsEngine *) e.TU->getDiagEng();
            diag->Report(e.FDECL->getLocation(), warnn_realtime_saftey_unknown)
                << e.FDECL->getQualifiedNameAsString();

            dlist_print_path(e.name, gb);
            result |= 1;
        }
next:
        ;
    }
    return result;
}

void add_manual_annotations(const char *fname, FuncEntries &e)
{
    std::ifstream in(fname);
    while(in) {
        std::string word;
        in >> word;
        if(e.has(word))
            e[word].ext_realtime();
    }
}
bool has_ext(const char *filename, const char *ext)
{
    if(!rindex(filename, '.'))
        return false;
    return !strcmp(rindex(filename, '.')+1, ext);
}

bool file_exists(const char * filename)
{
    if (FILE * file = fopen(filename, "r"))
    {
        printf("exists: %s\n", filename);
        fclose(file);
        printf("%s\n", rindex(filename, '.'));

        //Check for extension
        if(has_ext(filename, "cpp"))
            return true;
        if(has_ext(filename, "c"))
            return true;
        if(has_ext(filename, "C"))
            return true;
        if(has_ext(filename, "cxx"))
            return true;
        if(has_ext(filename, "h"))
            return true;

        //Found nothing
        return false;
    }
    return false;
}

#define USAGE \
    "Usage: sfpv [-Qvh] [-W whitelist] SOURCE-FILES\n\n" \
"    -Q\t\t  quiet mode, suppresses non warning/error related output\n" \
"    -v\t\t  Version\n"\
"    -h\t\t  help; prints this message\n"\
"    -W whitelist  uses the whitelist file to provide external annotations\n"\
"    -C option     pass option directly to clang\n"


const char *whitelist_file = NULL;
void print_usage(void)
{
    fprintf(stderr, USAGE);
    exit(EXIT_FAILURE);
}

//This program defaults to being very noisy and it will produce pages of output
//as it lists its internal state
//This option produces a more sane level of output
bool quiet = false;

void print_version(void)
{
    printf("Version %s\n", VERSION);
    exit(0);
}

void info(const char *str)
{
    if(!quiet)
        printf("[INFO] %s...\n", str);
}

static char *clang_options = NULL;

int parse_arguments(int argc, char **argv)
{
    if(argc == 1)
        print_usage();
    int opt;
    while((opt = getopt(argc, argv, "QC:W:vh")) != -1) {
        switch(opt)
        {
            case 'W':
                whitelist_file = optarg;
                break;
            case 'C':
                clang_options = strdup(optarg);
                break;
            case 'Q':
                quiet = true;
                break;
            case 'v':
                print_version();
                break;
            case 'h':
            default:
                print_usage();
        }
    }

    return optind;
}

int main(int argc, char **argv)
{
    int arg_loc = parse_arguments(argc, argv);

    info("Creating GraphBuilder");
    GraphBuilder gb;

    info("Setting up CompilerInstance for TranslationUnit");
    TranslationUnit **tus = new TranslationUnit*[argc-1];
    unsigned units = 0;
    for(int i=arg_loc; i<argc; ++i) {
        if(file_exists(argv[i])) {
            printf("Adding %s...\n", argv[i]);
            tus[units++] = new TranslationUnit(argv[i]);
        } else
            warn("Skipping invalid file %s...\n", argv[i]);
    }

    if(!units) {
        fprintf(stderr, "Expected valid translation units to parse...\n");
        exit(EXIT_FAILURE);
    }

    info("Collecting Information from TranslationUnit");
    for(int i=0; i<units; ++i)
        tus[i]->collect(&gb, clang_options);

    if(whitelist_file) {
        info("Adding User Specified Annotations");
        add_manual_annotations(whitelist_file, gb.getFunctions());
    }

    info("Performing Deductions");
    dlist_find_all(&gb);

    info("Printing Generated Information");

    //Show overall state
    if(!quiet) {
        info("Function Table");
        gb.getFunctions().print();
        info("Call Graph");
        gb.getCalls().print();
        info("Deductions");
        dlist_print();
        puts("");
        info("Found Results");
    }
    int result = find_inconsistent(&gb);

    //Cleanup
    for(int i=0; i<argc-1; ++i)
        delete tus[i];
    delete[] tus;

    return result;
}
