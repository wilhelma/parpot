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
#include "llvm/Function.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Metadata.h"
#include "DebugInfo/DebugInfo.h"
#include "DebugInfo/DebugInfoReader.h"
#include "DebugInfo/CompileUnit.h"
#include "DebugInfo/Subprogram.h"
#include "DebugInfo/Allocation.h"
#include "DebugInfo/GlobalVar.h"
#include "llvm/LLVMContext.h"
#include <set>

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

const std::string ParPot::NoObj = "";
const std::string ParPot::Separator =
"  -------------------------------------------------------------------------\n";

ArgModRefResult& operator|=(ArgModRefResult &lhs, ArgModRefResult rhs) {
	lhs = static_cast<ArgModRefResult>((int)lhs | (int)rhs);
	return lhs;
}

// Register this pass...
char ParPot::ID = 0;
static RegisterPass<ParPot>
X("parpot-analysis", "Parallelization potential measurement", false, true);

bool ParPot::runOnModule(Module &M) {

    // prepare members with passes
    pMod_ = &M;
    pDCG_ = &getAnalysis<DynCallGraphParserPass>();
    pCG_ = &getAnalysis<CallGraph>();
    pDSA_ = &getAnalysis<EquivBUDataStructures>();
    pAA_ = &getAnalysis<DSAA>();
    pBU_ = &getAnalysis<BUDataStructures>();

    // iterate through static callgraph and insert missing functions from
    // dynamic callgraph where applicable
    CallGraphNode *root = pCG_->getRoot();
    analyzeDependencies(root->getFunction());
    collectNodeSets(root->getFunction());

    for (NodeSetVecTy::iterator it = nodeSetVec_.begin(), e = nodeSetVec_.end();
          it != e; ++it)
      errs() << (*it)->getMaxSaving() << '\n';

    return false;
}

Function* ParPot::getFunctionPtr(const CallSite &cs) const {

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

void ParPot::analyzeDependencies(Function *parent) {

  // check if function has already been analyzed
  DepGraphMapTy::iterator gIt = fDepGraphs_.find(parent);
  if (gIt != fDepGraphs_.end())
    return;

  // create dependency graph for this node
  DepGraph *graph = new DepGraph(parent);
  fDepGraphs_[parent] = graph;

  typedef std::pair<DepGraphNode*, Function*> NodeTy;
  typedef std::vector<NodeTy> DGVectTy;
  DGVectTy nodes;

  // consider every pair of a callsite for function A and a callsite
  // for function B to check dependencies
  for (inst_iterator it = inst_begin(parent), e = inst_end(parent);
      it != e; ++it)
    if (isa<CallInst>(&*it) || isa<InvokeInst>(&*it)) {
      CallSite cs(&*it);
      Function *tmp = getFunctionPtr(cs);
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
      analyzeCorrelation(parent, iF->first->getInstruction(),
                                 iR->first->getInstruction(),
                                 /*both orders*/ true);

      /*
       * use alias analysis information to insert possible dependencies
       */
      analyzeAliasAnalysis(parent, iF->first->getInstruction(),
                                   iR->first->getInstruction());

      /*
       * analyze use of global variables
       */
      analyzeUseOfGlobals(parent, iF->first->getInstruction(),
                                  iR->first->getInstruction());
    }

    // analyze child node
    analyzeDependencies(iF->second);
  }
}

