//===- CountStoresPass.cpp - Count store instruction pass - Implementation ===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines methods of the CountStoresPass class and register the pass
// under the name "countstores".
//
//===----------------------------------------------------------------------===//

#include "Analysis/CountStoresPass.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/Casting.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

bool CountStoresPass::runOnModule(Module &M)  {
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		int stores = 0;
		for (inst_iterator I = inst_begin(*f), E = inst_end(*f); I != E; ++I)
			if (isa<StoreInst>(&*I))
				stores++;
		storeInstructions[&*f] = stores;
	}

	return false;
}

int CountStoresPass::getStores(const Function *f) {
	if (storeInstructions.find(f) != storeInstructions.end())
		return storeInstructions[f];
	else
		return MISSING;
}

// Register this pass...
static RegisterPass<CountStoresPass>
Y("countstores", "Count stores of functions", false, true);

char CountStoresPass::ID = 0;
