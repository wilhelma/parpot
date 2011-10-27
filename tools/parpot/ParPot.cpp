//===------------ ParPot.cpp - Parallelization measurement tool -----------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// The ParPot tool for measuring parallelization potential.
//
//===----------------------------------------------------------------------===//

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/PassManager.h"

#include "Instrumentation/Instrumentation.h"

#include <cerrno>
#include <memory>
using namespace llvm;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input bytcode>"), cl::Positional, cl::init("-"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));

  // Determine optimization level.
  cl::opt<char>
  OptLevel("O",
           cl::desc("Optimization level. [-O0, -O1, -O2, or -O3] "
                    "(default = '-O2')"),
           cl::Prefix,
           cl::ZeroOrMore,
           cl::init(' '));

  cl::opt<std::string>
  TargetTriple("mtriple", cl::desc("Override target triple for module"));

  cl::opt<std::string>
  MArch("march",
        cl::desc("Architecture to generate assembly for (see --version)"));

  cl::opt<std::string>
  MCPU("mcpu",
       cl::desc("Target a specific cpu type (-mcpu=help for details)"),
       cl::value_desc("cpu-name"),
       cl::init(""));

  cl::list<std::string>
  MAttrs("mattr",
         cl::CommaSeparated,
         cl::desc("Target specific attributes (-mattr=help for details)"),
         cl::value_desc("a1,+a2,-a3,..."));

  cl::opt<std::string>
  EntryFunc("entry-function",
            cl::desc("Specify the entry function (default = 'main') "
                     "of the executable"),
            cl::value_desc("function"),
            cl::init("main"));
  
  cl::opt<std::string>
  FakeArgv0("fake-argv0",
            cl::desc("Override the 'argv[0]' value passed into the executing"
                     " program"), cl::value_desc("executable"));

  cl::opt<bool>
  NoLazyCompilation("disable-lazy-compilation",
                  cl::desc("Disable JIT lazy compilation"),
                  cl::init(false));
}

static ExecutionEngine *EE = 0;

static void do_shutdown() {
  delete EE;
  llvm_shutdown();
}

// GetFileNameRoot - Helper function to get the basename of a filename.
static inline std::string getFileNameRoot(const std::string &InputFilename) {
  std::string IFN = InputFilename;
  std::string outputFilename;
  int Len = IFN.length();
  if ((Len > 2) &&
      IFN[Len-3] == '.' && IFN[Len-2] == 'b' && IFN[Len-1] == 'c') {
    outputFilename = std::string(IFN.begin(), IFN.end()-3); // s/.bc/.s/
  } else {
    outputFilename = IFN;
  }
  return outputFilename;
}

static int insTimeProfiling(int argc, char **argv, std::string &outFile) {
  // Load the module to be compiled...
  std::auto_ptr<Module> mod;
  std::string Errormessage;

  OwningPtr<MemoryBuffer> File;
  if (error_code ec = MemoryBuffer::getFileOrSTDIN(InputFile, File)) {
    mod.reset(ParseBitcodeFile(File.get(), getGlobalContext(), &Errormessage));
  }

  if (mod.get() == 0) {
    errs() << argv[0] << ": bytecode didn't read correctly.\n";
    return 1;
  }

  // Build up all of the passes that we want to do to the module...
  PassManager passes;

  // add instrumentation pass for function time profiling
  passes.add(createFTimeProfilerPass());

  // construct output filename
  outFile = getFileNameRoot(InputFile);
  outFile += ".ftime.inst";

  // prepare output file
  raw_fd_ostream *out = 0;
  std::string error;
  out = new raw_fd_ostream(outFile.c_str(), error);
  if (error.length()) {
    errs() << argv[0] << ": error opening " << outFile << "!\n";
    delete out;
    return 1;
  }

  // make sure that the Out file gets unlinked from the disk if we get a
  // SIGINT
  sys::RemoveFileOnSignal(sys::Path(outFile));

  // Add the writing of the output file to the list of passes
  passes.add (createBitcodeWriterPass(*out));

  // Run our queue of passes all at once now, efficiently.
  passes.run(*mod.get());

  // Delete the ostream
  delete out;

  return 0;
}

static int insDynCallGraph(int argc, char **argv, std::string &outFile) {
  // Load the module to be compiled...
  std::auto_ptr<Module> mod;
  std::string Errormessage;

  OwningPtr<MemoryBuffer> File;
  if (error_code ec = MemoryBuffer::getFileOrSTDIN(InputFile, File)) {
    mod.reset(ParseBitcodeFile(File.get(), getGlobalContext(), &Errormessage));
  }

  if (mod.get() == 0) {
    errs() << argv[0] << ": bytecode didn't read correctly.\n";
    return 1;
  }

  // Build up all of the passes that we want to do to the module...
  PassManager passes;

  // add instrumentation pass for function time profiling
  passes.add(createDynCallGraphPass());

  // construct output filename
  outFile = getFileNameRoot(InputFile);
  outFile += ".dcg.inst";

  // prepare output file
  raw_fd_ostream *out = 0;
  std::string error;
  out = new raw_fd_ostream(outFile.c_str(), error);
  if (error.length()) {
    errs() << argv[0] << ": error opening " << outFile << "!\n";
    delete out;
    return 1;
  }

  // make sure that the Out file gets unlinked from the disk if we get a
  // SIGINT
  sys::RemoveFileOnSignal(sys::Path(outFile));

  // Add the writing of the output file to the list of passes
  passes.add (createBitcodeWriterPass(*out));

  // Run our queue of passes all at once now, efficiently.
  passes.run(*mod.get());

  // Delete the ostream
  delete out;

  return 0;
}

