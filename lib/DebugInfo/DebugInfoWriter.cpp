//===- BasicAliasAnalysis.cpp - Local Alias Analysis Impl -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the default implementation of the Alias Analysis interface
// that simply implements a few identities (two different globals cannot alias,
// etc), but otherwise does no analysis.
//
//===----------------------------------------------------------------------===//

#include "DebugInfo/DebugInfo.h"
#include "DebugInfo/DebugInfoWriter.h"
#include "DebugInfo/CompileUnit.h"
#include "DebugInfo/Subprogram.h"
#include "DebugInfo/Allocation.h"
#include "DebugInfo/GlobalVar.h"
#include "DebugInfo/CallInstruction.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Casting.h"
#include "llvm/Instructions.h"
#include "llvm/Metadata.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Instructions.h"

using namespace llvm;

bool DebugInfoWriter::runOnModule(Module &M) {

	// initialize DebugInfoFinder
	DebugInfoFinder *finder = new DebugInfoFinder();
	finder->processModule(M);

	// store compile units
	for (DebugInfoFinder::iterator it = finder->compile_unit_begin(),
			e = finder->compile_unit_end(); it != e; ++it) {
		DICompileUnit dUnit(*it);
		CompileUnit unit(dUnit.getFilename().str(),
						 dUnit.getDirectory().str());
		unit.store(DebugInfo::getFileName());
	}

	// store subprograms
	for (DebugInfoFinder::iterator it = finder->subprogram_begin(),
			e = finder->subprogram_end(); it != e; ++it) {
		DISubprogram dSub(*it);
		Subprogram sub(dSub.getLinkageName().str(), dSub.getDisplayName().str(),
			(std::string)(dSub.getCompileUnit().getDirectory().str() + "/" +
			dSub.getCompileUnit().getFilename().str()), dSub.getLineNumber());
		sub.store(DebugInfo::getFileName());
	}

	errs() << "TEST\n";

	// store line numbers of function calls
	std::string displayName, file, directory, type;
	unsigned callCounter = 0;
	for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f) {
		if (f->isDeclaration()) continue;
		for (inst_iterator i = inst_begin(f), ie = inst_end(f); i != ie; ++i) {
			if (isa<CallInst>(&*i) || isa<InvokeInst>(&*i)) {
			  CallSite cs(&*i);
        if (cs.getCalledFunction() &&
        		(cs.getCalledFunction()->getName() == "llvm.dbg.declare" ||
        		 cs.getCalledFunction()->getName() == "llvm.dbg.value"))
          continue;

			  if (MDNode *N = i->getMetadata("dbg")) {

				   DILocation loc(N);
			  	 DILocation orig = loc.getOrigLocation();
			  	 if (orig)
			  		 loc = orig;

		  		 CallInstruction ci(callCounter, loc.getLineNumber(),
														  loc.getFilename());

				   ci.store(DebugInfo::getFileName());
			  }
				callCounter++;
			}
		}
	}

	// store global variables
	for (DebugInfoFinder::iterator it = finder->global_variable_begin(),
			e = finder->global_variable_end(); it != e; ++it) {
		DIGlobalVariable dgv(*it);
		GlobalVar gv(dgv.getName(), dgv.getDisplayName().str(),
				dgv.getType().getName().str(),
				(std::string)(dgv.getCompileUnit().getDirectory().str() + "/"
				+ dgv.getCompileUnit().getFilename().str()),
				dgv.getLineNumber());
		gv.store(DebugInfo::getFileName());
	}

	return false;
}


// Register the DebugInfoWriter pass...
char DebugInfoWriter::ID = 0;
static RegisterPass<DebugInfoWriter>
X("parpot-dbgwriter", "Debug-Info writer", false, true);
