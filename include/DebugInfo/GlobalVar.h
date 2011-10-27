//===---------- DebugInfo/GlobalVar.h - GlobalVar - Interface -------------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the GlobalVar class and the CompGlobalVar functor.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_GLOBALVAR_H_
#define PARPOT_DEBUGINFO_GLOBALVAR_H_

#include "DebugInfoTypes.h"
#include "DebugInfo.h"
#include <string>

namespace llvm {

  /// The GloblaVar class holds debug information for global variales.
	class GlobalVar: public DebugInfo {
		// attributes
		std::string name_;
		std::string displayName_;
		std::string type_;
		std::string compileUnit_;
		unsigned int lineNo_;

		// prohibit copy constructor
		GlobalVar(const GlobalVar&);
		GlobalVar& operator=(const GlobalVar*);
	public:
		GlobalVar(std::string name, std::string displayName,
					std::string type, std::string compileUnit,
					unsigned int lineNo):
			DebugInfo(TGlobalVar), name_(name), displayName_(displayName),
			type_(type), compileUnit_(compileUnit), lineNo_(lineNo) { }
		GlobalVar(std::string);

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
		std::string getCompileUnit() const {
			return compileUnit_;
		}
		unsigned int getLineNo() const {
			return lineNo_;
		}
	};

  /// The CompGlobalVar functor compares GlobalVar instances with strings.
	class CompGlobalVar {
		std::string cString_;
	public:
		CompGlobalVar(const std::string &comp): cString_(comp) { }
		bool operator()(const GlobalVar *pGVar) {
			return (pGVar->getName() == cString_);
		}
	};
}
#endif /* DEBUGINFO_GLOBALVAR_H_ */
