//======= Analysis/GlobalsAnalysis.h - GlobalsAnalysis - Interface ===========//
//
//          ParPot - Parallelization Potential - GlobalsAnalysis
//
//===----------------------------------------------------------------------===//
//
// This file defines the globals-analysis class
//
//===----------------------------------------------------------------------===//

#ifndef PARPOT_GLOBALSANALYSIS_H_
#define PARPOT_GLOBALSANALYSIS_H_

#include "Analysis/Analysis.h"

namespace llvm {
	class GlobalsAnalysis: public Analysis {

	public:
		GlobalsAnalysis(AnalysisContext *ctx): Analysis(ctx) { };

		// the analyze method is expected to be overwritten
		void analyze(Function*, Instruction*, Instruction*);

	private:
		GlobalsAnalysis(const GlobalsAnalysis&);            // DO NOT IMPL
		GlobalsAnalysis& operator=(const GlobalsAnalysis*); // DO NOT IMPL
	};
}
#endif /* PARPOT_GLOBALSANALYSIS_H_ */
