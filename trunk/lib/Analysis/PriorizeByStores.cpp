//==-- PriorizeByStores.cpp - Priorize-handler (Stores) - Implementation --=-=//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines methods of the PriorizeByStores class.
//===----------------------------------------------------------------------===//
#include "Analysis/PriorizeHandler.h"

using namespace llvm;

/// The method priorize calculates priority value depending on the number of
/// store instructions in a function.
std::vector<TimeProfileInfo::Priority> PriorizeByStores::priorize(
		const CallGraph &cg, TimeProfileInfo &pi) {

	// priorize by number of store instructions
	std::vector<TimeProfileInfo::Priority> list = pi.getPriorityList();
	for (std::vector<TimeProfileInfo::Priority>::iterator
			it = list.begin(); it != list.end(); ++it) {
	  // ignore useless functions
	  if (it->second == TimeProfileInfo::MissingValue)
	    continue;

		int prio = it->second;
		if (csPass_.getStores(it->first) <= CountStoresPass::UNCRITICAL)
			prio += 10;
		else if (csPass_.getStores(it->first) <= CountStoresPass::LOW)
			prio += 7;
		else if (csPass_.getStores(it->first) <= CountStoresPass::MID)
			prio += 4;
		else if (csPass_.getStores(it->first) <= CountStoresPass::HIGH)
			prio += 2;
		else if (csPass_.getStores(it->first) <= CountStoresPass::CRITICAL)
			prio += 0;

		pi.setPrioInformation(it->first, prio);
	}

	// super call to continue chain handling
	return PriorizeHandler::priorize(cg, pi);
}