int execute(std::string file, int argc, char **argv, char * const *envp) {

  LLVMContext &Context = getGlobalContext();

  // Load the bitcode...
  std::string ErrorMsg;
  Module *Mod = NULL;

  OwningPtr<MemoryBuffer> fileBuf;
  error_code ec = MemoryBuffer::getFileOrSTDIN(file, fileBuf);
  if (!ec.value()) {
	Mod = getLazyBitcodeModule(fileBuf.get(), Context, &ErrorMsg);
  }

  if (!Mod) {
    errs() << argv[0] << ": error loading program '" << file << "': "
           << ErrorMsg << "\n";
    exit(1);
  }

  // If not jitting lazily, load the whole bitcode file eagerly too.
  if (NoLazyCompilation) {
    if (Mod->MaterializeAllPermanently(&ErrorMsg)) {
      errs() << argv[0] << ": bitcode didn't read correctly.\n";
      errs() << "Reason: " << ErrorMsg << "\n";
      exit(1);
    }
  }

  EngineBuilder builder(Mod);
  builder.setMArch(MArch);
  builder.setMCPU(MCPU);
  builder.setMAttrs(MAttrs);
  builder.setErrorStr(&ErrorMsg);
  builder.setEngineKind(EngineKind::JIT);

  // If we are supposed to override the target triple, do so now.
  if (!TargetTriple.empty())
    Mod->setTargetTriple(TargetTriple);

  CodeGenOpt::Level OLvl = CodeGenOpt::Default;
  switch (OptLevel) {
  default:
    errs() << argv[0] << ": invalid optimization level.\n";
    return 1;
  case ' ': break;
  case '0': OLvl = CodeGenOpt::None; break;
  case '1': OLvl = CodeGenOpt::Less; break;
  case '2': OLvl = CodeGenOpt::Default; break;
  case '3': OLvl = CodeGenOpt::Aggressive; break;
  }
  builder.setOptLevel(OLvl);

  EE = builder.create();
  if (!EE) {
    if (!ErrorMsg.empty())
      errs() << argv[0] << ": error creating EE: " << ErrorMsg << "\n";
    else
      errs() << argv[0] << ": unknown error creating EE!\n";
    exit(1);
  }

  EE->RegisterJITEventListener(createOProfileJITEventListener());

  EE->DisableLazyCompilation(NoLazyCompilation);

  // If the user specifically requested an argv[0] to pass into the program,
  // do it now.
  if (!FakeArgv0.empty()) {
    file = FakeArgv0;
  } else {
    // Otherwise, if there is a .bc suffix on the executable strip it off, it
    // might confuse the program.
    if (file.rfind(".bc") == file.length() - 3)
      file.erase(file.length() - 3);
  }

  // Add the module's name to the start of the vector of arguments to main().
  InputArgv.insert(InputArgv.begin(), file);

  // Call the main function from M as if its signature were:
  //   int main (int argc, char **argv, const char **envp)
  // using the contents of Args to determine argc & argv, and the contents of
  // EnvVars to determine envp.
  //
  Function *EntryFn = Mod->getFunction(EntryFunc);
  if (!EntryFn) {
    errs() << '\'' << EntryFunc << "\' function not found in module.\n";
    return -1;
  }

  // If the program doesn't explicitly call exit, we will need the Exit 
  // function later on to make an explicit call, so get the function now. 
  Constant *Exit = Mod->getOrInsertFunction("exit", Type::getVoidTy(Context),
                                                    Type::getInt32Ty(Context),
                                                    NULL);
  
  // Reset errno to zero on entry to main.
  errno = 0;
 
  // Run static constructors.
  EE->runStaticConstructorsDestructors(false);

  if (NoLazyCompilation) {
    for (Module::iterator I = Mod->begin(), E = Mod->end(); I != E; ++I) {
      Function *Fn = &*I;
      if (Fn != EntryFn && !Fn->isDeclaration())
        EE->getPointerToFunction(Fn);
    }
  }

  // Run main.
  int Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);

  // Run static destructors.
  EE->runStaticConstructorsDestructors(true);
  
  // If the program didn't call exit explicitly, we should call it now. 
  // This ensures that any atexit handlers get called correctly.
  if (Function *ExitF = dyn_cast<Function>(Exit)) {
    std::vector<GenericValue> Args;
    GenericValue ResultGV;
    ResultGV.IntVal = APInt(32, Result);
    Args.push_back(ResultGV);
    EE->runFunction(ExitF, Args);
    errs() << "ERROR: exit(" << Result << ") returned!\n";
    abort();
  } else {
    errs() << "ERROR: exit defined with wrong prototype!\n";
    abort();
  }
  return 0;
}

//===----------------------------------------------------------------------===//
// main Driver function
//
int main(int argc, char **argv, char * const *envp) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);


  atexit(do_shutdown);  // Call llvm_shutdown() on exit.

  // If we have a native target, initialize it to ensure it is linked in and
  // usable by the JIT.
  InitializeNativeTarget();

  cl::ParseCommandLineOptions(argc, argv, "parpot measurement tool\n");
  std::string file;

  /*
   * Function time profiling
   */

  insTimeProfiling(argc, argv, file); // instrument file
  execute(file, argc, argv, envp);// execute instrumented bytecode

  /*
   * Dynamic call graph construction
   */
  insDynCallGraph(argc, argv, file); // instrument file
  execute(file, argc, argv, envp);// execute instrumented bytecode

}
