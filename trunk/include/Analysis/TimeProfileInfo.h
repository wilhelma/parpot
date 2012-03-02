//===- Analysis/TimeProfileInfo.h - Timeprofiling informations - Interfaces===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the TimeProfileInfo class, which is used to manage
// profiling information from time-measurements for functions.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_ANALYSIS_TIME_PROF_H
#define PARPOT_ANALYSIS_TIME_PROF_H

#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/ProfileInfo.h"
#include <vector>
#include <map>

namespace llvm {

	/// TimeProfileInfo class - contains profiling information regarding to
	/// timing of functions.
	class TimeProfileInfo: public ProfileInfo {
    // prohibit default assignment operations
    TimeProfileInfo(const TimeProfileInfo&);
    TimeProfileInfo& operator=(const TimeProfileInfo&);
	public:
		// Types for handling profiling information.
		typedef std::pair<const Function*, int> Priority;
		typedef std::vector<Priority> PrioList;

	private:
		// static attributes to calculate priority informations
		double static minExTime, maxExTime, totExTime;

	protected:
		// TimeInformation stores timing informations about some profiling of
		// function executions.
		std::map<const Function*, double> FunctionTimeInformation;

		// PrioInformation holds the calculated priority value by some
		// heuristics.
		std::map<const Function*, int> PrioInformation;

	public:
		TimeProfileInfo(): ProfileInfoT<Function, BasicBlock>() {}

    //===--------------------------------------------------------------===//
    /// Profile Information Queries
    ///
		double getExecutionTime(const Function *F) const {
		  std::map<const Function*, double>::const_iterator J =
		    FunctionTimeInformation.find(F);
		  if (J == FunctionTimeInformation.end()) return MissingValue;

		  return J->second;
	    }
		void setExecutionTime(const Function *F, const double&);

		int getPrioInformation(const Function *F) const {
			std::map<const Function*, int>::const_iterator J =
					PrioInformation.find(F);
			if (J == PrioInformation.end()) return MissingValue;

			return J->second;
		}

		void setPrioInformation(const Function*, const int&);

		double static getTotalExTime() {
			return TimeProfileInfo::getTotalExTime();
		}

		double static getMinExTime() {
			return TimeProfileInfo::minExTime;
		}

		double static getMaxExTime() {
			return TimeProfileInfo::maxExTime;
		}

	    //===--------------------------------------------------------------===//
	    /// Analysis Update Methods
	    ///
		PrioList getPriorityList() const;

	};

	class TimePrioComp {
	public:
		TimePrioComp() { }
		bool operator()(const TimeProfileInfo::Priority &p,
				const TimeProfileInfo::Priority &q) const {
			return (q.second < p.second);
		}
	};
}

#endif
