/*===-- FunctionTimeProfiling.c - Support library for f.t. profiling ------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source      
|* License. See LICENSE.TXT for details.                                      
|* 
|*===----------------------------------------------------------------------===*|
|* 
|* This file implements the call back routines for the function time profiling
|* instrumentation pass.  This should be used with the -insert-ftime-profiling
|* LLVM pass.
|*
\*===----------------------------------------------------------------------===*/

#include "Profiling.h"
#include <stdlib.h>
#include <stdint.h>
#include <papi.h>
#include <stdio.h>


static double *ArrayStart;
static unsigned NumElements;

//__inline__ uint64_t rdtsc() {
//  uint32_t lo, hi;
//  __asm__ __volatile__ (      // serialize
//  "xorl %%eax,%%eax \n        cpuid"
//  ::: "%rax", "%rbx", "%rcx", "%rdx");
//  /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
//  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
//  return (uint64_t)hi << 32 | lo;
//}

/* EdgeProfAtExitHandler - When the program exits, just write out the profiling
 * data.
 */
static void TimeProfAtExitHandler() {
  write_profiling_data_d(ArrayStart, NumElements);
}


/* llvm_start_ftime_profiling - This is the main entry point of the function
 * time profiling library.  It is responsible for setting up the atexit handler.
 */
int llvm_start_ftime_profiling(int argc, const char **argv,
                              double *arrayStart, unsigned numElements) {
  int Ret = save_arguments(argc, argv);
  ArrayStart = arrayStart;
  NumElements = numElements;
  atexit(TimeProfAtExitHandler);
  return Ret;
}

double llvm_get_time() {
  return PAPI_get_real_cyc();

//  struct timespec now;
//  clock_gettime(CLOCK_REALTIME_HR, &now);
//  return now.tv_nsec;
}
