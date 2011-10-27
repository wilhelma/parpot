//===--------- Analysis/DepGraph - Dependency Graph - Interfaces ----------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the dependency graph and its nodes.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_ANALYSIS_DEPGRAPH_H
#define PARPOT_ANALYSIS_DEPGRAPH_H

#include "llvm/Instruction.h"
#include <map>
#include <vector>

namespace llvm {

class DepGraphNode;

/// dependence type
enum DependenceType {
  NoDependence            = 0x0,
  TrueDependence          = 0x1,
  AntiDependence          = 0x2,
  OutputDependence        = 0x4,
  ControlDependence       = 0x8,
  NoDominateDependence    = 0x10,
  IncomingFlag            = 0x20
};

/// dependence class
class Dependence {
  DepGraphNode *pNode_;
  unsigned char depType_;
  std::string ownObj_, fgnObj_;

public:
  Dependence(DepGraphNode *node, DependenceType type, std::string ownObj,
      std::string fgnObj, bool isIncoming) : pNode_(node),
                    depType_(type | (isIncoming ? IncomingFlag : NoDependence)),
                    ownObj_(ownObj),
                    fgnObj_(fgnObj) { }

  Dependence(const Dependence &D) : pNode_(D.pNode_), depType_(D.depType_) { }

  bool operator==(const Dependence &D) const {
    return (pNode_ == D.pNode_
							&& depType_ == D.depType_
							&& ownObj_ == D.ownObj_
							&& fgnObj_ == D.fgnObj_);
  }

  unsigned char getDepType(void) {
    return depType_;
  }

  DepGraphNode* getToOrFromNode() const {
    return pNode_;
  }

  std::string getOwnObj() const {
    return ownObj_;
  }

  std::string getFgnObj() const {
    return fgnObj_;
  }
};

/// DepGraphNode class
class DepGraphNode {
  std::vector<Dependence *> incoming_;
  std::vector<Dependence *> outgoing_;
  Instruction *pInstruction_;

  DepGraphNode(const DepGraphNode&);            // DO NOT IMPLEMENT
  DepGraphNode& operator=(const DepGraphNode*); // DO NOT IMPLEMENT

public:
  DepGraphNode(Instruction *instruction) : pInstruction_(instruction) { }

  bool operator==(const DepGraphNode &rhs) const {
    return (pInstruction_ == rhs.pInstruction_);
  }

  bool operator!=(const DepGraphNode &rhs) const {
    return (pInstruction_ != rhs.pInstruction_);
  }

  typedef std::vector<Dependence *>::iterator             iterator;
  typedef std::vector<Dependence *>::const_iterator const_iterator;

        iterator inDepBegin()       { return incoming_.begin(); }
  const_iterator inDepBegin() const { return incoming_.begin(); }
        iterator inDepEnd()         { return incoming_.end(); }
  const_iterator inDepEnd()   const { return incoming_.end(); }

        iterator outDepBegin()       { return outgoing_.begin(); }
  const_iterator outDepBegin() const { return outgoing_.begin(); }
        iterator outDepEnd()         { return outgoing_.end(); }
  const_iterator outDepEnd()   const { return outgoing_.end(); }

  void addDependency(Dependence *dep) {
    if (dep->getDepType() & IncomingFlag)
      incoming_.push_back(dep);
    else
      outgoing_.push_back(dep);
  }

  Instruction* getInstruction(void) { return pInstruction_; }
};

/// DepGraph class
class DepGraph {
  DepGraph(const DepGraph&);            // DO NOT IMPLEMENT
  DepGraph& operator=(const DepGraph*); // DO NOT IMPLEMENT

  typedef std::map<Instruction*, DepGraphNode*> DepNodeMapTy;
  Function *pFunction_;
  DepNodeMapTy depNodeMap_;

public:
  DepGraph(Function *function): pFunction_(function) { }

  ~DepGraph() {
    for (iterator it = depNodeMap_.begin(), e = depNodeMap_.end();
          it != e; ++it)
      delete it->second;
  }

  typedef DepNodeMapTy::iterator                        iterator;
  typedef DepNodeMapTy::const_iterator            const_iterator;

  iterator begin(void) { return depNodeMap_.begin(); }
  iterator end(void) { return depNodeMap_.end(); }
  const_iterator begin(void) const { return depNodeMap_.begin(); }
  const_iterator end(void) const { return depNodeMap_.end(); }

  DepGraphNode* getNode(Instruction *cs, bool createIfMissing = false);
  const DepGraphNode* getNode(const Instruction *cs) const;

  void addDependence(Instruction *iA, Instruction *iB,
      DependenceType depType, std::string ownObj, std::string fgnObj);

  Function* getFunction(void) const { return pFunction_; }
};
}

#endif
