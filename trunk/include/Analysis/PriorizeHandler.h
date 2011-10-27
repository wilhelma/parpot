//===------ Analysis/PriorizeHandler.h - Priorize Handler - Interfaces ----===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the class PriorizeHandler and all it's inheritated
// classes. It uses the chain of responsibility pattern to realize a heuristic
// for measuring basically parallelization potential of functions.
//
//===----------------------------------------------------------------------===//
#include "Analysis/TimeProfileInfo.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/ProfileInfo.h"
#include "CountStoresPass.h"
#include "llvm/Module.h"

#ifndef PARPOT_ANALYSIS_PRIORIZE_HANDLER_H
#define PARPOT_ANALYSIS_PRIORIZE_HANDLER_H

namespace llvm {
  
  /// The abstract basis class of the priorization handler-chain. Every node
  /// has it's successor which will be called before the own implementation.
	class PriorizeHandler {
		PriorizeHandler *successor_;
  protected:
    virtual ~PriorizeHandler() = 0;
	public:
    static const double MINRATE;
		PriorizeHandler(PriorizeHandler *succ): successor_(succ) { }
    
    /// The priorize() method applies some logic on the priority data to
    /// recalculate the corresponding values.
		virtual std::vector<TimeProfileInfo::Priority> priorize(
				const CallGraph &cg, TimeProfileInfo &pi);

		/// Returns the factor of the priority handler.
		virtual unsigned getFactor() const = 0;
	};

  /// The PriorizeByTime class uses timing information to calculate priority
  /// values for functions. The longer a function execution takes the higher
  /// is the corresponding value (0 - 10)
	class PriorizeByTime: public PriorizeHandler {
	public:
		PriorizeByTime(PriorizeHandler *succ): PriorizeHandler(succ) { }
		virtual std::vector<TimeProfileInfo::Priority> priorize(
				const CallGraph &cg, TimeProfileInfo &pi);
		/// Returns the factor of the priority handler.
		virtual unsigned getFactor() const { return 20;	}
	};

  /// The PriorizeByStores class consider store instructions of functions to
  /// calculate priority values. A higher number of store instructions means
  /// a bigger priority value (0 - 10)
	class PriorizeByStores: public PriorizeHandler {
		CountStoresPass &csPass_;
	public:
		PriorizeByStores(CountStoresPass &csp, PriorizeHandler *succ):
		  PriorizeHandler(succ), csPass_(csp) { }
		virtual std::vector<TimeProfileInfo::Priority> priorize(
				const CallGraph &cg, TimeProfileInfo &pi);
		virtual unsigned getFactor() const { return 0; }
	};

  /// The PriorizeBySinglePar class compares execution times of sibling
  /// functions of the callgraph. Functions which have almost equal execution
  /// times get higher priority values.
	class PriorizeBySinglePar: public PriorizeHandler {
	public:
		PriorizeBySinglePar(PriorizeHandler *succ):	PriorizeHandler(succ) { }
		virtual std::vector<TimeProfileInfo::Priority> priorize(
				const CallGraph &cg, TimeProfileInfo &pi);
		void priorizeAdjacentNodes(const CallGraphNode&,
          TimeProfileInfo&, int level);
		virtual unsigned getFactor() const { return 5; }
	};
}

#endif
