//===- Analysis/TimeProfileInfoLoader.h - Timeprofiling loader - Interfaces===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file defines the ProfileInfoLoader class, which is used to load and
// represent profiling information read from the dump file. If conversions
// between formats are needed, it can also do this.
//
//===----------------------------------------------------------------------===//

#ifndef PARPOT_ANALYSIS_TIMEPROFILEINFOLOADER_H
#define PARPOT_ANALYSIS_TIMEPROFILEINFOLOADER_H

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Format.h"
#include "Analysis/TimeProfileInfoLoader.h"
#include "Analysis/TimeProfileInfo.h"
#include "llvm/Analysis/ProfileInfo.h"
#include "llvm/Pass.h"

namespace llvm {
  
  /// The TimeProfileInfoLoader class may be used to get timing information
  /// of functions from a binary file.
  class TimeProfileInfoLoader {
    const std::string &Filename;
    Module &M;
    std::vector<std::string> CommandLines;
    std::vector<double>    FunctionTimes;
  public:
    // ProfileInfoLoader ctor - Read the specified profiling data file, exiting
    // the program if the file is invalid or broken.
    TimeProfileInfoLoader(const char *ToolName, const std::string &Filename,
                      Module &M);

    static const unsigned Uncounted;

    // todo: check if method is needed
    unsigned getNumExecutions() const {
      return CommandLines.size();
    }

    // todo: check if method is needed
    const std::string &getExecution(unsigned i) const {
      return CommandLines[i];
    }

    const std::string &getFileName() const { return Filename; }

    // getRawFunctionTimes - This method delivers execution times of functions.
    //
    const std::vector<double> &getRawFunctionTimes() const {
      return FunctionTimes;
    }
  };

  /// The TimeLoaderPass class declares a llvm-pass to read profiling 
  /// information from a binary file and offers this to other passes.
  class TimeLoaderPass: public ModulePass, public TimeProfileInfo {
  std::string Filename;
  unsigned ReadCount;

  public:
    static char ID; // Class identification, replacement for typeinfo
    explicit TimeLoaderPass(const std::string &filename = "")
      : ModulePass(ID), Filename(filename) {
      if (filename.empty()) Filename = "llvmtimeprof.out";
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
      AU.addRequired<ProfileInfo>();
    }

    virtual const char *getPassName() const {
      return "TimeProfiling information loader";
    }

    /// getAdjustedAnalysisPointer - This method is used when a pass implements
    /// an analysis interface through multiple inheritance.  If needed, it
    /// should override this to adjust the this pointer as needed for the
    /// specified pass info.
    virtual void *getAdjustedAnalysisPointer(AnalysisID ID) {
      if (ID == &ProfileInfo::ID)
        return (ProfileInfo*)this;
      return this;
    }

    /// run - Load the profile information from the specified file.
    virtual bool runOnModule(Module &M);

    virtual void print(raw_ostream &O, const Module *M) const;
  };

} // End llvm namespace

#endif
