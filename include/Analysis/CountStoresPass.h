//=== Analysis/CountstoresPass.h - Count store instructions - Interface ---===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the class CountStoresPass which counts the number of sotre
// instructions by functions.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_ANALYSIS_COUNTSTORESPASS_H_
#define PARPOT_ANALYSIS_COUNTSTORESPASS_H_

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <map>

namespace llvm {

    class CountStoresPass: public ModulePass {
    	std::map<const Function*, int> storeInstructions;

    public:
    	static const int MISSING = -1;
    	static const int UNCRITICAL = 2;
    	static const int LOW = 5;
    	static const int MID = 10;
    	static const int HIGH = 20;
    	static const int CRITICAL = 50;

        static char ID; // Class identification, replacement for typeinfo
        CountStoresPass() : ModulePass(ID) {}

        virtual void getAnalysisUsage(AnalysisUsage &AU) const {
            AU.setPreservesAll();
        }

        virtual bool runOnModule(Module&);

        /// getStores() returns the number of store instruction of a function
        int getStores(const Function*);
    };
}

#endif /* COUNTSTORESPASS_H_ */
