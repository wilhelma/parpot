//=== Instrumentation/TimeProfilingUtils.h - TimeProfilingUtils - Interface===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines a few helper functions which are used by profile
// instrumentation code to instrument the code.  This allows the profiler pass
// to worry about *what* to insert, and these functions take care of *how* to do
// it.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_INSTRUMENTAION_TIMEPROFILINGUTILS_H
#define PARPOT_INSTRUMENTAION_TIMEPROFILINGUTILS_H

namespace llvm {
  class Function;
  class GlobalValue;
  class BasicBlock;

  /// Inserts the init call for time profiling (FnName) into the main function.
  /// Additionally an array for the timing information per function will be
  /// inserted.
  void InsertTimeProfilingInitCall(Function *MainFn, const char *FnName,
                               GlobalValue *Arr = 0);
  
  /// Adds getTime (FnName) calls at the given basic-block as well as in the
  /// last basic-block of the function. Afterwards a substraction statement
  /// will be inserted that calculates the execution time of the function and
  /// stores it into the array.
  void AddGetTimesInFunction(Function *f, const char *FnName,
                                unsigned CounterNum, GlobalValue *TimerArray);

}

#endif
