//===-- DynCallGraphParser.cpp -DynCallGraphParser Pass - Implementation --===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the DynCallGraphParser pass.
//
//===----------------------------------------------------------------------===//
#include "DynCallGraph/DynCallGraphParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instructions.h"
#include <assert.h>
#include <set>

using namespace llvm;

bool DynCallGraphParserPass::fillGraph(const char *filename) {
  
  // declarations
  std::string line, node, succ, name, num, count, sExTime;
  size_t pos, end;
  unsigned nodeID = 0, succID = 0, number = 0, countNo;
  double exTime;
  
  // open file
  std::ifstream file(filename, std::ios::in);
  if (!file) {
      errs() << "Error: Can't open file " << filename << '\n';
      return false;
  }

  // read file line by line
  while(file) {
    line.clear();
    if (!getline(file, line)) break; // file completely read

    if (line.compare(0, 5, "\tNode") == 0) {
      end = line.find(" ", 5);
      node = line.substr(5, end - 5); // get node number
      std::stringstream sNodeID(node);
      sNodeID >> nodeID;

      // add nodes
      if ((pos = line.find("\"{", 0)) != std::string::npos) {
        pos += 2; // shift by 2 chars to begin of label
        end = line.find(";", pos);
        name = line.substr(pos, end - pos);
        pos = end + 1;
        end = line.find(";", pos);
        num = line.substr(pos, end - pos);
        std::stringstream sNum(num);
        sNum >> number;
        pos = end + 1;
        end = line.find("}", pos);
        sExTime = line.substr(pos, end - pos);
        std::stringstream sTime(sExTime);
        sTime >> exTime;
        addNode(nodeID, name, number, exTime);
      }

      // add edges
      if ((pos = line.find("->", 0)) != std::string::npos) {
        pos += 7; // shift by 7 chars to begin of succ-node
        end = line.find(" ", pos);
        succ = line.substr(pos, end - pos);
        assert((pos = line.find("label", end)) != std::string::npos
            && "Wrong file format!");
        pos += 7;
        end = line.find("\"", pos);
        count = line.substr(pos, end - pos);

        std::stringstream sCount(count);
        sCount >> countNo;
        std::stringstream sSucc(succ);
        sSucc >> succID;
        assert(addEdge(nodeID, succID, countNo)
            && "Can't create edge in dyncallgraph. Edge doesn't exist!");
      }
    }
  }

  return true;
}

bool DynCallGraphParserPass::runOnModule(Module &M) {
	errs() << "oleee\n";
  fillGraph("dyncallgraph.dot");

  // retrieve main function
  Function *Main = M.getFunction("main");
  if (Main == 0) {
    errs() << "WARNING: module has no main function!\n";
    return false;
  }

  // filter functions that shall be instrumented
  std::set<Function *> FunctionsToConsider;
  unsigned NumFunctions = 0;
  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
    if (F->isDeclaration()) continue;
    ++NumFunctions;
    FunctionsToConsider.insert(F);
  }

  // find each call-/invoke-instruction and create link
  unsigned int i = 1; // 0 is for main function
  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F)
    if (FunctionsToConsider.count(F))

      // add notify-call instructions
      for (inst_iterator it = inst_begin(F), e = inst_end(F); it != e; ++it) {
        CallSite *pCS;
        if (CallInst *callInst = dyn_cast<CallInst>(&*it))
          pCS = new CallSite(callInst);
        else if (InvokeInst *invokeInst = dyn_cast<InvokeInst>(&*it))
          pCS = new CallSite(invokeInst);
        else
          continue;

        linkInstruction(i, &*it); // link instruction to dyncallgraph node
        i++; // increment function count
      }

  return false;
}

// Register the DynCallGraphParserPass pass...
char DynCallGraphParserPass::ID = 0;
static RegisterPass<DynCallGraphParserPass>
X("parpot-dyncallgraphreader", "Dynamic call graph reader", false, true);