bool ParPot::checkDefUse(Value *val, Instruction *iB, int level) {
  static std::set<Value *> visitedVals;
  if (!level)
    visitedVals.clear();

  // ignore alredy visited values
  if (visitedVals.find(val) != visitedVals.end())
    return false;
  visitedVals.insert(val);

  // check if function A is correlated with function B
  if (isa<CallInst>(val) || isa<InvokeInst>(val)) {
     Instruction *inst = dyn_cast<Instruction>(&*val);

     // correlation found
     if (inst == iB)
       return true;
  }

  // consider store instructions
  if (StoreInst *sInst = dyn_cast<StoreInst>(&*val)) {
    if (checkDefUse(&*sInst->getPointerOperand(), iB, level + 1))
      return true;
  }

  // consider branch instructions
  if (BranchInst *bInst = dyn_cast<BranchInst>(&*val)) {
    for (unsigned i=0; i<bInst->getNumSuccessors(); ++i) {
      BasicBlock *bb = bInst->getSuccessor(i);
      for (BasicBlock::iterator it = bb->begin(), e = bb->end();
            it != e; ++it) {

        // consider store instructions within branchens
        if (StoreInst *sInst = dyn_cast<StoreInst>(&*it))
          if (checkDefUse(&*sInst, iB, level + 1))
            return true;

        // consider PHINodes within branches
        if (PHINode *phi = dyn_cast<PHINode>(&*it))
          if (checkDefUse(&*phi, iB, level + 1))
            return true;
      }
    }
  }

  // check every use of the value recursively
  for (Value::use_iterator iUse = val->use_begin(), eUse = val->use_end();
        iUse != eUse; ++iUse) {

    if (checkDefUse(*iUse, iB, level + 1))
      return true;
  }

  return false;
}

void ParPot::analyzeCorrelation(Function *parent,
    Instruction *iA, Instruction *iB, bool invert) {

  // check instructions
  assert((isa<CallInst>(iA) || isa<InvokeInst>(iA)) &&
         (isa<CallInst>(iA) || isa<InvokeInst>(iA)) &&
         "Error, instructions are no function calls!");

  DepGraphMapTy::iterator dgIt = fDepGraphs_.find(parent);
  Function *fA = getFunctionPtr(CallSite(iA));
  if (pDSA_->hasDSGraph(*fA)) {
    DSGraph *dsgA = pDSA_->getDSGraph(*fA);

    // look for pointer/reference parameters
    for (Function::arg_iterator it = fA->arg_begin(), e = fA->arg_end();
          it != e; ++it) {
      if (it->getType()->isPointerTy()) {
        DSNode *node = dsgA->getNodeForValue(&*it).getNode();
        if (node->isModifiedNode()) {
          Value *arg = iA->getOperand(it->getArgNo()+1); //op(0)=function itself
          if (checkDefUse(arg, &*iB, 0))
          	errs() << "   control dep for " << parent->getName() << "\n";
            dgIt->second->addDependence(iA, iB, ControlDependence,
                                      arg->getName(), NoObj);
        }
      }
    }
  }

  // look for use of call instruction itself
  if (checkDefUse(&*iA, &*iB, 0)) {
    dgIt->second->addDependence(iA, iB, ControlDependence, NoObj, NoObj);
  }

  // analyze the inverted order
  if (invert) analyzeCorrelation (parent, iB, iA);
}

