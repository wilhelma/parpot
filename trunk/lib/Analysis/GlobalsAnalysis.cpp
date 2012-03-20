//======== GlobalsAnalysis.cpp - Globals Analysis - Implementation -========//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the globals analyis.
//
//===----------------------------------------------------------------------===//

#include "Analysis/GlobalsAnalysis.h"

void llvm::GlobalsAnalysis::analyze(Function *parent,
                              Instruction *iA, Instruction *iB) {

  // check if instructions are really call-/invoke-instructions
  assert ((isa<CallInst>(iA) || isa<InvokeInst>(iA)) && "Invalid instruction!");
  assert ((isa<CallInst>(iB) || isa<InvokeInst>(iB)) && "Invalid instruction!");

  // get called functions of instruction A and B and check dependencies with
  // global variables
  Function *fA = ctx_->getFunctionPtr(CallSite(iA));
  Function *fB = ctx_->getFunctionPtr(CallSite(iB));
  checkGlobalDependencies(fA);
  checkGlobalDependencies(fB);
  GlobAccMapTy &gMapA = (*ctx_->getGlobals())[fA];
  GlobAccMapTy &gMapB = (*ctx_->getGlobals())[fB];

  // get dependence graph
  DGNodeSet::DepGraphMapTy::iterator dgIt = ctx_->getDepGraphs()->find(parent);
  assert(dgIt != ctx_->getDepGraphs()->end()
						&& "No dependence graph found for function");

  // compare dependence sets
  for (GlobAccMapTy::iterator iGA = gMapA.begin(), eGA = gMapA.end();
        iGA != eGA; ++iGA) {
    GlobAccMapTy::iterator iGB = gMapB.find(iGA->first);
    if (iGB != gMapB.end()) {
      DependenceType dt;
      if (iGA->second == Change && iGB->second == Read)
        if (iA < iB) dt = TrueDependence;
        else dt = AntiDependence;
      else if (iGA->second == Read && iGB->second == Change)
        if (iA < iB) dt = AntiDependence;
        else dt = TrueDependence;
      else if (iGA->second == Change && iGB->second == Change)
        dt = OutputDependence;
      else continue;

      dgIt->second->addDependence(iA, iB, dt,
                        "G:"+iGA->first->getNameStr(), "G:"+iGA->first->getNameStr());
    }
  }
}
