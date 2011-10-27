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
#include <vector>
#include <map>

using namespace llvm;

static int const Threshold = 20;

namespace {

	enum ModRefResult {
		NoModRef = 0x00,
		Ref			 = 0x1,
		Mod			 = 0x2,
		ModRef = 0x3
	};

	ModRefResult operator|(ModRefResult lhs, ModRefResult rhs) {
		return (ModRefResult) ((int)lhs | (int)rhs);
	}

	ModRefResult operator|=(ModRefResult lhs, ModRefResult rhs) {
		return (ModRefResult) ((int)lhs | (int)rhs);
	}

  struct Test1 : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    Test1() : ModulePass(ID) {}

  	typedef std::vector<Value*> ArgVectTy;
  	typedef std::map<Instruction*, ArgVectTy*> CallArgsVectTy;

    virtual bool runOnModule(Module &M) {

    	AliasAnalysis &pAA = getAnalysis<AliasAnalysis>();
    	Module::iterator iFunc = M.begin(), eFunc = M.end();
    	for (; iFunc != eFunc; ++iFunc) {
      	ArgVectTy callArgs;

      	errs() << "Function " << iFunc->getName() << "\n";
      	Function::arg_iterator iArg = iFunc->arg_begin(),
															 eArg = iFunc->arg_end();
      	for (; iArg != eArg; ++iArg) {
      		if (!iArg->getType()->isPointerTy())
      			continue;
      		errs() << "  Argument: " << *iArg << "\n";
      		getModRefForArg(*iFunc, &*iArg);
      	}


//      	ArgVectTy::iterator iCall =callArgs.begin(), eCall =callArgs.end();
//      	for (; iCall != eCall; ++iCall) {
//
//      		CallSite cs(iCall->first);
//      		errs() << "CallSite: " << *iCall->first << "\n";
//
//      		ArgVectTy::iterator iArg = iCall->second->begin(),
//															eArg = iCall->second->end();
//					for (; iArg != eArg; ++iArg) {
//
//					}
//

//      		CallArgsVectTy::reverse_iterator rCall = callArgs.rbegin();
//      		for (; rCall->first != iCall->first; rCall++) {
//
//      			errs() << "check " << iCall->first << ":" << rCall->first
//  								 << *iCall->first << " - "
//  								 << *rCall->first << "\n";
//      			CallSite cs1(iCall->first), cs2(rCall->first);
//      			ArgVectTy::iterator iArg1 = iCall->second->begin(),
//  															eArg1 = iCall->second->end();
//      			for (; iArg1 != eArg1; ++iArg1) {
//							ArgVectTy::iterator iArg2 = rCall->second->begin(),
//																	eArg2 = rCall->second->end();
//  						for (; iArg2 != eArg2; ++iArg2) {
//  							errs() << "   1: " << (*iArg1)->getName()
//  										 << "   2: " << (*iArg2)->getName();
//  							AliasAnalysis::AliasResult res =
//  									pAA.alias(AliasAnalysis::Location(*iArg1),
//															AliasAnalysis::Location(*iArg2));
//  							errs() << " res: " << res << "\n";
//  						}
//
//  						getModRefForArg(*cs1.getCalledFunction(), *iArg1);
//      			}
//
//      		}
//      	}
    	}
      return false;
    }

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
      AU.addRequired<MemoryDependenceAnalysis>();
      AU.addRequired<AliasAnalysis>();
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

    ModRefResult getModRefForArg(const Function &F, Argument *pArg) const {
    	typedef std::map<const Value*, ModRefResult> ArgResSetTy;
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

			errs() << "Begin: " << *pArg << " in " << F.getName() << "\n";
			Value::use_iterator iUse =pArg->use_begin(), eUse =pArg->use_end();
			for (; iUse != eUse; ++iUse) {
				errs() << "  Use: " << **iUse;

				Instruction *inst = cast<Instruction>(*iUse);
				visited[pArg] |= getModRefForInst(inst, pArg);
				errs () << " and is: " << visited[pArg] << "\n";
			}

			return visited[pArg];
		}

