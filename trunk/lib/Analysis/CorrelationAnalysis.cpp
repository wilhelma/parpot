//======== AliasAnalysis.cpp - Correlation Analysis - Implementation -========//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the correlation analyis.
//
//===----------------------------------------------------------------------===//

#include "Analysis/CorrelationAnalysis.h"

#include "llvm/Support/InstIterator.h"

void CorrelationAnalysis::analyze(Function* parent,
																Instruction* iA,
																Instruction* iB) {

  // check instructions
  assert((isa<CallInst>(iA) || isa<InvokeInst>(iA)) &&
         (isa<CallInst>(iA) || isa<InvokeInst>(iA)) &&
         "Error, instructions are no function calls!");

  DGNodeSet::DepGraphMapTy::iterator dgIt = ctx_->getDepGraphs()->find(parent);
  Function *fA = ctx_->getFunctionPtr(CallSite(iA));

  if (fA) {
    // consider pointer/reference parameters
  	Function::arg_iterator it = fA->arg_begin(), e = fA->arg_end();
  	for (; it != e; ++it) {
      if (it->getType()->isPointerTy()) {
      	if (getModRefForArg(fA, &*it) & Mod) {
          Value *arg = iA->getOperand(it->getArgNo());
          if (checkDefUse(arg, &*iB, 0, false, false)) {
            dgIt->second->addDependence(iA, iB, ControlDependence,
                                      arg->getName(), "-");
          }
        }
      }
    }

  	// consider modified globals
  	checkGlobalDependencies(fA);
    GlobAccMapTy &gMapA = (*ctx_->getGlobals())[fA];
    for (GlobAccMapTy::iterator iGA = gMapA.begin(), eGA = gMapA.end();
          iGA != eGA; ++iGA) {
    	if (iGA->second == Change &&
    			checkDefUse(iGA->first, &*iB, 0, false, false)) {
        dgIt->second->addDependence(iA, iB, ControlDependence,
                                  iGA->first->getName(), "-");
      }
    }

		// consider instruction itself (control dependence caused by a return value)
		if (checkDefUse(&*iA, &*iB, 0, true, false)) {
			dgIt->second->addDependence(iA, iB, ControlDependence, NoObj, NoObj);
		}
  }
}
