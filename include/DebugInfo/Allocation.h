//===------ DebugInfo/Allocation.h - Allocation Debuginfo - Interfaces-----===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the Allocation class, which contains debug information for
// allocation instructions of llvm-bytecode. Additionally a functor is defined
// to compare allocation instances with strings.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_ALLOCATIONS_H_
#define PARPOT_DEBUGINFO_ALLOCATIONS_H_

#include "DebugInfoTypes.h"
#include "DebugInfo.h"
#include <string>

namespace llvm {

  /// The Allocation class holds debug information for allocation instructions.
	class Allocation: public DebugInfo {
		// attributes
		std::string name_;
		std::string displayName_;
		std::string type_;
		std::string subprogram_;
		std::string compileUnit_;
		unsigned int lineNo_;

		// prohibit copy constructor
		Allocation(const Allocation&);
		Allocation& operator=(const Allocation*);
	public:
		Allocation(std::string name, std::string displayName,
					std::string type, std::string subprogram,
					std::string compileUnit, unsigned int lineNo):
			DebugInfo(TAllocation), name_(name), displayName_(displayName),
			type_(type), subprogram_(subprogram), compileUnit_(compileUnit),
			lineNo_(lineNo) { }

		Allocation(std::string);

		virtual bool store(const char *filename) const;

		std::string getName() const {
			return name_;
		}
		std::string getDisplayName() const {
			return displayName_;
		}
		std::string getType() const {
			return type_;
		}
		std::string getSubprogram() const {
			return subprogram_;
		}
		std::string getCompileUnit() const {
			return compileUnit_;
		}
		unsigned int getLineNo() const {
			return lineNo_;
		}
	};

  /// The CompAllocation functor compares Allocation instances with strings.
	class CompAllocation {
		std::string cString_;
	public:
		CompAllocation(const std::string &comp): cString_(comp) { }
		bool operator()(const Allocation *pAlloc) {
			return (pAlloc->getName() == cString_);
		}
	};
}
#endif /* DEBUGINFO_ALLOCATIONS_H_ */
