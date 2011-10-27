//===- ProfileInfoLoaderPass.cpp - LLVM Pass to load profile info ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a concrete implementation of profiling information that
// loads the information from a profile dump file.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "timeprofile-loader"
#include "llvm/BasicBlock.h"
#include "llvm/InstrTypes.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/Passes.h"
#include "Analysis/TimeProfileInfo.h"
#include "Analysis/TimeProfileInfoLoader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Format.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SmallSet.h"
#include <set>
using namespace llvm;

char TimeLoaderPass::ID = 0;
static RegisterPass<TimeLoaderPass>
X("parpot-timeprofile-loader", "Load profile information from timellvmprof.out",
    false, true);

bool TimeLoaderPass::runOnModule(Module &M) {
  // get timeprofile- and profile-information from files
  TimeProfileInfoLoader PIL("timeprofile-loader", Filename, M);
  ProfileInfo *pi = getAnalysisIfAvailable<ProfileInfo>();

  // assign execution time information from file (average execution times are
  // calculated by using the execution count)
  FunctionTimeInformation.clear();
  std::vector<double> FuncTimes = PIL.getRawFunctionTimes();
  if (FuncTimes.size() > 0) {
    ReadCount = 0;
    for (Module::iterator it = M.begin(), e = M.end(); it != e; ++it) {
      if (it->isDeclaration()) continue;
      if (ReadCount < FuncTimes.size()) {
        int eC = pi->getExecutionCount(it);
        if (eC == ProfileInfo::MissingValue) eC = 1;
        setExecutionTime(it, FuncTimes[ReadCount++] / eC);
      }
    }
    if (ReadCount != FuncTimes.size()) {
      errs() << "WARNING: profile information is inconsistent with "
               << "the current program!\n";
    }
  }


  return false;
}

void TimeLoaderPass::print(raw_ostream &O, const Module *M) const {

  O << "Time-profile information: \n";

  for (std::map<const Function*, double>::const_iterator
        it = FunctionTimeInformation.begin(),
        e = FunctionTimeInformation.end(); it != e; ++it) {
    O << "  Function: " << it->first->getName()
      << " Time: " << it->second
      << '\n';
  }

}

