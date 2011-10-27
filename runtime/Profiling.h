/*===-- Profiling.h - Profiling support library support routines --*- C -*-===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source      
|* License. See LICENSE.TXT for details.                                      
|*
|*===----------------------------------------------------------------------===*|
|*
|* This file defines functions shared by the various different profiling
|* implementations.
|*
\*===----------------------------------------------------------------------===*/

#ifndef PARPOT_PROFILING_H
#define PARPOT_PROFILING_H

#include "llvm/Analysis/ProfileInfoTypes.h" /* for enum ProfilingType */

/* save_arguments - Save argc and argv as passed into the program for the file
 * we output.
 */
int save_arguments(int argc, const char **argv);

void write_profiling_data_d(double *Start, unsigned NumElements);

#endif
