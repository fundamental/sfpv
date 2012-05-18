#define __STDC_CONSTANT_MACROS 1
#define __STDC_FORMAT_MACROS 1
#define __STDC_LIMIT_MACROS 1
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
            ci.getDiagnosticClient().EndSourceFile();
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

//Magic from include-what-you-use
//This should be reworked before any official release
void ExpandArgsFromBuf(const char *Arg,
                       clang::SmallVectorImpl<const char*> &ArgVector,
                       std::set<std::string> &SavedStrings) {
  const char *FName = Arg + 1;
  llvm::OwningPtr<llvm::MemoryBuffer> MemBuf;
  if (llvm::MemoryBuffer::getFile(FName, MemBuf)) {
    ArgVector.push_back(SaveStringInSet(SavedStrings, Arg));
    return;
  }

  const char *Buf = MemBuf->getBufferStart();
  char InQuote = ' ';
  std::string CurArg;

  for (const char *P = Buf; ; ++P) {
    if (*P == '\0' || (isspace(*P) && InQuote == ' ')) {
      if (!CurArg.empty()) {

        if (CurArg[0] != '@') {
          ArgVector.push_back(SaveStringInSet(SavedStrings, CurArg));
        } else {
          ExpandArgsFromBuf(CurArg.c_str(), ArgVector, SavedStrings);
        }

        CurArg = "";
      }
      if (*P == '\0')
        break;
      else
        continue;
    }

    if (isspace(*P)) {
      if (InQuote != ' ')
        CurArg.push_back(*P);
      continue;
    }

    if (*P == '"' || *P == '\'') {
      if (InQuote == *P)
        InQuote = ' ';
      else if (InQuote == ' ')
        InQuote = *P;
      else
        CurArg.push_back(*P);
      continue;
    }

    if (*P == '\\') {
      ++P;
      if (*P != '\0')
        CurArg.push_back(*P);
      continue;
    }
    CurArg.push_back(*P);
  }
}

void ExpandArgv(int argc, const char **argv,
                llvm::SmallVectorImpl<const char*> &ArgVector,
                std::set<std::string> &SavedStrings) {
  for (int i = 0; i < argc; ++i) {
    const char *Arg = argv[i];
    if (Arg[0] != '@') {
      ArgVector.push_back(SaveStringInSet(SavedStrings, std::string(Arg)));
      continue;
    }

    ExpandArgsFromBuf(Arg, ArgVector, SavedStrings);
  }
}

//Create a compiler instance and parse the AST
void TranslationUnit::collect(GraphBuilder *gb, int argc, const char**argv)
{
    argc = 1;
    const char *name = "sfpv";
    argv = &name;

    //Setup Clang
    using clang::CompilerInstance;
    using clang::TargetOptions;
    using clang::TargetInfo;
    using clang::FileEntry;

    CompilerInstance &ci = impl->ci;

    //Hardcoded llvm system path
    llvm::sys::Path path("/usr/local/bin/clang");

    clang::TextDiagnosticPrinter* diagnostic_client =
        new clang::TextDiagnosticPrinter(llvm::errs(), clang::DiagnosticOptions());

    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagnostic_id(new clang::DiagnosticIDs());
    clang::DiagnosticsEngine diagnostics(diagnostic_id, diagnostic_client);
    clang::driver::Driver driver(path.str(), llvm::sys::getDefaultTargetTriple(), "a.out", false,
            diagnostics);


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
            args_start, args_end, diagnostics);
    invocation->getFrontendOpts().DisableFree = false;

    // Show the invocation, with -v.
    if (invocation->getHeaderSearchOpts().Verbose) {
        llvm::errs() << "clang invocation:\n";
        compilation->PrintJob(llvm::errs(), compilation->getJobs(), "\n", true);
        llvm::errs() << "\n";
    }

    // Create a compiler instance to handle the actual work.
    // The caller will be responsible for freeing this.
    ci.setInvocation(invocation.take());

    ci.createDiagnostics(0,NULL);

    TargetOptions to;
    to.Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *pti = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), to);
    ci.setTarget(pti);

    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());
    ci.createPreprocessor();

    //Add custom AST consumer to gather callgraph information
    clang::ASTConsumer *builder = gb->getConsumer(this);
    ci.setASTConsumer(builder);
    ci.createASTContext();

    //Select File
    printf("File Name (%s)\n", impl->name.c_str());
    const FileEntry *pFile = ci.getFileManager().getFile(impl->name);
    ci.getSourceManager().createMainFileID(pFile);

    //Set diagnostics
    ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
            &ci.getPreprocessor());

    //Parse
    clang::ParseAST(ci.getPreprocessor(), builder,
            ci.getASTContext());
    errors_init(ci.getDiagnostics());
}
        
void *TranslationUnit::getDiagEng(void)
{
    return &impl->ci.getDiagnostics();
}

