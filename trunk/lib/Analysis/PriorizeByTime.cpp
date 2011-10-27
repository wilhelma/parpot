//==----- PriorizeByTime.cpp - Priorize-handler (Time) - Implementation ---=-=//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines methods of the PriorizeBySinglePar class.
//===----------------------------------------------------------------------===//
#include "Analysis/PriorizeHandler.h"
#include "llvm/Analysis/ProfileInfo.h"
#include "llvm/Analysis/ProfileInfoLoader.h"
#include "llvm/PassManager.h"
#include <math.h>
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

/// The method priorize calculates priority values for functions depending on
/// the corresponding execution times.
std::vector<TimeProfileInfo::Priority> PriorizeByTime::priorize(
		const CallGraph &cg, TimeProfileInfo &pi) {

	// priorize by execution time
	std::vector<TimeProfileInfo::Priority> list = pi.getPriorityList();
	for (std::vector<TimeProfileInfo::Priority>::iterator
			it = list.begin(); it != list.end(); ++it) {
	  // ignore useless functions
	  double rate = pi.getExecutionTime(it->first) /
                    TimeProfileInfo::getTotalExTime();
	  if (rate < PriorizeHandler::MINRATE) {
      pi.setPrioInformation(it->first, TimeProfileInfo::MissingValue);
	    continue;
	  }

		int prio = pi.getPrioInformation(it->first);
		prio += ceil(rate * getFactor());
		pi.setPrioInformation(it->first, prio);
	}

	// super call to continue chain handling
	return PriorizeHandler::priorize(cg, pi);
}
