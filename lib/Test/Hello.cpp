//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hello"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Instructions.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Constants.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "Analysis/DepGraph.h"
#include <vector>
#include <map>

using namespace llvm;

static int const Threshold = 20;

namespace {

	enum ArgModRefResult {
		NoModRef = 0x0,
		Ref			 = 0x1,
		Mod			 = 0x2,
		ModRef 	 = 0x3
	};

	ArgModRefResult& operator|=(ArgModRefResult &lhs, ArgModRefResult rhs) {
		lhs = static_cast<ArgModRefResult>((int)lhs | (int)rhs);
		return lhs;
	}

  struct Test1 : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    Test1() : ModulePass(ID) {}

  	typedef std::vector<Value*> ArgVectTy;
  	typedef std::map<Instruction*, ArgVectTy*> CallArgsVectTy;

    enum AccTy { // access type
      Read            = 0x0,
      Change          = 0x1
    };
    typedef std::map<GlobalVariable*, AccTy> GlobAccMapTy; // access type per gv
    typedef std::map<Function*, GlobAccMapTy> FuncGlobalsMapTy; // globals per fn
    typedef std::map<Function*, DepGraph*> DepGraphMapTy; // dep. graph

    DepGraphMapTy fDepGraphs_;

    virtual bool runOnModule(Module &M) {

    	Module::iterator iFunc = M.begin(), eFunc = M.end();
    	for (; iFunc != eFunc; ++iFunc) {
      	CallArgsVectTy callArgs;
    		getCallArgs(*iFunc, callArgs);

    	  // create dependency graph for this node
    	  DepGraph *graph = new DepGraph(&*iFunc);
    	  fDepGraphs_[&*iFunc] = graph;

      	CallArgsVectTy::iterator iAV = callArgs.begin(), eAV = callArgs.end();
      	for (; iAV != eAV; ++iAV) {
        	CallArgsVectTy::reverse_iterator rAV = callArgs.rbegin();
        	for (; iAV->first != rAV->first; ++rAV) {
        		analyzeCorrelation(&*iFunc, iAV->first, rAV->first);
        	}
      	}
    	}

      return false;
    }

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    }

	private:
    void getCallArgs(Function &F, CallArgsVectTy &callArgs) const {

    	// collect callargs for function
      for (inst_iterator iInst = inst_begin(F), eInst = inst_end(F);
            iInst != eInst; ++iInst) {
      	if (isa<CallInst>(*iInst) || isa<InvokeInst>(*iInst)) {
      		Instruction *inst = cast<Instruction>(&*iInst);
      		CallSite cs(inst);
      		CallSite::arg_iterator iArg = cs.arg_begin(), eArg = cs.arg_end();
      		for (; iArg != eArg; ++iArg) {
       			if (iArg->get()->getType()->isPointerTy()) {
       				if (!callArgs[inst])
       					callArgs[inst] = new ArgVectTy();
       				callArgs[inst]->push_back(iArg->get());
       			}
      		}
       }
     }
    }

    void analyzeCorrelation(Function *parent,
        Instruction *iA, Instruction *iB, bool invert = false) {

      // check instructions
      assert((isa<CallInst>(iA) || isa<InvokeInst>(iA)) &&
             (isa<CallInst>(iA) || isa<InvokeInst>(iA)) &&
             "Error, instructions are no function calls!");

      DepGraphMapTy::iterator dgIt = fDepGraphs_.find(parent);
      Function *fA = CallSite(iA).getCalledFunction();

        // look for pointer/reference parameters
        for (Function::arg_iterator it = fA->arg_begin(), e = fA->arg_end();
              it != e; ++it) {
          if (it->getType()->isPointerTy()) {
            ArgModRefResult res = getModRefForArg(*fA, &*it);
            if (res & Mod) {
              Value *arg = iA->getOperand(it->getArgNo()); //op(0)=function itself
              if (checkDefUse(arg, &*iB, 0, false))
              	errs() << "   control dep for " << *arg
											 << " from " << parent->getName() << "\n";
                dgIt->second->addDependence(iA, iB, ControlDependence,
                                          arg->getName(), "-");
            }
          }
        }

      // look for use of call instruction itself
//      if (checkDefUse(&*iA, &*iB, 0)) {
//        dgIt->second->addDependence(iA, iB, ControlDependence, "-", "-");
//      }

      // analyze the inverted order
      if (invert) analyzeCorrelation (parent, iB, iA);
    }

    bool checkDefUse(Value *val, Instruction *iB, int level, bool phiVisited) {
    	errs() << "-value: " << *val << " at level: " << level << "\n";

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
           return phiVisited;
      }

      // consider store instructions
      if (StoreInst *sInst = dyn_cast<StoreInst>(&*val)) {
        if (checkDefUse(&*sInst->getPointerOperand(), iB, level + 1, phiVisited))
          return phiVisited;
      }

      // consider branch instructions
      if (BranchInst *bInst = dyn_cast<BranchInst>(&*val)) {
        for (unsigned i=0; i<bInst->getNumSuccessors(); ++i) {
          BasicBlock *bb = bInst->getSuccessor(i);
          for (BasicBlock::iterator it = bb->begin(), e = bb->end();
                it != e; ++it) {

            // consider store instructions within branchens
            if (StoreInst *sInst = dyn_cast<StoreInst>(&*it))
              if (checkDefUse(&*sInst, iB, level + 1, true))
                return true;

            // consider PHINodes within branches
            if (PHINode *phi = dyn_cast<PHINode>(&*it))
              if (checkDefUse(&*phi, iB, level + 1, true))
                return true;
          }
        }
      }

      // consider phi instructions
      if (PHINode *phi = dyn_cast<PHINode>(val)) {
      	for (unsigned i=0; i<phi->getNumIncomingValues(); ++i) {
      		if (checkDefUse(phi->getIncomingValue(i), iB, level + 1, true))
      			return true;
      	}
      }

      // check every use of the value recursively
      for (Value::use_iterator iUse = val->use_begin(), eUse = val->use_end();
            iUse != eUse; ++iUse) {

        if (checkDefUse(*iUse, iB, level + 1, phiVisited))
          return true;
      }

      return false;
    }

    ArgModRefResult getModRefForArg(const Function &F, Argument *pArg) const {
        typedef std::map<const Value*, ArgModRefResult> ArgResSetTy;
        typedef std::vector<const Use*> UseVecTy;

        static  ArgResSetTy visited;

        assert(&F && "Error: No Function!");
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
                        if (F.isDeclaration() || F.mayBeOverridden())
                                return NoModRef;

                        Value::use_iterator iUse =pArg->use_begin(), eUse =pArg->use_end();
                        for (; iUse != eUse; ++iUse) {
                                Instruction *inst = cast<Instruction>(*iUse);
                                visited[pArg] |= getModRefForInst(inst, pArg);
                        }

               return visited[pArg];
		}

		ArgModRefResult getModRefForInst(Instruction *I, const Value *pArg) const {
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
                  result        |= getModRefForArg(*cs.getCalledFunction(), &*iFArg);
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

  };
}

char Test1::ID = 0;
static RegisterPass<Test1>
Y("parpot-test", "Hello World Pass (with getAnalysisUsage implemented)");
