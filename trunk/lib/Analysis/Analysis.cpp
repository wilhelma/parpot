//============== AliasAnalysis.cpp - Analysis - Implementation -==============//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the analyis class.
//
//===----------------------------------------------------------------------===//

#include "Analysis/Analysis.h"

#include "llvm/Support/InstIterator.h"

using namespace llvm;

ArgModRefResult& operator|=(ArgModRefResult &lhs, ArgModRefResult rhs) {
	lhs = static_cast<ArgModRefResult>((int)lhs | (int)rhs);
	return lhs;
}

ArgModRefResult Analysis::getModRefForArg(const Function *pFunc,
																				  Argument *pArg) const {
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
			visited[pArg] = llvm::NoModRef;
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
ArgModRefResult Analysis::getModRefForInst(Instruction *I, const Value *pArg) const {

	typedef std::map<Instruction*, ArgModRefResult> MRInstMapTy;
	static MRInstMapTy visited;
	MRInstMapTy::iterator it = visited.find(I);
	if (it != visited.end())
		return it->second;


	visited[I] = NoModRef;

  switch (I->getOpcode()) {
  case Instruction::Call:
  case Instruction::Invoke: {
    CallSite cs(I);
    Function *f = ctx_->getFunctionPtr(cs);
    // Not captured if the callee is readonly, doesn't return a copy through
    // its return value and doesn't unwind (a readonly function can leak bits
    // by throwing an exception or not depending on the input value).
    if (!f || (cs.onlyReadsMemory()
								&& cs.doesNotThrow()
								&& I->getType()->isVoidTy())) {
    	break;
    }

    // get called function as well as the corresponding argument in order
    // to call this analysis-procedure recursively
    Function::arg_iterator iFArg = f->arg_begin();
    CallSite::arg_iterator iArg = cs.arg_begin(), eArg = cs.arg_end();
    for (; iArg != eArg; ++iArg, ++iFArg) {
			if (iArg->get() == pArg && iFArg->getType()->isPointerTy()) {
				visited[I]	|= getModRefForArg(f, &*iFArg);
			}
    }
    break;
  }
  case Instruction::Load:
  	if (pArg == I->getOperand(0))
  		visited[I] |= Ref;
  	break;
  case Instruction::Ret:
  	visited[I] |= Ref;
    break;
  case Instruction::Store:
    if (pArg == I->getOperand(1))
    	visited[I] |= Mod;
    break;
  case Instruction::GetElementPtr:
  case Instruction::BitCast:
  case Instruction::PHI:
  case Instruction::Select:
    // The original value is not touched via this if the new value isn't.
    for (Instruction::use_iterator UI = I->use_begin(), UE = I->use_end();
         UI != UE; ++UI) {
    	Instruction* inst = (Instruction*)*UI;
    	visited[I] |= getModRefForInst(inst, (Value*)I);
    }
    break;
  case Instruction::ICmp:
    // Don't count comparisons of a no-alias return value against null as
    // captures. This allows us to ignore comparisons of malloc *instPairSet[pair]s
    // with null, for example.
    if (isNoAliasCall(pArg->stripPointerCasts()))
      if (ConstantPointerNull *CPN =
            dyn_cast<ConstantPointerNull>(I->getOperand(1)))
        if (CPN->getType()->getAddressSpace() == 0)
          break;
    // Otherwise, be conservative. There are crazy ways to capture pointers
    // using comparisons.
    visited[I] |= Ref;
    break;
  default:
  	errs() << "Instruction has not been analyzed: " << *I << "\n";
  	// Something else - be speculative and say it isn't touched.
			break;
  }
  // All uses examined - not captured.

  return visited[I];
}

ArgModRefResult Analysis::getModRefForDSNode(const CallSite &cS,
																					 const DSNodeHandle &nH,
																					 const DSGraph *parentGraph) const {
	ArgModRefResult result = NoModRef;

	//if (nH.getNode()->isReadNode()) result |= Ref;
	//if (nH.getNode()->isModifiedNode()) result |= Mod;


	Function *f = ctx_->getFunctionPtr(cS);
	if (!f) return ModRef; // be conservative

	CallSite::arg_iterator iArg = cS.arg_begin(), eArg = cS.arg_end();
	for (int i=0; iArg != eArg; ++iArg, ++i)
		if (iArg->get()->getType()->isPointerTy()) {
			DSNodeHandle n = parentGraph->getNodeForValue(iArg->get());
			DSGraph::NodeMapTy nodeMap;
			parentGraph->computeNodeMapping(nH, n, nodeMap, false);
			if (!nodeMap.empty()) {
				Function::arg_iterator iFArg = f->arg_begin();
				for(int j=0; j < i; ++iFArg, ++j) { }
				if (iFArg->getType()->isPointerTy())
					result = this->getModRefForArg(f, &*iFArg);
				break;
			}
		}

	return result;
}

bool Analysis::checkDefUse(Value *val, Instruction *iB,
												 int level, bool phiVisited, bool print) {
	if (print)
		errs() << "-value: " << *val << " at level: " << level << "\n";

  static std::set<Value *> visitedVals;
  if (!level)
    visitedVals.clear();

  // ignore already visited values
  if (visitedVals.find(val) != visitedVals.end())
    return false;
  visitedVals.insert(val);

  // check if function A is correlated with function B
  if (isa<CallInst>(val) || isa<InvokeInst>(val)) {
     Instruction *inst = dyn_cast<Instruction>(&*val);

     // correlation found
     if (inst == iB)
       return phiVisited;
  }

  // consider store instructions
  if (StoreInst *sInst = dyn_cast<StoreInst>(&*val)) {
    if (checkDefUse(&*sInst->getPointerOperand(), iB, level + 1, phiVisited, print))
      return phiVisited;
  }

  // consider branch instructions
  if (BranchInst *bInst = dyn_cast<BranchInst>(&*val))
  	if (checkBranchDefUse(bInst, iB, level + 1, 0, print))
  		return true;

  // check every use of the value recursively
  for (Value::use_iterator iUse = val->use_begin(), eUse = val->use_end();
        iUse != eUse; ++iUse) {

    if (checkDefUse(*iUse, iB, level + 1, phiVisited, print))
      return true;
  }

  return false;
}

bool Analysis::checkBranchDefUse(BranchInst *bInst, Instruction *iB,
															 int level, int branchLevel, bool print) {
  static std::set<BranchInst *> visitedBranches;
  if (!branchLevel)
    visitedBranches.clear();

  // ignore already visited values
  if (visitedBranches.find(bInst) != visitedBranches.end())
    return false;
  visitedBranches.insert(bInst);

  for (unsigned i=0; i<bInst->getNumSuccessors(); ++i) {
    BasicBlock *bb = bInst->getSuccessor(i);
    for (BasicBlock::iterator it = bb->begin(), e = bb->end();
          it != e; ++it) {

      // consider store instructions within branchens
      if (StoreInst *sInst = dyn_cast<StoreInst>(&*it))
        if (checkDefUse(&*sInst, iB, level + 1, true, print))
          return true;

      // consider PHINodes within branches
      if (PHINode *phi = dyn_cast<PHINode>(&*it))
        if (checkDefUse(&*phi, iB, level + 1, true, print))
          return true;

      // check if branch is correlated with function B
      if (isa<CallInst>(&*it) || isa<InvokeInst>(&*it))
      	if (&*it == iB)
      		return true;

      // consider new branch instructions within branches
      if (BranchInst *bInst = dyn_cast<BranchInst>(&*it))
      	return checkBranchDefUse(bInst, iB, level+1, branchLevel + 1, print);
    }
  }
  return false;
}

void Analysis::checkGlobalDependencies(Function *func) {

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
      Function *f = ctx_->getFunctionPtr(CallSite(&*it));
      if (!f)
        continue; // no function found => it wasn't called during run

      checkGlobalDependencies(f);
      FuncGlobalsMapTy::iterator fgIt = ctx_->getGlobals()->find(f);
      if (fgIt != ctx_->getGlobals()->end())
        for (GlobAccMapTy::iterator iG = fgIt->second.begin(),
              eG = fgIt->second.end(); iG != eG; ++iG) {
          if ((*ctx_->getGlobals())[func][iG->first] != Change)
          	(*ctx_->getGlobals())[func][iG->first] = iG->second;
        }
    }
  }

  // 2. check own dependencies with global variables
  for (Module::global_iterator it = ctx_->getMod()->global_begin(),
        e = ctx_->getMod()->global_end(); it != e; ++it)
    for (GlobalVariable::use_iterator iU = it->use_begin(),
          eU = it->use_end(); iU != eU; ++iU)
      if (Instruction *inst = dyn_cast<Instruction>(*iU))
        if (inst->getParent()->getParent() == func) {

          // dependence due to store instruction
          if (isa<StoreInst>(inst))
            (*ctx_->getGlobals())[func][&*it] = Change;

          // dependence due to read instruction
          else if (isa<LoadInst>(inst) &&
									(*ctx_->getGlobals())[func][&*it] != Change)
            (*ctx_->getGlobals())[func][&*it] = Read;
        }

  return;
}