		ModRefResult getModRefForInst(Instruction *I,
																						const Value *pArg) const {

			errs() << "    inst is... " << *I << "\n";

			ModRefResult result = NoModRef;

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
	          result	|= getModRefForArg(*cs.getCalledFunction(), &*iFArg);
					}
        }
        break;
      }
      case Instruction::Load:
      	errs() << "    a load at " << *I << " with " << *pArg << " and " << *I->getOperand(0) << "\n";
      	if (pArg == I->getOperand(0))
      		result |= Ref;
      	break;
      case Instruction::Ret:
        if (pArg == I->getOperand(0))
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
        // Something else - be speculative and say it isn't touched.
					break;
      }
      // All uses examined - not captured.
      return result;
		}

    /// PointerMayBeCaptured - Return true if this pointer value may be captured
    /// by the enclosing function (which is required to exist).  This routine can
    /// be expensive, so consider caching the results.  The boolean ReturnCaptures
    /// specifies whether returning the value (or part of it) from the function
    /// counts as capturing it or not.  The boolean StoreCaptures specified whether
    /// storing the value (or part of it) into memory anywhere automatically
    /// counts as capturing it or not.
    bool PointerMayBeCaptured(const Value *V, bool ReturnCaptures,
                                    bool StoreCaptures) const {
      assert(V->getType()->isPointerTy() && "Capture is for pointers only!");
      SmallVector<Use*, Threshold> Worklist;
      SmallSet<Use*, Threshold> Visited;
      int Count = 0;

      for (Value::const_use_iterator UI = V->use_begin(), UE = V->use_end();
           UI != UE; ++UI) {
        // If there are lots of uses, conservatively say that the value
        // is captured to avoid taking too much compile time.
        if (Count++ >= Threshold)
          return true;

        Use *U = &UI.getUse();
        Visited.insert(U);
        Worklist.push_back(U);
      }

      while (!Worklist.empty()) {
        Use *U = Worklist.pop_back_val();
        Instruction *I = cast<Instruction>(U->getUser());
        V = U->get();

        switch (I->getOpcode()) {
        case Instruction::Call:
        case Instruction::Invoke: {
          CallSite CS(I);
          // Not captured if the callee is readonly, doesn't return a copy through
          // its return value and doesn't unwind (a readonly function can leak bits
          // by throwing an exception or not depending on the input value).
          if (CS.onlyReadsMemory() && CS.doesNotThrow() && I->getType()->isVoidTy())
            break;

          // Not captured if only passed via 'nocapture' arguments.  Note that
          // calling a function pointer does not in itself cause the pointer to
          // be captured.  This is a subtle point considering that (for example)
          // the callee might return its own address.  It is analogous to saying
          // that loading a value from a pointer does not cause the pointer to be
          // captured, even though the loaded value might be the pointer itself
          // (think of self-referential objects).
          CallSite::arg_iterator B = CS.arg_begin(), E = CS.arg_end();
          for (CallSite::arg_iterator A = B; A != E; ++A)
            if (A->get() == V && !CS.paramHasAttr(A - B + 1, Attribute::NoCapture))
              // The parameter is not marked 'nocapture' - captured.
              return true;
          // Only passed via 'nocapture' arguments, or is the called function - not
          // captured.
          break;
        }
        case Instruction::Load:
          // Loading from a pointer does not cause it to be captured.
          break;
        case Instruction::VAArg:
          // "va-arg" from a pointer does not cause it to be captured.
          break;
        case Instruction::Ret:
          if (ReturnCaptures)
            return true;
          break;
        case Instruction::Store:
          if (V == I->getOperand(0))
            // Stored the pointer - conservatively assume it may be captured.
            // TODO: If StoreCaptures is not true, we could do Fancy analysis
            // to determine whether this store is not actually an escape point.
            // In that case, BasicAliasAnalysis should be updated as well to
            // take advantage of this.
            return true;
          // Storing to the pointee does not cause the pointer to be captured.
          break;
        case Instruction::BitCast:
        case Instruction::GetElementPtr:
        case Instruction::PHI:
        case Instruction::Select:
          // The original value is not captured via this if the new value isn't.
          for (Instruction::use_iterator UI = I->use_begin(), UE = I->use_end();
               UI != UE; ++UI) {
            Use *U = &UI.getUse();
            if (Visited.insert(U))
              Worklist.push_back(U);
          }
          break;
        case Instruction::ICmp:
          // Don't count comparisons of a no-alias return value against null as
          // captures. This allows us to ignore comparisons of malloc results
          // with null, for example.
          if (isNoAliasCall(V->stripPointerCasts()))
            if (ConstantPointerNull *CPN =
                  dyn_cast<ConstantPointerNull>(I->getOperand(1)))
              if (CPN->getType()->getAddressSpace() == 0)
                break;
          // Otherwise, be conservative. There are crazy ways to capture pointers
          // using comparisons.
          return true;
        default:
          // Something else - be conservative and say it is captured.
          return true;
        }
      }

      // All uses examined - not captured.
      return false;
    }

  };
}

char Test1::ID = 0;
static RegisterPass<Test1>
Y("parpot-test", "Hello World Pass (with getAnalysisUsage implemented)");
