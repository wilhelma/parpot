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

#include "DynCallGraph/DynCallGraphParser.h"
#include "DynCallGraph/DynCallGraph.h"
#include "Analysis/Analysis.h"
#include "Analysis/TimeProfileInfo.h"
#include "Analysis/CorrelationAnalysis.h"
#include "Analysis/PointerAnalysis.h"
#include "Analysis/GlobalsAnalysis.h"
#include "Analysis/CountStoresPass.h"
#include "Analysis/TimeProfileInfoLoader.h"
#include "Analysis/AnalysisContext.h"
#include "Analysis/DGNodeSet.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Value.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Module.h"

#include <DataStructure.h>
#include <DataStructureAA.h>
#include <DSGraph.h>

#include <math.h>
#include <map>
#include <queue>
#include <vector>

namespace llvm {

	/// ParPot class -
	class ParPot: public ModulePass {
	public:
		DGNodeSet::NodeSetVecTy nodeSetVec_;

		static const std::string NoObj;

	private:
		ParPot(const ParPot&);            // DO NOT IMPLEMENT
		ParPot& operator=(const ParPot*); // DO NOT IMPLEMENT

		// constants
		static const std::string Separator;

		// members
		AnalysisContext *ctx_;
		PointerAnalysis *pointerAnalysis;
		CorrelationAnalysis *corrAnalysis;
		GlobalsAnalysis *globalsAnalysis;

		/// analyze dependence graph of a given ParPotNode and all children
		void analyzeDependencies(Function*);

		/// collect every set of nodes which are at the same level (has the same
		/// parent)
		void collectNodeSets(Function *parent);

		/// helper function to print a dependency type string
		void printDepType(raw_ostream&, unsigned char type) const;

	public:
		static char ID; // Class identification, replacement for typeinfo
		ParPot() : ModulePass(ID) { }

		~ParPot() {
			const DGNodeSet::DepGraphMapTy *graphs = ctx_->getDepGraphs();
			for (DGNodeSet::DepGraphMapTy::const_iterator it = graphs->begin(),
						e = graphs->end(); it != e; ++ it)
				delete it->second;
		}

		virtual bool runOnModule(Module &M);

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.setPreservesAll();
				//AU.addRequired<CallGraph>();
				AU.addRequired<EquivBUDataStructures>();
				AU.addRequired<DynCallGraphParserPass>();
				AU.addRequired<DSAA>();
				AU.addRequired<BUDataStructures>();
	 }

		/// dump result of the dependency analysis into the given stream.
		virtual void print(raw_ostream &O, const Module *M) const;
	};
}

#endif
