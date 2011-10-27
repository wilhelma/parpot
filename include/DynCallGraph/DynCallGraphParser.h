//===-- DynCallGraph/DynCallGraphParser.h -DynCallGraph Pass - Interface --===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the DynCallGraphParserPass class. It is used to read data
// from a dynamic call graph file (.dot format). The class is derived from
// DynCallGraph which is also an interface to obtain the data.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DYNCALLGRAPH_DYNCALLGRAPHPARSERPASS_H_
#define PARPOT_DYNCALLGRAPH_DYNCALLGRAPHPARSERPASS_H_

#include "llvm//Pass.h"
#include "llvm/Module.h"
#include "DynCallGraph/DynCallGraph.h"
#include <vector>
#include <map>

namespace llvm {

  /// The class DynCallGraphParser reads a .dot file and offers an
  /// interface to retrieve the containing graph data.
	class DynCallGraphParserPass: public ModulePass, public DynCallGraph{
	public:
		static char ID; // Class identification, replacement for typeinfo
		DynCallGraphParserPass() : ModulePass(ID), DynCallGraph() { }

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.setPreservesAll();
		}
		virtual bool runOnModule(Module &M);

	private:
		bool fillGraph(const char *filename);
	};
}


#endif /* PARPOT_DYNCALLGRAPH_DYNCALLGRAPHPARSERPASS_H_ */
