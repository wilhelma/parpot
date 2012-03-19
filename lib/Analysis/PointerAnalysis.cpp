//========= PointerAnalysis.cpp - Alias Analysis - Implementation -===========//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the pointer analyis.
//
//===----------------------------------------------------------------------===//

#include "Analysis/PointerAnalysis.h"

using namespace llvm;

void PointerAnalysis::analyze(Function* parent,
																Instruction* iA,
																Instruction* iB) {

	CallSite csA(iA);
  CallSite csB(iB);
  Function *fA = ctx_->getFunctionPtr(csA);
  Function *fB = ctx_->getFunctionPtr(csB);

  if (!fA || !fB )
  	return;

  typedef std::map<DSNodeHandle, llvm::StringRef> nHMapTy;
  nHMapTy nhA, nhB;
  DSGraph *pDSG = ctx_->getDSA()->getDSGraph(*parent);

  // calculate node handles for call instruction A
	CallSite::arg_iterator iArg = csA.arg_begin(), eArg = csA.arg_end();
	for (int i=0; iArg != eArg; ++iArg, ++i) {
		if (iArg->get()->getType()->isPointerTy()) {
			nhA[pDSG->getNodeForValue(iArg->get())] = iArg->get()->getName();
		}
	}

  // calculate node handles for call instruction B
	iArg = csB.arg_begin(), eArg = csB.arg_end();
	for (int i=0; iArg != eArg; ++iArg, ++i) {
		if (iArg->get()->getType()->isPointerTy()) {
			nhB[pDSG->getNodeForValue(iArg->get())] = iArg->get()->getName();
		}
	}

  // get dependence graph
  DGNodeSet::DepGraphMapTy::const_iterator dgIt =
																						ctx_->getDepGraphs()->find(parent);
  assert(dgIt != ctx_->getDepGraphs()->end()
								&& "No dependence graph found for function");

  // compute uses of both functions
  DSGraph::NodeMapTy nodeMap;
  nHMapTy::iterator iSetA = nhA.begin(), eSetA = nhA.end();
  for (; iSetA != eSetA; ++iSetA) {
  	nHMapTy::iterator iSetB = nhB.begin(), eSetB = nhB.end();
  	for (; iSetB != eSetB; ++iSetB) {

  		//pDSG->computeNodeMapping(iSetA->first, iSetB->first, nodeMap, false);
  		if (iSetA->first.getNode() == iSetB->first.getNode()){//!nodeMap.empty()){
        std::string objA = NoObj, objB = NoObj;
        objA = iSetA->second;
        objB = iSetB->second;

				ArgModRefResult resA = getModRefForDSNode(csA, iSetA->first, pDSG);
				ArgModRefResult resB = getModRefForDSNode(csB, iSetB->first, pDSG);

				// true (and anti-) dependence (A -> B)
				if ((resA & Mod) && (resB & Ref))
					if (iA < iB)
						dgIt->second->addDependence(iA, iB, TrueDependence, objA, objB);
					else
						dgIt->second->addDependence(iB, iA, AntiDependence, objB, objA);
				// true (and anti-) dependence (B -> A)
				else if ((resA & Ref) && (resB & Mod))
					if (iA < iB)
						dgIt->second->addDependence(iA, iB, AntiDependence, objA, objB);
					else
						dgIt->second->addDependence(iB, iA, TrueDependence, objB, objA);

				// output dependence
				else if ((resA & Mod) && (resB & Mod))
					dgIt->second->addDependence(iA, iB, OutputDependence, objA, objB);
  		}
		}
	}
}
