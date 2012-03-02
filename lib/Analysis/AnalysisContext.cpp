//============== AliasAnalysis.cpp - Analysis - Implementation -==============//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the analyis class.
//
//===----------------------------------------------------------------------===//

#include "Analysis/AnalysisContext.h"

using namespace llvm;

ArgModRefResult& operator|=(ArgModRefResult &lhs, ArgModRefResult rhs) {
	lhs = static_cast<ArgModRefResult>((int)lhs | (int)rhs);
	return lhs;
}

Function* AnalysisContext::getFunctionPtr(const CallSite &cs) const {

  Function *f = cs.getCalledFunction();
  if (f && f->isDeclaration())
    return NULL;
  else if (f)
    return f;

  std::string concrete;
  if (!pDCG_->getConcreteName(cs.getInstruction(), concrete))
    return NULL; // no function ptr. found

  CallGraphNode *cgNode = pCG_->getExternalCallingNode();
  for (CallGraphNode::iterator iCGN = cgNode->begin(),
        eCGN = cgNode->end(); iCGN != eCGN; ++iCGN) {
    if (iCGN->second->getFunction()->getNameStr() == concrete) {
      return iCGN->second->getFunction();
    }
  }

  return NULL; // no function ptr. found
}
