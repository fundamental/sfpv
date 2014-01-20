#include "TranslationUnit.h"
#include "GraphBuilder.h"
#include "Errors.h"

#include <llvm/Support/Host.h>
#include <llvm/Support/PathV1.h>
#include <llvm/Support/system_error.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Job.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Tool.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <cstdio>
#include <err.h>

class TranslationUnitImpl
{
    public:
        ~TranslationUnitImpl(void)
        {
            fprintf(stderr, "hm, ah, hrum...\n");
            //ci.getDiagnosticClient().EndSourceFile();
        }
        clang::CompilerInstance ci;
        std::string name;
};

TranslationUnit::TranslationUnit(std::string str)
    :impl(new TranslationUnitImpl)
{
    impl->name = str;
}

TranslationUnit::~TranslationUnit(void)
{
    delete impl;
}

const char *SaveStringInSet(std::set<std::string> &SavedStrings, llvm::StringRef S)
{
      return SavedStrings.insert(S).first->c_str();
}

void ExpandArgv(int argc, const char **argv,
                llvm::SmallVectorImpl<const char*> &ArgVector,
                std::set<std::string> &SavedStrings) {
    ArgVector.push_back(SaveStringInSet(SavedStrings, std::string(argv[0])));

    //Parse arguments delimited by space
    if(argc==2) {
        char *data_cpy = strdup(argv[1]);
        char *ptr = strtok(data_cpy, " ");
        do {
            ArgVector.push_back(SaveStringInSet(SavedStrings, std::string(ptr)));
        } while((ptr = strtok(NULL," ")));

        free(data_cpy);
    }
}

#ifndef CLANG_LOCATION
#define CLANG_LOCATION "/usr/local/bin/clang"
#endif

//Create a compiler instance and parse the AST
void TranslationUnit::collect(GraphBuilder *gb, char *clang_options)
{
    int argc = 1;
    const char *argv[2] = {"sfpv", NULL};
    if(clang_options != NULL) {
        argc = 2;
        argv[1] = clang_options;
    }

    //Setup Clang
    using clang::CompilerInstance;
    using clang::TargetOptions;
    using clang::TargetInfo;
    using clang::FileEntry;

    CompilerInstance &ci = impl->ci;

    //Hardcoded llvm system path
    const char *path = CLANG_LOCATION;

    //clang::TextDiagnosticPrinter* diagnostic_client =
    //    new clang::TextDiagnosticPrinter(llvm::errs(), clang::DiagnosticOptions());
    clang::DiagnosticOptions diags = clang::DiagnosticOptions();


    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagnostic_id(new clang::DiagnosticIDs());
    clang::DiagnosticsEngine *diagnostics =
        new clang::DiagnosticsEngine(diagnostic_id, &diags);
    clang::driver::Driver driver(path, llvm::sys::getDefaultTargetTriple(), "a.out",
            *diagnostics);


    driver.setTitle("Magical Magic");

    // Expand out any response files passed on the command line
    std::set<std::string> SavedStrings;
    llvm::SmallVector<const char*, 256> args;

    ExpandArgv(argc, argv, args, SavedStrings);
    args.push_back(impl->name.c_str());

    //Only Work on AST
    args.push_back("-fsyntax-only");
    llvm::OwningPtr<clang::driver::Compilation> compilation(driver.BuildCompilation(args));
    if (!compilation)
        errx(1, "Failed to setup fsyntax-only");

    const clang::driver::JobList& jobs = compilation->getJobs();
    //if (jobs.size() != 1 || !clang::isa<clang::driver::Command>(*jobs.begin()))
    //    errx(1, "Failed to get expected job count from deiver");

    const clang::driver::Command *command = clang::cast<clang::driver::Command>(*jobs.begin());
    if (llvm::StringRef(command->getCreator().getName()) != "clang")
        errx(1, "Doooom");

    // Initialize a compiler invocation object from the clang (-cc1) arguments.
    // More magic
    const clang::driver::ArgStringList &cc_arguments = command->getArguments();
    const char** args_start = const_cast<const char**>(cc_arguments.data());
    const char** args_end = args_start + cc_arguments.size();
    llvm::OwningPtr<clang::CompilerInvocation> invocation(new clang::CompilerInvocation);
    clang::CompilerInvocation::CreateFromArgs(*invocation,
            args_start, args_end, *diagnostics);
    invocation->getFrontendOpts().DisableFree = false;

    // Show the invocation, with -v.
    if (invocation->getHeaderSearchOpts().Verbose) {
        llvm::errs() << "clang invocation: unknown because they changed the API\n";
        llvm::errs() << "\n";
    }

    // Create a compiler instance to handle the actual work.
    // The caller will be responsible for freeing this.
    ci.setInvocation(invocation.take());

    ci.createDiagnostics();//0,NULL);

    TargetOptions to;
    to.Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *pti = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), &to);
    ci.setTarget(pti);

    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());
    ci.createPreprocessor();
    ci.createASTContext();

    //Initialize preprocessor
    ci.getPreprocessor().getBuiltinInfo().InitializeBuiltins(
            ci.getPreprocessor().getIdentifierTable(),
            ci.getLangOpts());

    //Select File
    const FileEntry *pFile = ci.getFileManager().getFile(impl->name);
    ci.getSourceManager().createMainFileID(pFile);

    //Set diagnostics
    ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
            &ci.getPreprocessor());

    //Parse with custom AST consumer to gather callgraph information
    clang::ASTConsumer *builder = gb->getConsumer(this);
    clang::ParseAST(ci.getPreprocessor(), builder,
            ci.getASTContext());
    errors_init(ci.getDiagnostics());
}

clang::DiagnosticsEngine *TranslationUnit::getDiagEng(void)
{
    return &impl->ci.getDiagnostics();
}

