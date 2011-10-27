//===----- DebugInfo/CompileUnit.h - Compile-unit debuginfo - Interfaces---===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the CompileUnit class, which contains debug information for
// compile units of applications compiled by the llvm framework . Additionally a
// functor is defined to compare CompileUnit instances with strings.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_COMPILEUNIT_H_
#define PARPOT_DEBUGINFO_COMPILEUNIT_H_

#include "DebugInfoTypes.h"
#include "DebugInfo.h"
#include <string>

namespace llvm {

  /// The CompileUnit class holds debug information for compile units of
  /// applications that are compiled by the llvm framework.
	class CompileUnit: public DebugInfo {

		// attributes
		std::string filename_;
		std::string directory_;

		// prohibit copy constructor
		CompileUnit(const CompileUnit&);
		CompileUnit& operator=(const CompileUnit*);
	public:
		CompileUnit(std::string file, std::string dir):
			DebugInfo(TCompileUnit), filename_(file), directory_(dir) {}
		CompileUnit(std::string);

		virtual bool store(const char *filename) const;
		std::string getFilename() const {
			return filename_;
		}
		std::string getDirectory() const {
			return directory_;
		}
	};

  /// The CompComileUnit functor compares the full path of a compile unit wiht
  /// an instance of the corresponding class.
	class CompCompileUnit {
		std::string cString_;
	public:
		CompCompileUnit(const std::string &comp): cString_(comp) { }
		bool operator()(const CompileUnit *pCUnit) {
			std::string fPath = pCUnit->getDirectory()
					+ "/" + pCUnit->getFileName();
			return (fPath == cString_);
		}
	};
}


#endif /* COMPILEUNIT_H_ */
