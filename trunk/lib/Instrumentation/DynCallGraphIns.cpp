//===- DynCallGraphIns.cpp - Insert library calls for building a call graph ==//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This pass instruments the specified bytecode with some library calls in order
// to retrieve execution information. This is needed to build a dynamic
// call graph of the corresponding application.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "insert-callgraph-instructions"

#include "Instrumentation/DynCallGraphInsUtils.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "Instrumentation/Instrumentation.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Instructions.h"
#include <set>
#include <map>
using namespace llvm;

STATISTIC(NumFunctionsModified, "The # of functions modified.");

namespace {
  
  /// DynCallGraphIns is a module pass which inserts library calls in llvm
  /// bytecode in order to retrieve a dynamic call graph of an execution.
  class DynCallGraphIns : public ModulePass {
    bool runOnModule(Module &M);
  private:
    GlobalVariable* getGV(Module &mod, LLVMContext &context, StringRef str);
  public:
    static char ID; // Pass identification, replacement for typeid
    DynCallGraphIns() : ModulePass(ID) {}

    virtual const char *getPassName() const {
      return "Dynamic callgraph instrumentation";
    }
  };
}

char DynCallGraphIns::ID = 0;
static RegisterPass<DynCallGraphIns>
X("insert-callgraph-instructions",
    "Insert instrumentation for building a call graph");

ModulePass *llvm::createDynCallGraphPass() {
  return new DynCallGraphIns();
}

GlobalVariable* DynCallGraphIns::getGV(Module &mod, LLVMContext &context,
    StringRef str) {
  static std::map<StringRef, GlobalVariable*> globalVars;

  // check if global variable already exist
  if (globalVars.find(str) != globalVars.end())
    return globalVars.find(str)->second;

  // add global variable with function name
  Constant *msg_0 = ConstantArray::get(context, str);
  GlobalVariable *fnc = new GlobalVariable(
    mod, msg_0->getType(), true, GlobalValue::InternalLinkage, msg_0, "fnc");
  globalVars[str] = fnc;
  return fnc;
}

bool DynCallGraphIns::runOnModule(Module &M) {
  
  // retrieve main function
  Function *Main = M.getFunction("main");
  if (Main == 0) {
    errs() << "WARNING: cannot insert call graph instrumentation into a module"
           << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  // filter functions that shall be instrumented
  std::set<Function*> FunctionsToInstrument;
  unsigned NumFunctions = 0;
  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
    if (F->isDeclaration()) continue;
    ++NumFunctions;
    FunctionsToInstrument.insert(F);
  }
  NumFunctionsModified = NumFunctions;

  // instrument every call / invoke instruction
  unsigned int i = 1; // 0 is for main function
  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
    if (FunctionsToInstrument.count(F)) {

      // add notify-call instructions
      for (inst_iterator it = inst_begin(F), e = inst_end(F); it != e; ++it) {
        CallSite *pCS;
        if (CallInst *callInst = dyn_cast<CallInst>(&*it))
          pCS = new CallSite(callInst);
        else if (InvokeInst *invokeInst = dyn_cast<InvokeInst>(&*it))
          pCS = new CallSite(invokeInst);
        else
          continue;

        Function *f = pCS->getCalledFunction();
        GlobalVariable *gv;
        if (f && f->getNameStr() == "llvm_call_finished_instruction")
        	continue;
        if (f) {
          gv = getGV(M, F->getContext(), f->getName());
        } else {
          gv = getGV(M, F->getContext(), "ext");
        }
        addNotifyCall(pCS , it->getParent(), "llvm_call_instruction",
            "llvm_call_finished_instruction", gv, i);

        delete pCS;

        ++it; // increment iterator to pass over new calls
        i++; // increment function count
      }

      // add function called - call
      GlobalVariable *gv = getGV(M, F->getContext(), F->getName());
      addNotifyFnCalled(&*F, "llvm_function_called", gv);
      }
    }

  // Add the initialization call to main.
  insertPrepareCallGraph(Main, "llvm_build_and_write_dyncallgraph");

  // Add overhead measurement code to main
  insertOverheadMeasurement(Main, "llvm_dummy_call",
																	"llvm_ovhd_start",
																	"llvm_ovhd_stop");

  // Add loop overhead measurement code to main
  insertLoopOverheadMeasurement(Main, "llvm_loop_start", "llvm_loop_stop");
  return true;
}

