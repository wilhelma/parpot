//============== Analysis/Analysis.h - Analysis - Interface ==================//
//
//                 ParPot - Parallelization Potential - Analysis
//
//===----------------------------------------------------------------------===//
//
// This file defines the abstract analysis class
//
//===----------------------------------------------------------------------===//

#ifndef PARPOT_ANALYSIS_H_
#define PARPOT_ANALYSIS_H_

#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Operator.h"

#include "Analysis/AnalysisContext.h"

namespace llvm {

	enum ArgModRefResult {
		NoModRef = 0x0,
		Ref			 = 0x1,
		Mod			 = 0x2,
		ModRef 	 = 0x3
	};

	class Analysis {

	protected:
		AnalysisContext *ctx_;

		/// check every dependence (read or write) of a given function to globals
		void checkGlobalDependencies(Function *func);

	  /// check if the definition of a value reaches an instruction (recursively).
	  bool checkDefUse(Value*, Instruction*, int, bool, bool);

	  /// check if the definition of a value is used within a branch (recursively).
	  bool checkBranchDefUse(BranchInst*, Instruction*, int, int, bool);

	  /// analyzes whether and how an argument is accessed by a function
	  ArgModRefResult getModRefForArg(const Function*, Argument*) const;

	  /// analyzes how the instruction accesses the given value
	  ArgModRefResult getModRefForInst(Instruction *I, const Value *pArg) const;

	  /// returns the mod/ref behavior of a callsite concerning a specific arg
	  ArgModRefResult getModRefForDSNode(const CallSite&,
																			 const DSNodeHandle&,
																			 const DSGraph*) const;

	public:
		Analysis(AnalysisContext *ctx): ctx_(ctx) { }

		// the analyze method is expected to be overwritten
		virtual void analyze(Function*, Instruction*, Instruction*) = 0;

	private:
		Analysis(const Analysis&);            // DO NOT IMPLEMENT
		Analysis& operator=(const Analysis*); // DO NOT IMPLEMENT
	};
}
#endif /* PARPOT_ANALYSIS_H_ */
