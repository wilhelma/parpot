//===------- DebugInfo/DebugInfoReader.h - DebugInfoReader - Interfaces ---===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the DebugInfoReader class.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_DEBUGINFOREADER_H_
#define PARPOT_DEBUGINFO_DEBUGINFOREADER_H_

#include "llvm/Module.h"
#include "DebugInfoTypes.h"
#include "DebugInfo.h"
#include "Allocation.h"
#include "CompileUnit.h"
#include "GlobalVar.h"
#include "Subprogram.h"
#include "CallInstruction.h"
#include <vector>
#include <map>
using namespace llvm;

namespace llvm {

  /// The class DebugInfoReader reads a input file that contains debug
  /// information from an application compiled with the llvm framework.
	class DebugInfoReader{
		// attributes
		std::vector<Allocation*> allocations_;
		std::vector<CompileUnit*> compileUnits_;
		std::vector<GlobalVar*> globalVars_;
		std::vector<Subprogram*> subprograms_;
		std::vector<CallInstruction*> callInstructions_;

		// datastructure for call-instructions
		std::map<unsigned, const Instruction*> lineInstructionMap_;

		// prohibit copy constructor
		DebugInfoReader(const DebugInfoReader&);
		DebugInfoReader& operator=(const DebugInfoReader*);
	public:
		static char ID; // Class identification, replacement for typeinfo
		DebugInfoReader(const char*, const Module &M);

		bool getAllocation(const std::string&,
				std::vector<Allocation*>::const_iterator) const;
		bool getCompileUnit(const std::string&,
				std::vector<CompileUnit*>::const_iterator) const;
		bool getGlobalVar(const std::string&,
				std::vector<GlobalVar*>::const_iterator) const;
		bool getSubprogram(const std::string&,
				std::vector<Subprogram*>::const_iterator&) const;
		bool getCallInstruction(Instruction*,
		    std::vector<CallInstruction*>::const_iterator&) const;
	};
}

#endif /* DEBUGINFO_DEBUGINFOREADER_H_ */
