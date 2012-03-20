//=== ParPot.cpp - Parallelization potential measurement - Implementation -===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines
//
//===----------------------------------------------------------------------===//

#include "Analysis/ParPot.h"
#include "DebugInfo/DebugInfo.h"
#include "DebugInfo/DebugInfoReader.h"
#include "DebugInfo/CompileUnit.h"
#include "DebugInfo/Subprogram.h"
#include "DebugInfo/Allocation.h"
#include "DebugInfo/GlobalVar.h"

#include "llvm/Function.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Metadata.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

const std::string ParPot::Separator =
"  -------------------------------------------------------------------------\n";

// Register this pass...
char ParPot::ID = 0;
static RegisterPass<ParPot>
X("parpot-analysis", "Parallelization potential measurement", false, true);

bool ParPot::runOnModule(Module &M) {

		// instantiate analysis tool
		errs() << "1. Prepare analysis context\n";
		ctx_ = new AnalysisContext(&M, &getAnalysis<DynCallGraphParserPass>(),
															 &getAnalysis<CallGraph>(),
															 &getAnalysis<EquivBUDataStructures>(),
															 &getAnalysis<BUDataStructures>());

    // prepare analysis
		pointerAnalysis = new PointerAnalysis(ctx_);
    corrAnalysis = new CorrelationAnalysis(ctx_);
    globalsAnalysis = new GlobalsAnalysis(ctx_);

    // iterate through static callgraph and insert missing functions from
    // dynamic callgraph where applicable
    CallGraphNode *root = ctx_->getGC()->getRoot();

    errs() << "2. Analyze dependencies\n";
    analyzeDependencies(root->getFunction());
		errs() << "3. Collect node sets\n";
    collectNodeSets(root->getFunction());


    // sort function sets
		std::sort(nodeSetVec_.begin(), nodeSetVec_.end(), DGNodeSet::compare);

    return false;
}

void ParPot::analyzeDependencies(Function *parent) {

  // check if function has already been analyzed
  DGNodeSet::DepGraphMapTy::const_iterator gIt =
																					ctx_->getDepGraphs()->find(parent);
  if (gIt != ctx_->getDepGraphs()->end())
    return;

  // create dependency graph for this node
  DepGraph *graph = new DepGraph(parent);
  (*ctx_->getDepGraphs())[parent] = graph;

  typedef std::pair<DepGraphNode*, Function*> NodeTy;
  typedef std::vector<NodeTy> DGVectTy;
  DGVectTy nodes;

  // consider every pair of a callsite for function A and a callsite
  // for function B to check dependencies
  for (inst_iterator it = inst_begin(parent), e = inst_end(parent);
      it != e; ++it)
    if (isa<CallInst>(&*it) || isa<InvokeInst>(&*it)) {
      CallSite cs(&*it);
      Function *tmp = ctx_->getFunctionPtr(cs);
      if (!tmp) continue;
      DepGraphNode *node = graph->getNode(&*it, /*create if missing*/ true);
      nodes.push_back(std::make_pair(node, tmp));
    }


  for (DGVectTy::iterator iF = nodes.begin(), eF = nodes.end(); iF!=eF; iF++) {
    for (DGVectTy::reverse_iterator iR = nodes.rbegin();
          iF->first != iR->first; ++iR) {

      /*
       * analyze def-use correlations
       */
      corrAnalysis->analyze(parent, iF->first->getInstruction(),
																		iR->first->getInstruction());

      /*
       * use alias analysis information to insert possible dependencies
       */
      pointerAnalysis->analyze(parent, iF->first->getInstruction(),
																			 iR->first->getInstruction());

      /*
       * analyze use of global variables
       */
      globalsAnalysis->analyze(parent, iF->first->getInstruction(),
																			 iR->first->getInstruction());
    }

    // analyze child node
    analyzeDependencies(iF->second);
  }
}
void ParPot::printDepType(raw_ostream &out, unsigned char type) const {

  if (type & TrueDependence)
    out << "True dependence ";
  if (type & AntiDependence)
    out << "Anti dependence ";
  if (type & OutputDependence)
    out << "Output dependence ";
  if (type & ControlDependence)
    out << "Control dependence ";
  if (type & NoDominateDependence)
    out << "No dominator dependence ";
}

