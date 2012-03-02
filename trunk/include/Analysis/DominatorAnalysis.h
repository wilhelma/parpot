//====== Analysis/DominatorAnalysis.h - DominatorAnalysis - Interface ========//
//
//          ParPot - Parallelization Potential - DominatorAnalysis
//
//===----------------------------------------------------------------------===//
//
// This file defines the dominator-analysis class
//
//===----------------------------------------------------------------------===//

#ifndef PARPOT_DOMINATORALYSIS_H_
#define PARPOT_DOMINATORALYSIS_H_

#include "Analysis/Analysis.h"

namespace llvm {
	class DominatorAnalysis: public Analysis {

	public:
		DominatorAnalysis(AnalysisContext *ctx): Analysis(ctx) { };

		// the analyze method is expected to be overwritten
		void analyze(Function*, Instruction*, Instruction*);

	private:
		DominatorAnalysis(const DominatorAnalysis&);            // DO NOT IMPL
		DominatorAnalysis& operator=(const DominatorAnalysis*); // DO NOT IMPL
	};
}
#endif /* PARPOT_DOMINATORALYSIS_H_ */
