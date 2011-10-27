/*===--- DynCallGraph.c - Support library for for dynamic call graph ------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source      
|* License. See LICENSE.TXT for details.                                      
|* 
|*===----------------------------------------------------------------------===*|
|* 
|* This file implements the call back routines for the dynamic call graph
|* creation.
|*
\*===----------------------------------------------------------------------===*/

#include "DynCallGraph.h"
#include "DynCallGraphUtils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *SavedArgs = 0;
static unsigned SavedArgsLength = 0;
static const char *OutputFilename = "dyncallgraph.dot";
static fGraphT graph;

/* save_arguments - Save argc and argv as passed into the program for the file
 * we output.
 */
int save_dyn_arguments(int argc, const char **argv) {
  unsigned Length, i;
  if (SavedArgs || !argv) return argc;  /* This can be called multiple times */

  /* Check to see if there are any arguments passed into the program for the
   * profiler.  If there are, strip them off and remember their settings.
   */
  while (argc > 1 && !strncmp(argv[1], "-llvmdycg-", 10)) {
    /* Ok, we have an llvmprof argument.  Remove it from the arg list and decide
     * what to do with it.
     */
    const char *Arg = argv[1];
    memmove(&argv[1], &argv[2], (argc-1)*sizeof(char*));
    --argc;

    if (!strcmp(Arg, "-llvmdycg-output")) {
      if (argc == 1)
        puts("-llvmdycg-output requires a filename argument!");
      else {
        OutputFilename = strdup(argv[1]);
        memmove(&argv[1], &argv[2], (argc-1)*sizeof(char*));
        --argc;
      }
    } else {
      printf("Unknown option to the profiler runtime: '%s' - ignored.\n", Arg);
    }
  }

  for (Length = 0, i = 0; i != (unsigned)argc; ++i)
    Length += strlen(argv[i])+1;

  SavedArgs = (char*)malloc(Length);
  for (Length = 0, i = 0; i != (unsigned)argc; ++i) {
    unsigned Len = strlen(argv[i]);
    memcpy(SavedArgs+Length, argv[i], Len);
    Length += Len;
    SavedArgs[Length++] = ' ';
  }

  SavedArgsLength = Length;

  return argc;
}

/* EdgeProfAtExitHandler - When the program exits, just write out the callgraph
 * data.
 */
static void CallGraphAtExitHandler() {
  writeGraphToFile(&graph, OutputFilename);
}

void llvm_function_called(char* fnName, unsigned fnNum) {
  if (strcmp(fnName, "main") == 0) // main function => insert new node
    insertNode(&graph, fnName, fnNum);
  else  // other function => change actual name {
    changeCurrentFunctionName(&graph, fnName);
}

void llvm_call_instruction(char* callOp, unsigned ownFnNum) {
  insertNode(&graph, callOp, ownFnNum);
}

void llvm_call_finished_instruction(char* callOp, unsigned ownFnNum) {
  leaveNode(&graph, ownFnNum);
}

void llvm_build_and_write_dyncallgraph(int argc, const char **argv) {
  save_dyn_arguments(argc, argv);
  atexit(CallGraphAtExitHandler);
}
