//= Instrumentation/DynCallGraphInsUtils.h - DynCallGraphInsUtils - Interface=//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines a few helper functions which are used by dynamic call graph
// instrumentation code to instrument the code.  This allows the dyncallgraph
// pass to worry about *what* to insert, and these functions take care of *how*
// to do it.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_INSTRUMENTATION_DYNCALLGRAPHINSUTILS_H
#define PARPOT_INSTRUMENTATION_DYNCALLGRAPHINSUTILS_H

#include "llvm/BasicBlock.h"
#include "llvm/Support/CallSite.h"

namespace llvm {
  class Function;
  class BasicBlock;

  /// inserts the call instruction for preparing a dynamic call graph with the
  /// collected calling information.
  void insertPrepareCallGraph(Function *mainFn, const char *fnName);

  /// Inserts a loop where several calls to the runtime library will be made.
  /// The needed time for all calls will be written to a global value.
  void insertOverheadMeasurement(Function *mainFn, const char *fnName,
																 const char *ovhdStart, const char *ovhdStop);

  /// Inserts an empty loop for measuring the runtime. The result will be
  /// written to a global value.
  void insertLoopOverheadMeasurement(Function *mainFn, const char *loopStart,
																											 const char *loopStop);

  /// adds a call to a given library function (fnName) which indicates an
  /// upcoming call instruction. This is an important step to build a dynamic
  /// call graph.
  void addNotifyCall(CallSite *cs, BasicBlock *bb, const char *fnNameBefore,
      const char *fnNameAfter, GlobalVariable *gv, unsigned fnNum);

  /// adds a call to a given library function (fnName) that register a called
  /// function in order to build a dynamic call graph.
  void addNotifyFnCalled(Function *fn, const char *fnName, GlobalVariable *gv);
}

#endif
