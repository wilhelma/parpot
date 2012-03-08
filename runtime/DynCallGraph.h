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

#ifndef DYNCALLGRAPH_H
#define DYNCALLGRAPH_H

/* save_arguments - Save argc and argv as passed into the program for the file
 * we output.
 */
int save_dyn_arguments(int argc, const char **argv);

/*
 * Inform the system about a called function.
 */
void llvm_function_called(char* fnName, unsigned fnNum);

/*
 * Inform the system about a call instruction.
 */
void llvm_call_instruction(char* callOp, unsigned ownFnNum);

/*
 * Inform the system about the finishing of a function call.
 */
void llvm_call_finished_instruction(char* callOp, unsigned ownFnNum);

/*
 * Build a dynamic callgraph from the collected calling information.
 */
void llvm_build_and_write_dyncallgraph(int argc, const char **argv);

/*
 * A dummy call in order to measure the overhead for procedure calls.
 */
void llvm_dummy_call(int, int, int);

/*
 * Indicates a start of the overhead measurment.
 */
void llvm_ovhd_start(void);

/*
 * Indicates a stop of the overhead measurment with the used number of dummy.
 * calls.
 */
void llvm_ovhd_stop(unsigned loopSize);

/*
 * Indicates a start of the loop overhead measurment.
 */
void llvm_loop_start(void);

/*
 * Indicates a stop of the loop overhead measurment.
 */
void llvm_loop_stop(void);

#endif
