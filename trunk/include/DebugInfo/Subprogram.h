//===---------- DebugInfo/Subprogram.h - Subprogram - Interface -----------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the Subprogram class and the CompSubprogram functor.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_SUBPROGRAM_H_
#define PARPOT_DEBUGINFO_SUBPROGRAM_H_

#include "DebugInfoTypes.h"
#include "DebugInfo.h"
#include <string>

namespace llvm {

  /// The Subprogram class holds debug information for procedures of
  /// applications that are compiled by the llvm framework.
	class Subprogram: public DebugInfo {
		// attributes
		std::string name_;
		std::string displayName_;
		std::string compileUnit_;
		unsigned int lineNo_;

		// prohibit copy constructor
		Subprogram(const Subprogram&);
		Subprogram& operator=(const Subprogram*);
	public:
		Subprogram(std::string name, std::string displayName,
				   std::string compileUnit, unsigned int lineNo):
			DebugInfo(TSubprogram), name_(name), displayName_(displayName),
			compileUnit_(compileUnit), lineNo_(lineNo) { }
		Subprogram(std::string);

		virtual bool store(const char *filename) const;
		std::string getName() const {
			return name_;
		}
		std::string getDisplayName() const {
			return displayName_;
		}
		std::string getCompileUnit() const {
			return compileUnit_;
		}
		unsigned int getLineNo() const {
			return lineNo_;
		}
	};

  /// The CompSubprogram functor compares Subprogram instances with strings.
	class CompSubprogram {
		std::string cString_;
	public:
		CompSubprogram(const std::string &comp): cString_(comp) { }
		bool operator()(const Subprogram *pSProg) {
			return (pSProg->getName() == cString_);
		}
	};
}
#endif /* COMPILEUNIT_H_ */
