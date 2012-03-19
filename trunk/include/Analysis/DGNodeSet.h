//======= Analysis/DGNodeSet.h - Dependency Graph Node Set - Interface =======//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the DGNodeSet class
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DGNODESET_H
#define PARPOT_DGNODESET_H

#include "Analysis/DepGraph.h"

#include <set>

namespace llvm {
	class AnalysisContext;

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
		typedef std::vector<DGNodeSet *> NodeSetVecTy;
		typedef std::map<Function*, DepGraph*> DepGraphMapTy;

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

		DGNodeSet(const DepGraph &graph, const DGNodeVecTy &dSet,
							const AnalysisContext &ctx);

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
		size_t							size() const			{ return nodes_.size(); }

		double getMinSaving() const { return minSaving_; }
		double getMaxSaving() const { return maxSaving_; }

		const DepGraph* getGraph(void) const { return graph_; }

		static bool compare(const DGNodeSet*, const DGNodeSet*);
	};
}

#endif
