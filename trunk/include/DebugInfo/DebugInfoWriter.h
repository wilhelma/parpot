//===--- DebugInfo/DebugInfoWriter.h - DebugInfoReaderWriter - Interface --===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the DebugInfoWriter class.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_DEBUGINFOWRITER_H_
#define PARPOT_DEBUGINFO_DEBUGINFOWRITER_H_

#include "llvm//Pass.h"
#include "llvm/Module.h"
#include "DebugInfoTypes.h"

using namespace llvm;

namespace {

  /// The DebugInfoWriter class is a llvm pass which collects debug information
  /// from bytecode and writes it to a external file.
	class DebugInfoWriter: public ModulePass {

	public:
		static char ID; // Class identification, replacement for typeinfo
		DebugInfoWriter() : ModulePass(ID) { }

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.setPreservesAll();
		}
		virtual bool runOnModule(Module &M);
	};
}


#endif /* DEBUGINFOWRITER_H_ */
