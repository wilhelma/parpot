////===--- AnalyzeTimeProfile.cpp - Analyze time profiling - Implementation -===//
////
////                 ParPot - Parallelization Potential - Measurement
////
////===----------------------------------------------------------------------===//
////
//// This file defines the AnalyzeTimeProfile class, a module pass that uses
//// time profiling information as well as execution counts to build some
//// heuristics. Afterwards a static interprocedural analysis will be used to
//// give some advice about the parallelization potential of functions.
////
//// The pass is registered under the name "timeprof".
////
////===----------------------------------------------------------------------===//
//#define DEBUG_TYPE "timeprof"
//#include "llvm/Analysis/AliasAnalysis.h"
//#include "llvm/Analysis/CallGraph.h"
//#include "llvm/Analysis/Passes.h"
//#include "Analysis/PriorizeHandler.h"
//#include "Analysis/TimeProfileInfo.h"
//#include "Analysis/TimeProfileInfoLoader.h"
//#include "llvm/Function.h"
//#include "llvm/Module.h"
//#include "Analysis/CountStoresPass.h"
//#include "llvm/Support/raw_ostream.h"
//#include "llvm/Support/Debug.h"
//#include "llvm/Analysis/DebugInfo.h"
//#include "llvm/Analysis/ProfileInfo.h"
//#include "DynCallGraph/DynCallGraphParser.h"
//#include "Analysis/ParPot.h"
//#include <dsa/DataStructure.h>
//#include <dsa/DSGraph.h>
//#include <vector>
//
//using namespace llvm;
//
//namespace {
//
//class AnalyzeTimeProfile: public ModulePass {
//  PriorizeByStores *pStores;
//  PriorizeByTime *pTime;
//  PriorizeBySinglePar *pSPar;
//  TimeProfileInfo *tpi;
//  ProfileInfo *pi;
//  ParPot *pp_;
//
//  std::vector<TimeProfileInfo::Priority> functions_;
//
//public:
//  static char ID; // Class identification, replacement for typeinfo
//  AnalyzeTimeProfile() : ModulePass(&ID) { }
//
//  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
//      AU.setPreservesAll();
//      AU.addRequired<CallGraph>();
//      AU.addRequired<EQTDDataStructures>();
//      //AU.addRequired<EquivBUDataStructures>();
//      AU.addRequired<CountStoresPass>();
//      AU.addRequired<TimeLoaderPass>();
//      AU.addRequired<DynCallGraphParserPass>();
//  }
//
//  void print(DynCallGraphNode* node, unsigned count, int level) {
//    for (int i=0; i<level; ++i)
//      errs() << "  ";
//    errs() << node->getNum() << ": " << node->getName() << " " << count << '\n';
//    for (DynCallGraphNode::iterator it = node->begin(), e = node->end();
//          it != e; ++it)
//      print (it->first, it->second, level+1);
//  }
//
//  virtual bool runOnModule(Module &M) {
//
//    // prepare chain
//    pSPar = new PriorizeBySinglePar(0);
//    pStores = new PriorizeByStores(
//        *getAnalysisIfAvailable<CountStoresPass>(), pSPar);
//    pTime = new PriorizeByTime(pStores);
//
//    // load profile information
//    tpi = &getAnalysis<TimeLoaderPass>();
//
//    // print time profile information
//    DEBUG(
//      for (Module::const_iterator it = M.begin();
//          it != M.end(); ++it)
//        errs() << "Timing for: " << it->getName() << " "
//               << tpi->getExecutionTime(it) << '\n';
//    );
//
//    // do the priorization work
//    functions_ = pTime->priorize(getAnalysis<CallGraph>(), *tpi);
//
//    pp_ = new ParPot(&M,
//                     &getAnalysis<DynCallGraphParserPass>(),
//                     &getAnalysis<EQTDDataStructures>(),
//                     //&getAnalysis<EquivBUDataStructures>(),
//                     &getAnalysis<CallGraph>(),
//                     &functions_);
//    pp_->analyzeDependencies();
//
//    return false;
//  }
//
//  virtual void print(raw_ostream &O, const Module *M) const {
//    for (std::vector<TimeProfileInfo::Priority>::const_iterator it =
//          functions_.begin(), e = functions_.end(); it != e; ++it) {
//        std::string fName;
//      if (it->first) {
//        fName = it->first->getName();
//      }
//      else
//        fName = "External Function";
//    O << "Procedure " << fName << ": " << it->second << " Points" << '\n';
//      }
//
//    // print parpot results
//    pp_->printResult(O);
//
//  }
//};
//}
//
//// Register this pass...
//char AnalyzeTimeProfile::ID = 0;
//static RegisterPass<AnalyzeTimeProfile>
//X("parpot-analysis", "Parallelization potential measurement", false, true);
