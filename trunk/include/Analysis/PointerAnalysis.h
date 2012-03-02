//=========== Analysis/PointerAnalysis.h - AliasAnalysis - Interface =========//
//
//            ParPot - Parallelization Potential - AliasAnalysis
//
//===----------------------------------------------------------------------===//
//
// This file defines the alias-analysis class
//
//===----------------------------------------------------------------------===//

#ifndef PARPOT_POINTERANALYSIS_H_
#define PARPOT_POINTERANALYSIS_H_

#include "Analysis/Analysis.h"

namespace llvm {
	class PointerAnalysis: public Analysis {
	public:
		PointerAnalysis(AnalysisContext* tool): Analysis(tool) { };

		// the analyze method is expected to be overwritten
		void analyze(Function*, Instruction*, Instruction*);

	private:
		PointerAnalysis(const PointerAnalysis&);            // DO NOT IMPLEMENT
		PointerAnalysis& operator=(const PointerAnalysis*); // DO NOT IMPLEMENT
	};
}
#endif /* PARPOT_POINTERANALYSIS_H_ */
