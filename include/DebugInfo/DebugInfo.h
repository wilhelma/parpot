//===----------- DebugInfo/DebugInfo.h - DebugInfo - Interfaces -----------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the abstract DebugInfo class.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_DEBUGINFO_H_
#define PARPOT_DEBUGINFO_DEBUGINFO_H_

#include "llvm/Support/raw_ostream.h"
#include "DebugInfoTypes.h"
#include <string>
#include <vector>
#include <stdlib.h>
#include <stdio.h>

namespace llvm {

	// The abstract DebugInfo class provides store and load instructions for
	// the debug metadata.
	class DebugInfo {
		DebugInfoType debugType_;
	public:
		DebugInfo(DebugInfoType t): debugType_(t) { }
		virtual bool store(const char *filename) const = 0;

		static const char* getFileName() {
			return "llvmdbginfo.out";
		}

		static void terminate(std::string line) {
		    errs() << "Error: Line is corrupt\n" << line << '\n';
		    perror(0);
		    exit(1);
		}

		DebugInfoType getDebugType() const {
			return debugType_;
		}
	};
}


#endif /* DEBUGINFO_H_ */