void ParPot::print(raw_ostream &out, const Module *M) const {

  DebugInfoReader reader(DebugInfo::getFileName(), *const_cast<Module*>(M));


  out << " maintime: " << ctx_->getDCG()->getTotExecutionTime() << '\n';


  out << "Dependence Analysis Result: \n";


  // consider all function sets
  for (DGNodeSet::NodeSetVecTy::const_iterator iSet = nodeSetVec_.begin(),
      e1 = nodeSetVec_.end(); iSet != e1; ++iSet) {

    // check timing
    double minPerc=((*iSet)->getMinSaving() /
												ctx_->getDCG()->getTotExecutionTime())*100;
    double maxPerc=((*iSet)->getMaxSaving() /
												ctx_->getDCG()->getTotExecutionTime())*100;

    std::stringstream ss;
    std::string sMinPerc, sMaxPerc;
    ss << minPerc;
    sMinPerc = ss.str();
    ss << maxPerc;
    sMaxPerc = ss.str();

    if (maxPerc < 1)
      continue;

    std::vector<Subprogram*>::const_iterator iS;
    std::vector<Allocation*>::const_iterator iA;
    std::vector<CallInstruction*>::const_iterator iC;

    // dump name of parent function
    out << "  Parent-function: ";
    if (reader.getSubprogram((*iSet)->getGraph()->getFunction()->getName(), iS)) {
      out << (*iS)->getDisplayName() << " ("
          << (*iS)->getCompileUnit() << ")\n";
    }
    else
      out << "IR<" << (*iSet)->getGraph()->getFunction()->getName() << ">";

    // dump function names of set
    out << "  Functions: ( ";
    for (DGNodeSet::DGNodeVecTy::const_iterator iNode = (*iSet)->begin(),
							eNode = (*iSet)->end(); iNode != eNode; iNode++) {
      CallSite cs((*iNode)->getInstruction());
      Function *tmp = ctx_->getFunctionPtr(cs);
      assert(tmp && "Function wasn't found in dependence graph!\n");
      if (reader.getSubprogram(tmp->getName(), iS))
        out << (*iS)->getDisplayName();
      else
        out << "IR<" << tmp->getName();

      // dump line number of instruction
      if (reader.getCallInstruction((*iNode)->getInstruction(), iC)) {
      	if (tmp->getName() == "_Z12plate_detectP5img_t") {
      		errs() << *(*iNode)->getInstruction() << " . " << *(*iC)->getInstruction() << "\n";
      	}
      	out << "(" << (*iC)->getFile() << ", " << (*iC)->getLineNo() << ") ";
      }
    }

    // dump savings
    if (minPerc == maxPerc)
      out << " )     saving: [ " << sMinPerc << " % ]\n";
    else
      out << " )     saving: [" << sMinPerc << " % - " << sMaxPerc << " % ]\n";

    // consider every instruction that has dependencies
    for (DGNodeSet::DepSetTy::iterator iDep = (*iSet)->dep_begin(),
							eDep = (*iSet)->dep_end(); iDep != eDep; iDep++) {
    	std::string sFgn;
    	Function *fFgn = ctx_->getFunctionPtr(
												CallSite((*iDep)->getToOrFromNode()->getInstruction()));
      if (reader.getSubprogram(fFgn->getName(), iS))
        sFgn = (*iS)->getDisplayName();
      else {
        sFgn = "IR<";
        sFgn += fFgn->getName();
        sFgn += "> ";
      }
      out << "    " << sFgn << ": ";
      out << (*iDep)->getOwnObj() << " -> " << (*iDep)->getFgnObj() << " - ";
      printDepType(out, (*iDep)->getDepType());
      out << '\n';
    }
    out << Separator;
  }
}

void ParPot::collectNodeSets(Function *parent) {

  // visit each function only once
  static std::set<Function *> visitedFuncs;
  if (visitedFuncs.find(parent) != visitedFuncs.end())
    return;
  visitedFuncs.insert(parent);

  typedef std::pair<DepGraphNode*, Function*> NodeTy;
  typedef std::vector<NodeTy> DGVectTy;
  DGVectTy nodes;
  DepGraph *graph = (*ctx_->getDepGraphs())[parent];

  // consider every pair of a callsite for function A and a callsite
  // for function B to check dependencies
  for (inst_iterator it = inst_begin(parent), e = inst_end(parent);
      it != e; ++it) {
    if (isa<CallInst>(&*it) || isa<InvokeInst>(&*it)) {
      CallSite cs(&*it);
      Function *tmp = ctx_->getFunctionPtr(cs);
      if (!tmp) continue;
      DepGraphNode *node = graph->getNode(&*it, /*create if missing*/ true);
      nodes.push_back(std::make_pair(node, tmp));
    }
  }

  // create node-sets
  for (DGVectTy::iterator iF = nodes.begin(), eF = nodes.end(); iF!=eF; ++iF) {
    for (DGVectTy::reverse_iterator iR = nodes.rbegin();
          *iF->first != *iR->first; ++iR) {
      DGNodeSet::DGNodeVecTy tmp;
      tmp.push_back(iF->first);
      tmp.push_back(iR->first);
      nodeSetVec_.push_back(new DGNodeSet(*graph, tmp, *ctx_));
    }
    collectNodeSets(iF->second);
  }
}
