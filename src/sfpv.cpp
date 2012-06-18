#include "TranslationUnit.h"
#include "FuncCalls.h"
#include "FuncEntries.h"
#include "Deductions.h"
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


//Perform manual annotations to the function entries
void add_manual_whitelist(const char *fname, FuncEntries &e)
{
    std::ifstream in(fname);
    while(in) {
        std::string word;
        in >> word;
        if(e.has(word))
            e[word].ext_realtime();
    }
    in.close();
}

void add_manual_blacklist(const char *fname, FuncEntries &e)
{
    std::string reason = "Unknown";
    std::ifstream in(fname);
    while(in) {
        std::string word;
        in >> word;
        printf("'%s'(%d)\n", word.c_str(),word.length());

        if(word[word.length()-1] == ':')
            reason = word.substr(0, word.length()-1);
        else if(e.has(word))
            e[word].ext_not_realtime(reason);
    }
    in.close();
}

//Verify the extention of a file
bool has_ext(const char *filename, const char *ext)
{
    if(!rindex(filename, '.'))
        return false;
    return !strcmp(rindex(filename, '.')+1, ext);
}

//Verify a file exists with a known extention
bool file_exists(const char *filename)
{
    if (FILE * file = fopen(filename, "r"))
    {
        fclose(file);

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
    "Usage: sfpv [-Qvh] [-W whitelist] [-C options] SOURCE-FILES\n\n" \
"    -Q\t\t  quiet mode, suppresses non warning/error related output\n" \
"    -v\t\t  Version\n"\
"    -h\t\t  help; prints this message\n"\
"    -W whitelist  uses the whitelist file to provide external annotations\n"\
"    -B blacklist  uses the blacklist file to provide external annotations\n"\
"    -C option     pass options directly to clang\n"


const char *whitelist_file = NULL;
const char *blacklist_file = NULL;
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

static char *clang_options = NULL;

int parse_arguments(int argc, char **argv)
{
    if(argc == 1)
        print_usage();
    int opt;
    while((opt = getopt(argc, argv, "QC:W:B:vh")) != -1) {
        switch(opt)
        {
            case 'W':
                whitelist_file = optarg;
                break;
            case 'B':
                blacklist_file = optarg;
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

//Print running information
void info(const char *str)
{
    if(!quiet)
        printf("[INFO] %s...\n", str);
}

int main(int argc, char **argv)
{
    int arg_loc = parse_arguments(argc, argv);

    info("Creating GraphBuilder");
    GraphBuilder gb;

    info("Setting up CompilerInstance for TranslationUnit");
    TranslationUnit **tus = new TranslationUnit*[argc-1];
    memset(tus, 0, sizeof(TranslationUnit*)*(argc-1));
    unsigned units = 0;
    for(int i=arg_loc; i<argc; ++i) {
        if(file_exists(argv[i])) {
            tus[units++] = new TranslationUnit(argv[i]);
        } else
            warn("Skipping invalid file %s...\n", argv[i]);
    }

    if(!units) {
        fprintf(stderr, "Expected valid translation units to parse...\n");
        exit(EXIT_FAILURE);
    }

    info("Collecting Information from TranslationUnit");
    for(unsigned i=0; i<units; ++i)
        tus[i]->collect(&gb, clang_options);

    if(whitelist_file) {
        info("Adding User Specified Whitelist");
        add_manual_whitelist(whitelist_file, gb.getFunctions());
    }

    if(blacklist_file) {
        info("Adding User Specified Blacklist");
        add_manual_blacklist(blacklist_file, gb.getFunctions());
    }

    info("Performing Deductions");
    Deductions ded(&gb);

    info("Printing Generated Information");

    //Show overall state
    if(!quiet) {
        info("Function Table");
        gb.getFunctions().print();
        info("Call Graph");
        gb.getCalls().print();
        info("Deductions");
        ded.print();
        puts("");
        info("Found Results");
    }

    //Get the actual errors from contradictions/ambiguities
    int result = ded.find_inconsistent(&gb);

    //Cleanup
    for(int i=0; i<argc-1; ++i)
        delete tus[i];
    delete[] tus;

    return result;
}
