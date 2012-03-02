//========= Analysis/AnalysisContext.h - AnalysisTool - Interface ============//
//
//             ParPot - Parallelization Potential - Analysis Context
//
//===----------------------------------------------------------------------===//
//
// This file defines the analysis context class
//
//===----------------------------------------------------------------------===//

#ifndef PARPOT_ANALYSISCONTEXT_H_
#define PARPOT_ANALYSISCONTEXT_H_

#include "llvm/Instructions.h"
#include "llvm/Analysis/CallGraph.h"

#include "DynCallGraph/DynCallGraph.h"
#include "Analysis/DGNodeSet.h"

#include <DataStructure.h>
#include <DataStructureAA.h>
#include <DSGraph.h>

namespace llvm {
	class ParPot;

	// constants
	static const std::string NoObj = "";

	// enums
	enum ArgModRefResult {
		NoModRef = 0x0,
		Ref			 = 0x1,
		Mod			 = 0x2,
		ModRef 	 = 0x3
	};
	enum AccTy { // access type
		Read            = 0x0,
		Change          = 0x1
	};

	// types
	typedef std::map<GlobalVariable*, AccTy> GlobAccMapTy; // access type per gv
	typedef std::map<Function*, GlobAccMapTy> FuncGlobalsMapTy; //globals per fn

	ArgModRefResult& operator|=(ArgModRefResult &lhs, ArgModRefResult rhs);

	class AnalysisContext {
		AnalysisContext(const AnalysisContext&);            // DO NOT IMPLEMENT
		AnalysisContext& operator=(const AnalysisContext*); // DO NOT IMPLEMENT

		// members
		FuncGlobalsMapTy fGlobs_;

		// members
		Module *pMod_;
		DynCallGraph *pDCG_;
		CallGraph *pCG_;
		EquivBUDataStructures *pDSA_;
		DSAA *pAA_;
		BUDataStructures *pBU_;

		DGNodeSet::DepGraphMapTy fDepGraphs_;

	public:
		AnalysisContext(Module *pMod, DynCallGraph *pDCG, CallGraph *pCG,
								 EquivBUDataStructures *pDSA, DSAA *pAA,
								 BUDataStructures *pBU): pMod_(pMod), pDCG_(pDCG), pCG_(pCG),
																				 pDSA_(pDSA), pAA_(pAA), pBU_(pBU){ }

		/// returns a function pointer tied to the given callsite. If a dynamic
		/// function is called, the pointer to the concrete function may be returned
		/// if possible. Returns false if this isn't possible.
		Function* getFunctionPtr(const CallSite&) const;

		DGNodeSet::DepGraphMapTy* getDepGraphs(void) { return &fDepGraphs_; }
		FuncGlobalsMapTy *getGlobals(void) { return &fGlobs_; }

	  DynCallGraph* getDCG(void) const { return pDCG_; }
	  CallGraph* getGC(void) const { return pCG_; }
	  EquivBUDataStructures* getDSA(void) const { return pDSA_; }
	  DSAA* getAA(void) const { return pAA_; }
	  BUDataStructures* getBU(void) const { return pBU_; }
	  Module* getMod(void) const { return pMod_; }
	};
}


#endif /* PARPOT_ANALYSISCONTEXT_H_ */
