//===-------- DynCallGraph.cpp - Dynamic callgraph - Implementation -------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the DynCallGraph class.
//
//===---------------------------------------------------------------------
#include "DynCallGraph/DynCallGraph.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

DynCallGraphNode* DynCallGraph::addNode(unsigned nodeID, std::string node,
    unsigned num, double exTime) {

  // check if node is main function
  if (node == "main")
    totExTime = exTime;

  // if a node with given id exist take it, otherwise create a new one
  DynCallGraphNode *pNode = idMap_[nodeID];
  DynCallGraphNode *pNodeNum = numMap_[num];
  if (!pNode) {
    pNode = new DynCallGraphNode(nodeID, node, num, exTime);
    idMap_[nodeID] = pNode;
    if (!pNodeNum)
      numMap_[num] = pNode;
    else if (pNodeNum->getExTime() < exTime)
      pNodeNum->setExTime(exTime);
  }

  // set root node if it's the main function
  if (node == "main")
    pRoot_ = pNode;

  return pNode;
}

bool DynCallGraph::addEdge(unsigned parentID, unsigned nodeID, unsigned count) {
  // check if both nodes exist
  DynCallGraphNode *pNode = idMap_[nodeID], *pParent = idMap_[parentID];
  if (!(pNode && pParent))
    return false;

  // add called function to parent node
  pParent->addCalledFunction(pNode, count);

  // increment number of total calls by count
  incTotNoCalls(count);
  return true;
}

bool DynCallGraph::linkInstruction(unsigned num, Instruction* inst) {
  DynCallGraph::iterator node = numMap_.find(num);
  if (node == numMap_.end())
    return false;

  node->second->setInstruction(inst); // link node
  instMap_[inst] = node->second;
  return true;
}

bool DynCallGraph::getConcreteName(Instruction *inst, std::string &name) const {
  DynInstMapTy::const_iterator node = instMap_.find(inst);
  if (node == instMap_.end())
    return false;
  name = node->second->getName();
  return true;
}

double DynCallGraph::getExecutionTime(Instruction *inst) const {
  DynInstMapTy::const_iterator node = instMap_.find(inst);
  if (node == instMap_.end())
    return 0.0;
  else
    return node->second->getExTime();
}

char DynCallGraph::ID = 0;
const std::string DynCallGraph::FILENAME = "dyncallgraph.dot";
double DynCallGraph::totExTime = 0.0;
unsigned DynCallGraph::totNoCalls = 0;
