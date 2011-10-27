#include "Analysis/PriorizeHandler.h"
#include <math.h>
using namespace llvm;

const double PriorizeHandler::MINRATE = 0.01;

std::vector<TimeProfileInfo::Priority> PriorizeHandler::priorize(
		const CallGraph &cg, TimeProfileInfo &pi) {

	if (!successor_) {
		// return sorted profiling informations
		return pi.getPriorityList();
	} else {
		// go ahead with the next PriorizeHandler
		return successor_->priorize(cg, pi);
	}
}
