//===- TimeProf.cpp - Build a callgraph with time profiling information ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the CallGraph class and provides the TimeProf
// implementation.
//
//===----------------------------------------------------------------------===//

#include "Analysis/PriorizeHandler.h"
#include "Analysis/TimeProfileInfo.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

double TimeProfileInfo::totExTime = 0;
double TimeProfileInfo::minExTime = 0;
double TimeProfileInfo::maxExTime = 0;

void TimeProfileInfo::setExecutionTime(const Function* f, const double &time) {

  // update static values
  std::map<const Function*, double>::iterator J =
      FunctionTimeInformation.find(f);

  if (f->getName() == "main") {
    TimeProfileInfo::totExTime = time;
    return;
  } else {
    if (time < TimeProfileInfo::minExTime || TimeProfileInfo::minExTime == 0)
      TimeProfileInfo::minExTime = time;
    if (time > TimeProfileInfo::maxExTime || TimeProfileInfo::maxExTime == 0)
      TimeProfileInfo::maxExTime = time;
  }

  // set timing for function
  FunctionTimeInformation[f] = time;

  // reset (initialize) priority information
  PrioInformation[f] = 0;
}

void TimeProfileInfo::setPrioInformation(const Function* f, const int &prio) {
  PrioInformation[f] = prio;
}

std::vector<TimeProfileInfo::Priority> TimeProfileInfo::getPriorityList() const {
  std::vector<TimeProfileInfo::Priority> v;
  for (std::map<const Function*, int>::const_iterator it = PrioInformation.begin();
      it != PrioInformation.end(); ++it) {
    v.push_back(*it);
  }

  std::sort(v.begin(), v.end(), TimePrioComp());
  return v;
}
