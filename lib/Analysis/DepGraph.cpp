//=== DepGraph.cpp - Dependence graph - Implementation -===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines
//
//===----------------------------------------------------------------------===//

#include "Analysis/DepGraph.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

void DepGraph::addDependence(Instruction *iA, Instruction *iB,
      DependenceType depType, std::string ownObj, std::string fgnObj)  {
  DepGraphNode *fromNode = getNode(iA, true);
  DepGraphNode *toNode = getNode(iB, true);

  fromNode->addDependency(new Dependence(toNode, depType,
      ownObj, fgnObj, false));
  toNode->addDependency(new Dependence(fromNode, depType,
      fgnObj, ownObj, true));
}

const DepGraphNode* DepGraph::getNode(const Instruction *cs) const  {
  return const_cast<DepGraph *>(this)->getNode(
      const_cast<Instruction *>(cs));
}

DepGraphNode* DepGraph::getNode(Instruction *cs, bool createIfMissing) {
    iterator it = depNodeMap_.find(cs);
    if (it == depNodeMap_.end()) {
      if (createIfMissing) {
        depNodeMap_[cs] =  new DepGraphNode(cs);
        return depNodeMap_[cs];
      }
      else
        return NULL;
    }

    return it->second;
  }
