//=== Analysis/Parpot.h - Parallelization potential measurement - Interface===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the ParPot class
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_PARPOT_H
#define PARPOT_PARPOT_H

#include "llvm/Analysis/Passes.h"
#include "DynCallGraph/DynCallGraph.h"
#include "llvm/Analysis/CallGraph.h"
#include "Analysis/TimeProfileInfo.h"
#include "llvm/Value.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "Analysis/DepGraph.h"
#include "llvm/Module.h"
#include "Analysis/CountStoresPass.h"
#include "Analysis/TimeProfileInfoLoader.h"
#include "DynCallGraph/DynCallGraphParser.h"
#include "llvm/Analysis/Dominators.h"
#include <DataStructure.h>
#include <DataStructureAA.h>
#include <DSGraph.h>
#include <math.h>
#include <map>
#include <queue>
#include <vector>

enum ArgModRefResult {
	NoModRef = 0x0,
	Ref			 = 0x1,
	Mod			 = 0x2,
	ModRef 	 = 0x3
};

ArgModRefResult& operator|=(ArgModRefResult &lhs, ArgModRefResult rhs);

namespace llvm {
class ParPot;

class DGNodeSet {
  DGNodeSet(const DGNodeSet&);            // DO NOT IMPLEMENT
  bool operator= (const DGNodeSet&) const;  // DO NOT IMPLEMENT

public:
  // constants
  static const int TRUE_FACTOR = 2;
  static const int ANTI_FACTOR = 1;
  static const int OUT_FACTOR = 1;
  static const int CNT_FACTOR = 2;
  static const int DOM_FACTOR = 2;

  // types
  typedef std::vector<DepGraphNode *> DGNodeVecTy;
  typedef std::set<Dependence *> DepSetTy;

private:
  // members
  const DepGraph *graph_;
  DGNodeVecTy nodes_;
  DepSetTy deps_;

  double minSaving_, maxSaving_;
  unsigned trueDeps_, antiDeps_, outDeps_, cntDeps_, domDeps_;

  // helper-methods
  bool findDeps(const DepGraph &graph, DepGraphNode *src, DepGraphNode *dst,
                bool init = false);
public:

  DGNodeSet(const DepGraph &graph, const DGNodeVecTy &dSet, const ParPot &pp);

  typedef DGNodeVecTy::iterator									iterator;
  typedef DGNodeVecTy::const_iterator			const_iterator;
  typedef DepSetTy::iterator                dep_iterator;
  typedef DepSetTy::const_iterator    dep_const_iterator;

  iterator        		begin()       		{ return nodes_.begin(); }
  iterator        		end()         		{ return nodes_.end(); }
  const_iterator  		begin() const 		{ return nodes_.begin(); }
  const_iterator  		end() const   		{ return nodes_.end(); }
  dep_iterator        dep_begin()       { return deps_.begin(); }
  dep_iterator        dep_end()         { return deps_.end(); }
  dep_const_iterator  dep_begin() const { return deps_.begin(); }
  dep_const_iterator  dep_end() const   { return deps_.end(); }

  double getMinSaving() const { return minSaving_; }
  double getMaxSaving() const { return maxSaving_; }

  const DepGraph* getGraph(void) const { return graph_; }

  static bool compare(const DGNodeSet*, const DGNodeSet*);
};

/// ParPot class -
class ParPot: public ModulePass {
public:
  typedef std::vector<DGNodeSet *> NodeSetVecTy;
  NodeSetVecTy nodeSetVec_;

private:
  ParPot(const ParPot&);            // DO NOT IMPLEMENT
  ParPot& operator=(const ParPot*); // DO NOT IMPLEMENT

  enum AccTy { // access type
    Read            = 0x0,
    Change          = 0x1
  };
  typedef std::map<GlobalVariable*, AccTy> GlobAccMapTy; // access type per gv
  typedef std::map<Function*, GlobAccMapTy> FuncGlobalsMapTy; // globals per fn
  typedef std::map<Function*, DepGraph*> DepGraphMapTy; // dep. graph

  // constants
  static const std::string NoObj;
  static const std::string Separator;

  // members
  Module *pMod_;
  DynCallGraph *pDCG_;
  CallGraph *pCG_;
  EquivBUDataStructures *pDSA_;
  DSAA *pAA_;
  BUDataStructures *pBU_;

  DepGraphMapTy fDepGraphs_;
  FuncGlobalsMapTy fGlobs_;

  /// analyze dependence graph of a given ParPotNode and all children
  void analyzeDependencies(Function*);

  /// check if the definition of a value reaches an instruction (recursively).
  bool checkDefUse(Value*, Instruction*, int, bool, bool);

  /// check if the definition of a value is used within a branch (recursively).
  bool checkBranchDefUse(BranchInst*, Instruction*, int, int, bool);

  /// check every dependence (read or write) of a given function to globals
  void checkGlobalDependencies(Function *func);

  /// analyze whether function call A correlates with function call B.
  void analyzeCorrelation(Function*, Instruction*, Instruction*,
      bool invert = false);

  /// use alias analysis to find mutual references of function call A and B.
  void analyzeAliasAnalysis(Function*, Instruction*, Instruction*);

  /// check dependencies among functions due to global variables
  void analyzeUseOfGlobals(Function*, Instruction*, Instruction*);

  /// check if the instructions are on different paths in the cfg
  void analyzeDominatorPath(Function *, Instruction*, Instruction*);

  /// collect every set of nodes which are at the same level (has the same
  /// parent)
  void collectNodeSets(Function *parent);

  /// helper function to print a dependency type string
  void printDepType(raw_ostream&, unsigned char type) const;

  /// analyzes whether and how an argument is accessed by a function
  ArgModRefResult getModRefForArg(const Function*, Argument*) const;

  /// analyzes how the instruction accesses the given value
  ArgModRefResult getModRefForInst(Instruction *I, const Value *pArg) const;

  /// returns the mod/ref behavior of a callsite concerning a specific argument
  ArgModRefResult getModRefForDSNode(const CallSite&,
																		 const DSNodeHandle&,
																		 const DSGraph*) const;

public:
  static char ID; // Class identification, replacement for typeinfo
  ParPot() : ModulePass(ID) { }

  ~ParPot() {
    for (DepGraphMapTy::iterator it = fDepGraphs_.begin(),
          e = fDepGraphs_.end(); it != e; ++ it)
      delete it->second;
  }

  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
      AU.addRequired<CallGraph>();
      AU.addRequired<EquivBUDataStructures>();
      AU.addRequired<CountStoresPass>();
      AU.addRequired<DynCallGraphParserPass>();
      AU.addRequired<DominatorTree>();
      AU.addRequired<DSAA>();
      AU.addRequired<BUDataStructures>();
 }

  /// dump result of the dependency analysis into the given stream.
  virtual void print(raw_ostream &O, const Module *M) const;

  /// returns a function pointer tied to the given callsite. If a dynamic
  /// function is called, the pointer to the concrete function may be returned
  /// if possible. Returns false if this isn't possible.
  Function* getFunctionPtr(const CallSite&) const;

  DynCallGraph* getDCG(void) const { return pDCG_; }
  CallGraph* getGC(void) const { return pCG_; }
  EquivBUDataStructures* getDSA(void) const { return pDSA_; }
  DSAA* getAA(void) const { return pAA_; }
  BUDataStructures* getBU(void) const { return pBU_; }
};
}

#endif
