//===-- DebugInfo/DebugInfoReaderPass.h - DebugInfoReaderPass - Interface -===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the DebugInfoReader class.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_DEBUGINFOREADERPASS_H_
#define PARPOT_DEBUGINFO_DEBUGINFOREADERPASS_H_

#include "llvm//Pass.h"
#include "llvm/Module.h"
#include "DebugInfoTypes.h"

using namespace llvm;

namespace {

  /// The class DebugInfoReaderPass is a llvm module pass to read debug
  /// information from an input file.
	class DebugInfoReaderPass: public ModulePass {

	public:
		static char ID; // Class identification, replacement for typeinfo
		DebugInfoReaderPass() : ModulePass(ID) { }

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.setPreservesAll();
		}
		virtual bool runOnModule(Module &M);
	};
}


#endif /* DEBUGINFO_DEBUGINFOREADERPASS_H_ */
