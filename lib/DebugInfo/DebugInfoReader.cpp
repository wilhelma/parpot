//===--------- DebugInfoReader.cpp - DebugInfoReader Impl -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the default implementation of the DebugInfoReader class.
//
//===----------------------------------------------------------------------===//

#include "DebugInfo/DebugInfoReader.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <fstream>
#include "DebugInfo/CompileUnit.h"
#include "DebugInfo/Subprogram.h"
#include "DebugInfo/Allocation.h"
#include "DebugInfo/GlobalVar.h"
#include "DebugInfo/CallInstruction.h"
#include "llvm/Instructions.h"
#include <vector>
#include <sstream>

using namespace llvm;

DebugInfoReader::DebugInfoReader(const char *filename, const Module &M) {
  std::vector<DebugInfo> dbgInfos;

	// open file to write
	std::ifstream file(filename);
	if (!file) {
	    errs() << "Error: Can't open file " << filename << '\n';
	    perror(0);
	    exit(1);
	}

	// build line-instruction-map
  unsigned instNo = 0;
  for (Module::const_iterator f = M.begin(), fe = M.end(); f != fe; ++f) {
    if (f->isDeclaration()) continue;
    for (Function::const_iterator b = f->begin(), be = f->end(); b != be; ++b)
      for (BasicBlock::const_iterator i = b->begin(), ie= b->end();
          i != ie; ++i)
        if (isa<CallInst>(&*i) || isa<InvokeInst>(&*i))
          lineInstructionMap_[instNo++] = &*i;
  }

	// read complete file
	std::string line;
	int dbgType;
	instNo=0;
	while (file) {
		// read first line and corresponding debug type
		line.clear();
		if (!getline(file, line)) break;
		size_t pos = line.find_first_of(',', 0);
		std::stringstream strStream(line.substr(0, pos));
		strStream >> dbgType;

		// fill vectors with fitting debug types
		switch (dbgType) {
		case TCompileUnit:
			compileUnits_.push_back(new CompileUnit(line));
			break;
		case TSubprogram:
			subprograms_.push_back(new Subprogram(line));
			break;
		case TAllocation:
			allocations_.push_back(new Allocation(line));
			break;
		case TGlobalVar:
			globalVars_.push_back(new GlobalVar(line));
			break;
		case TCallInstruction:
			callInstructions_.push_back(new CallInstruction(line,
					lineInstructionMap_[instNo++]));
			break;
		default:
			errs() << "Error. Wrong debug type in input file "
				<< filename << '\n';
		}
	}
	file.close();
}

bool DebugInfoReader::getAllocation(const std::string &cmp
		,std::vector<Allocation*>::const_iterator it) const {
	it = std::find_if(allocations_.begin(), allocations_.end(),
			CompAllocation(cmp));
	return (it != allocations_.end());
}

bool DebugInfoReader::getCompileUnit(const std::string &cmp
		,std::vector<CompileUnit*>::const_iterator it) const {
	it = std::find_if(compileUnits_.begin(), compileUnits_.end(),
			CompCompileUnit(cmp));
	return (it != compileUnits_.end());
}

bool DebugInfoReader::getGlobalVar(const std::string &cmp
		,std::vector<GlobalVar*>::const_iterator it) const {
	it = std::find_if(globalVars_.begin(), globalVars_.end(),
			CompGlobalVar(cmp));
	return (it != globalVars_.end());
}

bool DebugInfoReader::getSubprogram(const std::string &cmp
		,std::vector<Subprogram*>::const_iterator &it) const {
	it = std::find_if(subprograms_.begin(), subprograms_.end(),
			CompSubprogram(cmp));
	return (it != subprograms_.end());
}

bool DebugInfoReader::getCallInstruction(Instruction *inst,
    std::vector<CallInstruction*>::const_iterator &it) const {
  it = std::find_if(callInstructions_.begin(), callInstructions_.end(),
      CompCallInstruction(inst));
  return (it != callInstructions_.end());
}
