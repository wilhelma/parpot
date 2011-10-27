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

#include "DebugInfo/DebugInfoReaderPass.h"
#include "DebugInfo/DebugInfo.h"
#include "DebugInfo/DebugInfoReader.h"
#include "DebugInfo/CompileUnit.h"
#include "DebugInfo/Subprogram.h"
#include "DebugInfo/Allocation.h"
#include "DebugInfo/GlobalVar.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

bool DebugInfoReaderPass::runOnModule(Module &M) {

	DebugInfoReader reader(DebugInfo::getFileName(), M);

	return false;
}


// Register the DebugInfoWriter pass...
char DebugInfoReaderPass::ID = 0;
static RegisterPass<DebugInfoReaderPass>
X("debuginforeader", "Debug-Info reader", false, true);
