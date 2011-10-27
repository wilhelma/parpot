/*===------ Analysis/TimeProfileInfoTypes.h - Timeprofiling Types  --------===//
|*
|*                 ParPot - Parallelization Potential - Measurement
|*
|*===----------------------------------------------------------------------===//
|*
|* This file defines constants shared by the various different time profiling
|* runtime libraries and the LLVM C++ profile info loader. It must be a
|* C header because, at present, the profiling runtimes are written in C.
|*
\*===----------------------------------------------------------------------===*/

#ifndef PARPOT_ANALYSIS_TIMEPROFILEINFOTYPES_H
#define PARPOT_ANALYSIS_TIMEPROFILEINFOTYPES_H

enum TimeProfilingType {
  ArgumentInfo  = 1,   /* The command line argument block */
  FunctionTInfo = 8	   /* Function time-profiling information */
};

#endif
