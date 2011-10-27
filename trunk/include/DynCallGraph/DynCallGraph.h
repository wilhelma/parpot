//===---- DynCallGraph/DynCallGraph.h - Dynamic callgraph - Interface -----===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the class DynCallGraphNode and DynCallGraph. They are used
// to represent a control-flow-graph of a specific execution run of an
// application.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DYNCALLGRAPH_DYNCALLGRAPH_H_
#define PARPOT_DYNCALLGRAPH_DYNCALLGRAPH_H_

#include "llvm/Function.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/ValueHandle.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/Support/DOTGraphTraits.h"
#include "llvm/Instruction.h"
#include <map>
#include <vector>
#include <sstream>

namespace llvm {

/// The class DynCallGraphNode represents a function node of the dynamic call
/// graph. It holds the ID as well as the corresponding name of the function
/// node.
class DynCallGraphNode {
  unsigned nodeID_;
  std::string name_;
  unsigned num_;
  Instruction *pInstruction_;
  double exTime_;

  DynCallGraphNode(const DynCallGraphNode&);  // DO NOT IMPLEMENT
  void operator=(const DynCallGraphNode&);    // DO NOT IMPLEMENT

public:
  typedef std::pair<DynCallGraphNode*, unsigned> calledFunctionTy;

private:
  std::vector<calledFunctionTy> calledFunctions_;

public:
  DynCallGraphNode(unsigned id, std::string name, unsigned num, double exTime)
    : nodeID_(id), name_(name), num_(num), exTime_(exTime) { }

  //===---------------------------------------------------------------------
  // Accessor methods.
  //
  typedef std::vector<calledFunctionTy>::iterator iterator;
  typedef std::vector<calledFunctionTy>::const_iterator const_iterator;

  inline iterator begin() { return calledFunctions_.begin(); }
  inline iterator end()   { return calledFunctions_.end();   }
  inline const_iterator begin() const { return calledFunctions_.begin(); }
  inline const_iterator end()   const { return calledFunctions_.end();   }
  inline bool empty() const { return calledFunctions_.empty(); }
  inline unsigned size() const { return (unsigned)calledFunctions_.size(); }

  /// addCalledFunction - Add a function to the list of functions called by this
  /// one.
  void addCalledFunction(DynCallGraphNode *node, unsigned count) {
    calledFunctions_.push_back(std::make_pair(node, count));
  }

  /// return the name of the function that this call graph node represents.
  std::string getName() const { return name_; }

  /// return the execution time
  double getExTime() const { return exTime_; }

  void setExTime(double exTime) { exTime_ = exTime; }

  /// return id of this call graph node.
  unsigned int getNum(void) const { return num_; }

  /// set instruction pointer of this node
  void setInstruction(Instruction *inst) { pInstruction_ = inst; }

  /// get instruction pointer of this node
  Instruction *getInstruction(void) { return pInstruction_; }
};

/// The class DynCallGraph represents a control-flow-graph of a specific
/// execution of a application. It contains entities of DynCallGraphNode class
/// for the function nodes.
class DynCallGraph {

  // Root is root of the call graph, or the external node if a 'main' function
  // couldn't be found.
  DynCallGraphNode *pRoot_;

  typedef std::map<unsigned, DynCallGraphNode*> DynFigMapTy;
  DynFigMapTy idMap_;    // Map from a node-id to its node
  DynFigMapTy numMap_;    // Map from the node number to its node

  typedef std::map<Instruction*, DynCallGraphNode*> DynInstMapTy;
  DynInstMapTy instMap_;

  DynCallGraph(const DynCallGraph&);      // DO NOT IMPLEMENT
  void operator=(const DynCallGraph&);    // DO NOT IMPLEMENT

public:
  // the total execution time of the application
  static double totExTime;

  static char ID; // Class identification, replacement for typeinfo
  static const std::string FILENAME;
  DynCallGraph(): pRoot_(0){ }
  ~DynCallGraph() {
    for (DynFigMapTy::iterator it = idMap_.begin(), e = idMap_.end();
        it != e; ++it)
      delete it->second;
    idMap_.clear();
   }

  //===---------------------------------------------------------------------
  // Accessors.
  //
  typedef DynFigMapTy::iterator iterator;
  typedef DynFigMapTy::const_iterator const_iterator;

  inline       iterator begin()       { return idMap_.begin(); }
  inline       iterator end()         { return idMap_.end();   }
  inline const_iterator begin() const { return idMap_.begin(); }
  inline const_iterator end()   const { return idMap_.end();   }


  typedef DynInstMapTy::iterator instr_iterator;
  inline       instr_iterator instr_begin()       { return instMap_.begin(); }
  inline       instr_iterator instr_end()         { return instMap_.end();   }

  // addNode - adds a function with a given ID to the callgraph.
  //
  DynCallGraphNode* addNode(unsigned nodeID, std::string node,
                            unsigned num, double exTime);

  // addEdge - adds an directed edge between two nodes.
  bool addEdge(unsigned parentID, unsigned nodeID, unsigned count);

  /// get root node
  DynCallGraphNode* getRoot() const { return pRoot_; }

  /// create a link between a node and an instruction
  bool linkInstruction(unsigned num, Instruction* inst);

  /// resolve the name of the concrete function of a given call(inv) instruction
  bool getConcreteName(Instruction*, std::string&) const;

  /// get execution time of given call- (invoke-) instruction
  double getExecutionTime(Instruction*) const;

  /// return the total execution time of the application
  static double getTotExecutionTime(void) { return totExTime; }
};
}

#endif
