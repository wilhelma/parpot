////===---- Instrumentation/Instrumentation.h - Instrumentation - Interface--===//
////
////                 ParPot - Parallelization Potential - Measurement
////
////===----------------------------------------------------------------------===//
////
//// This file defines the Instrumen class, which contains debug information for
//// allocation instructions of llvm-bytecode. Additionally a functor is defined
//// to compare allocation instances with strings.
////
////===----------------------------------------------------------------------===//
#ifndef TIMEPROFILE_INSTRUMENTATION_H
#define TIMEPROFILE_INSTRUMENTATION_H

namespace llvm {

class ModulePass;

// Insert edge profiling instrumentation
ModulePass *createFTimeProfilerPass();

// Insert dynamic call graph instrumentation
ModulePass *createDynCallGraphPass();

} // End llvm namespace

#endif
