//== PriorizeBySinglePar.cpp - Priorize-handler (SinglePar) - Implementation =//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines methods of the PriorizeBySinglePar class.
//===----------------------------------------------------------------------===//
#include "Analysis/PriorizeHandler.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include <algorithm>
#include <math.h>
#include <set>
#include <vector>
using namespace llvm;

/// priorizeAdjacentNodes calculates the corresponding priority values due to
/// execution times of sibling functions (callgraph). This will be done in a
/// recursively manner.
void PriorizeBySinglePar::priorizeAdjacentNodes(const CallGraphNode &node,
    TimeProfileInfo &pi, int level) {
	static std::set<const CallGraphNode*> visitedNodes;
	std::vector<const CallGraphNode*> adj;
	double totalTime = 0;

	// check if node has already been visited
	if (visitedNodes.find(&node) != visitedNodes.end() || node.size() <= 1)
	  return;
	visitedNodes.insert(&node);

	// collect adjacent nodes
	for (CallGraphNode::const_iterator it = node.begin(); it != node.end(); ++it) {
		if (!it->second->getFunction() ||
			it->second->getFunction()->isDeclaration()) continue; //||
			//it->second->getFunction()->hasExternalLinkage()) continue;
      adj.push_back(&*it->second);
      double eT = pi.getExecutionTime(it->second->getFunction());
      if (eT != TimeProfileInfo::MissingValue)
        totalTime += eT;
      DEBUG(errs() << "level " << level << " "
                   << it->second->getFunction()
                   << '\n'; );
	}

	//calculate priority value
	for (std::vector<const CallGraphNode*>::const_iterator it = adj.begin();
          it != adj.end(); ++it) {

	  // get profileinfo of function 1
	  int p1 = pi.getPrioInformation((*it)->getFunction());
	  double t1 = pi.getExecutionTime((*it)->getFunction());

	  // ignore useless functions
	  if (p1 == TimeProfileInfo::MissingValue)
	    continue;

		for (std::vector<const CallGraphNode*>::const_reverse_iterator ti =
		    adj.rbegin(); (*ti)->getFunction() != (*it)->getFunction(); ++ti) {

		  // get profileinfo of function 2
			int p2 = pi.getPrioInformation((*ti)->getFunction());
			double t2 = pi.getExecutionTime((*ti)->getFunction());

	    // ignore useless functions
	    if (p2 == TimeProfileInfo::MissingValue)
	      continue;

			double base = t1 / t2;
			if (base < 1)
			  base = pow(base, -1);
			int prio1 = ceil((1/base) * (t1/totalTime) * getFactor());
			int prio2 = ceil((1/base) * (t2/totalTime) * getFactor());

			pi.setPrioInformation((*it)->getFunction(), p1 + prio1);
			pi.setPrioInformation((*ti)->getFunction(), p2 + prio2);
			DEBUG(
      std::string f1 = "External Function";
			std::string f2 = "External Function";
			if ((*it)->getFunction())
			  f1 = (*it)->getFunction()->getName();
			if ((*ti)->getFunction())
			  f2 = (*ti)->getFunction()->getName();

			errs() << "Function 1: " << f1
            << " Function 2: " << f2
            << " prio1: " << prio1
            << " prio2: " << prio2 << '\n';
			);
		}
	}

	// breadth search algorithm
  for (std::vector<const CallGraphNode*>::const_iterator it = adj.begin();
          it != adj.end(); ++it)
    priorizeAdjacentNodes(**it, pi, level+1);
}

/// The priorize() method starts the calculation of priority values starting
/// on the root function.
std::vector<TimeProfileInfo::Priority> PriorizeBySinglePar::priorize(
		const CallGraph &cg, TimeProfileInfo &pi) {

	// priorize by parallelization potential of adjacent nodes of the cg
	// recursive node handling
	priorizeAdjacentNodes(*cg.getRoot(), pi, 0);

	// super call to continue chain handling
	return PriorizeHandler::priorize(cg, pi);
}
