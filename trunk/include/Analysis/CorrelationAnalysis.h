//========= Analysis/Analysis.h - CorrelationAnalysis - Interface ============//
//
//          ParPot - Parallelization Potential - CorrelationAnalysis
//
//===----------------------------------------------------------------------===//
//
// This file defines the alias-analysis class
//
//===----------------------------------------------------------------------===//

#ifndef PARPOT_CORRELATIONANALYSIS_H_
#define PARPOT_CORRELATIONANALYSIS_H_

#include "Analysis/Analysis.h"

namespace llvm {
	class CorrelationAnalysis: public Analysis {

	public:
		CorrelationAnalysis(AnalysisContext *ctx): Analysis(ctx) { };

		// the analyze method is expected to be overwritten
		void analyze(Function*, Instruction*, Instruction*);

	private:
		CorrelationAnalysis(const CorrelationAnalysis&);            // DO NOT IMPL
		CorrelationAnalysis& operator=(const CorrelationAnalysis*); // DO NOT IMPL
	};
}
#endif /* PARPOT_CORRELATIONANALYSIS_H_ */
