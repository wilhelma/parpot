//===-------- DGNodeSet - Dependence graph node set - Implementation ------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines
//
//===----------------------------------------------------------------------===//

#include "Analysis/DGNodeSet.h"
#include "Analysis/AnalysisContext.h"

#include "llvm/Support/raw_ostream.h"

#include <set>
#include <numeric>

using namespace llvm;

DGNodeSet::DGNodeSet(const DepGraph &graph, const DGNodeVecTy &dSet,
	const AnalysisContext &ctx) : graph_(&graph), nodes_(dSet), trueDeps_(0),
													antiDeps_(0), outDeps_(0), cntDeps_(0), domDeps_(0) {
  std::vector<double> times;

  // compare dependencies pairwise
  for (DGNodeVecTy::const_iterator it = nodes_.begin(), e = nodes_.end();
						it != e; ++it) {
    for (DGNodeVecTy::const_reverse_iterator ti = nodes_.rbegin();
        (*it)->getInstruction() != (*ti)->getInstruction(); ++ti)
      findDeps(graph, *it, *ti, true);

    // collect runtimes
    times.push_back(ctx.getDCG()->getExecutionTime((*it)->getInstruction()));
  }

	// calculate min/max savings
  if (times.size() > 1) {
		minSaving_ = *std::min_element(times.begin(), times.end());
		maxSaving_ = std::accumulate(times.begin(), times.end(), 0.0) -
               *std::max_element(times.begin(), times.end());
  }
}

bool DGNodeSet::findDeps(const DepGraph &graph, DepGraphNode *src,
                         DepGraphNode *dst, bool init) {

//  // vist just once
//  static std::set<DepGraphNode *> visitedNodes;
//  if (init) visitedNodes.clear();
//  if (visitedNodes.find(src) != visitedNodes.end())
//    return false;
//  visitedNodes.insert(src);

  if (src == dst) return true; // destination reached => return true
  bool result = false; // result returns true, if a connection to dest exist

  // check outgoing dependencies
  for (DepGraphNode::const_iterator it = src->outDepBegin(),
      e = src->outDepEnd(); it != e; ++it) {

  	if ((*it)->getToOrFromNode() == dst) {
  		if ((*it)->getDepType() & TrueDependence) trueDeps_++;
      if ((*it)->getDepType() & AntiDependence) antiDeps_++;
      if ((*it)->getDepType() & OutputDependence) outDeps_++;
      if ((*it)->getDepType() & ControlDependence) cntDeps_++;
      if ((*it)->getDepType() & NoDominateDependence) domDeps_++;
      deps_.insert(*it);
      result = true;
  	}

//    if (findDeps(graph, (*it)->getToOrFromNode(), dst)) {
//      if ((*it)->getDepType() & TrueDependence) trueDeps_++;
//      if ((*it)->getDepType() & AntiDependence) antiDeps_++;
//      if ((*it)->getDepType() & OutputDependence) outDeps_++;
//      if ((*it)->getDepType() & ControlDependence) cntDeps_++;
//      if ((*it)->getDepType() & NoDominateDependence) domDeps_++;
//      deps_.insert(*it);
//      result = true;
//    }
  }

  return result;
}

bool DGNodeSet::compare(const DGNodeSet *lhs, const DGNodeSet *rhs) {

  // compute factored dependence counts
  double valL = TRUE_FACTOR * lhs->trueDeps_ + ANTI_FACTOR * lhs->antiDeps_
                 + OUT_FACTOR * lhs->outDeps_ + CNT_FACTOR * lhs->cntDeps_
                 + DOM_FACTOR * lhs->domDeps_;
  double valR = TRUE_FACTOR * rhs->trueDeps_ + ANTI_FACTOR * rhs->antiDeps_
                 + OUT_FACTOR * rhs->outDeps_ + CNT_FACTOR * rhs->cntDeps_
                 + DOM_FACTOR * rhs->domDeps_;

  // compute weighted savings for comparing
  valL = exp (-(valL / 10)) * lhs->maxSaving_;
  valR = exp (-(valR / 10)) * rhs->maxSaving_;

  return (valL > valR);
}
