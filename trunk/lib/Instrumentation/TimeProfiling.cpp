//===- TimeProfiling.cpp - Insert timers for function tiem profiling ------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This pass instruments the specified program with getTime() calls for function
// time profiling.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "insert-ftime-profiling"
#include "Instrumentation/TimeProfilingUtils.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "Instrumentation/Instrumentation.h"
#include "llvm/ADT/Statistic.h"
#include <set>
using namespace llvm;

STATISTIC(NumFunctionsModified, "The # of functions modified.");

namespace {
  class FunctionTimeProfiler : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID; // Pass identification, replacement for typeid
    FunctionTimeProfiler() : ModulePass(ID) {}

    virtual const char *getPassName() const {
      return "Function Time Profiler";
    }
  };
}

char FunctionTimeProfiler::ID = 0;
static RegisterPass<FunctionTimeProfiler>
X("insert-ftime-profiling", "Insert instrumentation for ftime profiling");

ModulePass *llvm::createFTimeProfilerPass() {
  return new FunctionTimeProfiler();
}

bool FunctionTimeProfiler::runOnModule(Module &M) {
  Function *Main = M.getFunction("main");
  if (Main == 0) {
    errs() << "WARNING: cannot insert edge profiling into a module"
           << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  std::set<Function*> FunctionsToInstrument;
  unsigned NumFunctions = 0;
  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
    if (F->isDeclaration()) continue;
    ++NumFunctions;
    FunctionsToInstrument.insert(F);
  }

  Type *ATy = ArrayType::get(Type::getDoubleTy(M.getContext()), NumFunctions);
  GlobalVariable *Timers =
    new GlobalVariable(M, ATy, false, GlobalValue::InternalLinkage,
                       Constant::getNullValue(ATy), "FunctionProfTimers");
  NumFunctionsModified = NumFunctions;

  // Instrument all of the functions...
  unsigned i = 0;
  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
    if (FunctionsToInstrument.count(F)) {
    	// instrument first basic block of function
    	AddGetTimesInFunction(&*F, "llvm_get_time", i++, Timers);
    }
  }

  // Add the initialization call to main.
  InsertTimeProfilingInitCall(Main, "llvm_start_ftime_profiling", Timers);
  return true;
}