void ParPot::analyzeAliasAnalysis(Function *parent,
                                  Instruction *iA, Instruction*iB) {

  CallSite csA(iA);
  CallSite csB(iB);
  Function *fA = getFunctionPtr(csA);
  Function *fB = getFunctionPtr(csB);

  DSGraph *pDSG = pDSA_->getDSGraph(*parent);

  // get DSCallSites and DSGraph for both functions
  DSCallSite dscA = pDSG->getDSCallSiteForCallSite(csA);
  DSCallSite dscB = pDSG->getDSCallSiteForCallSite(csB);
  DSGraph *pDSGA = pDSA_->getDSGraph(*fA);
  DSGraph *pDSGB = pDSA_->getDSGraph(*fB);

  // compute uses of both functions
  DSGraph::NodeMapTy nodeMapA, nodeMapB;
  pDSG->computeCalleeCallerMapping(dscA, *fA, *pDSGA, nodeMapA);
  pDSG->computeCalleeCallerMapping(dscB, *fB, *pDSGB, nodeMapB);

  // get dependence graph
  DepGraphMapTy::iterator dgIt = fDepGraphs_.find(parent);
  assert(dgIt != fDepGraphs_.end() && "No dependence graph found for function");

  // get scalar maps
  DSGraph::ScalarMapTy &mapA = pDSGA->getScalarMap();
  DSGraph::ScalarMapTy &mapB = pDSGB->getScalarMap();

  // compare pointer arguments
  for (DSGraph::NodeMapTy::iterator itA = nodeMapA.begin(),
        eA = nodeMapA.end(); itA != eA; ++itA)
    for (DSGraph::NodeMapTy::iterator itB = nodeMapB.begin(),
          eB = nodeMapB.end(); itB != eB; ++itB)
      if (itA->second == itB->second) {

        // get object names
        std::string objA = NoObj, objB = NoObj;
        for (DSGraph::ScalarMapTy::iterator it = mapA.begin(), e = mapA.end();
              it != e; ++it) {
          if (it->second.getNode() == itA->first && objA == NoObj) {
            objA = it->first->getName();
          }
        }

        for (DSGraph::ScalarMapTy::iterator it = mapB.begin(), e = mapB.end();
              it != e && objB == NoObj; ++it) {
          if (it->second.getNode() == itB->first)
            objB = it->first->getName();
        }

        ArgModRefResult resA = getModRefForDSNode(csA, itA->second);
        ArgModRefResult resB = getModRefForDSNode(csB, itB->second);

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

void ParPot::analyzeUseOfGlobals(Function *parent,
                                 Instruction *iA, Instruction *iB) {

  // check if instructions are really call-/invoke-instructions
  assert ((isa<CallInst>(iA) || isa<InvokeInst>(iA)) && "Invalid instruction!");
  assert ((isa<CallInst>(iB) || isa<InvokeInst>(iB)) && "Invalid instruction!");

  // get called functions of instruction A and B and check dependencies with
  // global variables
  Function *fA = getFunctionPtr(CallSite(iA));
  Function *fB = getFunctionPtr(CallSite(iB));
  checkGlobalDependencies(fA);
  checkGlobalDependencies(fB);
  GlobAccMapTy &gMapA = fGlobs_[fA];
  GlobAccMapTy &gMapB = fGlobs_[fB];

  // get dependence graph
  DepGraphMapTy::iterator dgIt = fDepGraphs_.find(parent);
  assert(dgIt != fDepGraphs_.end() && "No dependence graph found for function");

  // compare dependence sets
  for (GlobAccMapTy::iterator iGA = gMapA.begin(), eGA = gMapA.end();
        iGA != eGA; ++iGA) {
    GlobAccMapTy::iterator iGB = gMapB.find(iGA->first);
    if (iGB != gMapB.end()) {
      DependenceType dt;
      if (iGA->second == Change && iGB->second == Read)
        if (iA < iB) dt = TrueDependence;
        else dt = AntiDependence;
      else if (iGA->second == Read && iGB->second == Change)
        if (iA < iB) dt = AntiDependence;
        else dt = TrueDependence;
      else if (iGA->second == Change && iGB->second == Change)
        dt = OutputDependence;
      else continue;

      dgIt->second->addDependence(iA, iB, dt,
                        "G:"+iGA->first->getNameStr(), "G:"+iGA->first->getNameStr());
    }
  }
}

void ParPot::analyzeDominatorPath(Function *parent,
                                  Instruction *iA, Instruction *iB) {
  Instruction *first, *second;
  if (iA < iB) { first = iA; second = iB; }
  else { first = iB; second = iA; }

  // get dominator tree for function
  DominatorTree &dT = getAnalysis<DominatorTree>(*parent);
  if (!dT.dominates(first, second)){

    // get dependence graph
    DepGraphMapTy::iterator dgIt = fDepGraphs_.find(parent);
    assert(dgIt != fDepGraphs_.end()
           && "No dependence graph found for function");

    // add dependence
    dgIt->second->addDependence(iA, iB, NoDominateDependence, NoObj, NoObj);
  }
}

void ParPot::checkGlobalDependencies(Function *func) {

  static std::set<Function *> visitedFuncs;

  // check if function is available
  assert(func && "Function doesn't exist!");

  // check if function has already been analyzed
  if (visitedFuncs.find(func) != visitedFuncs.end())
    return;
  visitedFuncs.insert(func);

  // 1. add global dependencies from every called function recursively
  for (inst_iterator it = inst_begin(func), e = inst_end(func);
        it != e; ++it) {
    if (isa<CallInst>(&*it) || isa<InvokeInst>(&*it)) {
      Function *f = getFunctionPtr(CallSite(&*it));
      if (!f)
        continue; // no function found => it wasn't called during run

      checkGlobalDependencies(f);
      FuncGlobalsMapTy::iterator fgIt = fGlobs_.find(f);
      if (fgIt != fGlobs_.end())
        for (GlobAccMapTy::iterator iG = fgIt->second.begin(),
              eG = fgIt->second.end(); iG != eG; ++iG) {
          if (fGlobs_[func][iG->first] != Change)
            fGlobs_[func][iG->first] = iG->second;
        }
    }
  }

  // 2. check own dependencies with global variables
  for (Module::global_iterator it = pMod_->global_begin(),
        e = pMod_->global_end(); it != e; ++it)
    for (GlobalVariable::use_iterator iU = it->use_begin(),
          eU = it->use_end(); iU != eU; ++iU)
      if (Instruction *inst = dyn_cast<Instruction>(*iU))
        if (inst->getParent()->getParent() == func) {

          // dependence due to store instruction
          if (isa<StoreInst>(inst))
            fGlobs_[func][&*it] = Change;

          // dependence due to read instruction
          else if (isa<LoadInst>(inst) && fGlobs_[func][&*it] != Change)
            fGlobs_[func][&*it] = Read;
        }

  return;
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


  DebugInfoReader reader(DebugInfo::getFileName(), *M);


  out << " maintime: " << pDCG_->getTotExecutionTime() << '\n';


  out << "Dependence Analysis Result: \n";


  // consider all function sets (already sorted!)
  for (NodeSetVecTy::const_iterator iSet = nodeSetVec_.begin(),
      e1 = nodeSetVec_.end(); iSet != e1; ++iSet) {

    // check timing
    double minPerc=((*iSet)->getMaxSaving() / pDCG_->getTotExecutionTime()) * 100;
    double maxPerc=((*iSet)->getMaxSaving() / pDCG_->getTotExecutionTime()) * 100;

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
      Function *tmp = getFunctionPtr(cs);
      assert(tmp && "Function wasn't found in dependence graph!\n");
      if (reader.getSubprogram(tmp->getName(), iS))
        out << (*iS)->getDisplayName();
      else
        out << "IR<" << tmp->getName();

      // dump line number of instruction
      if (reader.getCallInstruction((*iNode)->getInstruction(), iC))
        out << "(" << (*iC)->getLineNo() << ") ";
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
    	Function *fFgn =
    			getFunctionPtr(CallSite((*iDep)->getToOrFromNode()->getInstruction()));
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
  DepGraph *graph = fDepGraphs_[parent];

  // consider every pair of a callsite for function A and a callsite
  // for function B to check dependencies
  for (inst_iterator it = inst_begin(parent), e = inst_end(parent);
      it != e; ++it) {
    if (isa<CallInst>(&*it) || isa<InvokeInst>(&*it)) {
      CallSite cs(&*it);
      Function *tmp = getFunctionPtr(cs);
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
      nodeSetVec_.push_back(new DGNodeSet(*graph, tmp, *this));
    }
    collectNodeSets(iF->second);
  }

  // sort function sets
    std::sort(nodeSetVec_.begin(), nodeSetVec_.end(), DGNodeSet::compare);
}

ArgModRefResult ParPot::getModRefForArg(const Function *pFunc,
																				Argument* pArg) const {
 	typedef std::map<const Value*, ArgModRefResult> ArgResSetTy;
	typedef std::vector<const Use*> UseVecTy;

	static  ArgResSetTy visited;

	assert(pArg->getType()->isPointerTy() && "Capture is for pointers only!");

	// check if argument has already visited => return old result
	{
		ArgResSetTy::iterator it = visited.find(pArg);
		if (it != visited.end())
			return it->second;
		else
			visited[pArg] = NoModRef;
	}

	// Definitions with weak linkage may be overridden at linktime with
	// something that writes memory, so treat them like declarations.
	if (pFunc->isDeclaration() || pFunc->mayBeOverridden())
		return NoModRef;

	Value::use_iterator iUse = pArg->use_begin(), eUse = pArg->use_end();
	for (; iUse != eUse; ++iUse) {
		Instruction *inst = cast<Instruction>(*iUse);
		visited[pArg] |= getModRefForInst(inst, pArg);
	}

	return visited[pArg];
}

/// analyzes how the instruction accesses the given value
ArgModRefResult ParPot::getModRefForInst(Instruction *I, const Value *pArg) const {
	ArgModRefResult result = NoModRef;

  switch (I->getOpcode()) {
  case Instruction::Call:
  case Instruction::Invoke: {
    CallSite cs(I);
    // Not captured if the callee is readonly, doesn't return a copy through
    // its return value and doesn't unwind (a readonly function can leak bits
    // by throwing an exception or not depending on the input value).
    if (cs.onlyReadsMemory() && cs.doesNotThrow()
														 && I->getType()->isVoidTy())
      break;

    // get called function as well as the corresponding argument in order
    // to call this analysis-procedure recursively
    Function::arg_iterator iFArg = cs.getCalledFunction()->arg_begin();
    CallSite::arg_iterator iArg = cs.arg_begin(), eArg = cs.arg_end();
    for (; iArg != eArg; ++iArg, ++iFArg) {
			if (iArg->get() == pArg) {
        result	|= getModRefForArg(cs.getCalledFunction(), &*iFArg);
			}
    }
    break;
  }
  case Instruction::Load:
  	if (pArg == I->getOperand(0))
  		result |= Ref;
  	break;
  case Instruction::Ret:
    	result |= Ref;
    break;
  case Instruction::Store:
    if (pArg == I->getOperand(1))
    	result |= Mod;
    break;
  case Instruction::GetElementPtr:
  case Instruction::BitCast:
  case Instruction::PHI:
  case Instruction::Select:
    // The original value is not touched via this if the new value isn't.
    for (Instruction::use_iterator UI = I->use_begin(), UE = I->use_end();
         UI != UE; ++UI) {
    	Instruction* inst = (Instruction*)*UI;
    	result |= getModRefForInst(inst, (Value*)I);
    }
    break;
  case Instruction::ICmp:
    // Don't count comparisons of a no-alias return value against null as
    // captures. This allows us to ignore comparisons of malloc results
    // with null, for example.
    if (isNoAliasCall(pArg->stripPointerCasts()))
      if (ConstantPointerNull *CPN =
            dyn_cast<ConstantPointerNull>(I->getOperand(1)))
        if (CPN->getType()->getAddressSpace() == 0)
          break;
    // Otherwise, be conservative. There are crazy ways to capture pointers
    // using comparisons.
    result |= Ref;
    break;
  default:
  	errs() << "Instruction has not been analyzed: " << *I << "\n";
  	// Something else - be speculative and say it isn't touched.
			break;
  }
  // All uses examined - not captured.
  return result;
}

ArgModRefResult ParPot::getModRefForDSNode(const CallSite &cS,
																					 const DSNodeHandle &nH) const {
	ArgModRefResult result = NoModRef;

	if (nH.getNode()->isReadNode()) result |= Ref;
	if (nH.getNode()->isModifiedNode()) result |= Mod;

	CallSite::arg_iterator iArg = cS.arg_begin(), eArg = cS.arg_end();
	for (int i=0; iArg != eArg; ++iArg, ++i)
		if (iArg->get()->getType()->isPointerTy()) {
			const DSNodeHandle n =
				nH.getNode()->getParentGraph()->getNodeForValue(iArg->get()).getNode();
			if (n == nH) {
				Function::arg_iterator iFArg = cS.getCalledFunction()->arg_begin();
				for(int j=0; j < i; ++iFArg, ++j) { }
				result = this->getModRefForArg(cS.getCalledFunction(), &*iFArg);
				break;
			}
		}

	return result;
}
