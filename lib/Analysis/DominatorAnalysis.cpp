//======== GlobalsAnalysis.cpp - Globals Analysis - Implementation -========//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the globals analyis.
//
//===----------------------------------------------------------------------===//

#include "Analysis/DominatorAnalysis.h"

#include "llvm/Analysis/Dominators.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Function.h"

using namespace llvm;

class LoaderInterface : public ModulePass {
	DominatorTree *DT_;
	Function* f_;
public:
	static char ID; // Class identification, replacement for typeinfo
	explicit LoaderInterface(Function* f) : ModulePass(ID), f_(f) {}

	virtual void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.setPreservesAll();
		AU.addRequired<DominatorTree>();
	}

	DominatorTree* getDT() {
		return DT_;
	}

	bool runOnModule(Module &M) {
		DT_ = &getAnalysis<DominatorTree>();
		return false;
	}
};

char LoaderInterface::ID = 0;

void DominatorAnalysis::analyze(Function *parent,
																Instruction *iA, Instruction *iB) {
  // get dominator tree for function
  PassManager PassMgr = PassManager();
  PassMgr.add(createProfileLoaderPass());
  LoaderInterface *LI = new LoaderInterface(parent);
  PassMgr.add(LI);
  PassMgr.run(*ctx_->getMod());
  DominatorTree *dT = LI->getDT();

  Instruction *first, *second;
  if (iA < iB) { first = iA; second = iB; }
  else { first = iB; second = iA; }

  if (!dT->dominates(first, second)){

    // get dependence graph
    DGNodeSet::DepGraphMapTy::iterator dgIt= ctx_->getDepGraphs()->find(parent);
    assert(dgIt != ctx_->getDepGraphs()->end()
           && "No dependence graph found for function");

    // add dependence
    dgIt->second->addDependence(iA, iB, NoDominateDependence, NoObj, NoObj);
  }
}
