//===------ DebugInfo/CallInstruction.h - CallInstruction - Interface -----===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the CallInstruction class.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_CALLINSTRUCTION_H_
#define PARPOT_DEBUGINFO_CALLINSTRUCTION_H_

#include "DebugInfoTypes.h"
#include "DebugInfo.h"
#include "llvm/Instruction.h"
#include <string>

namespace llvm {

  /// The CallInstruction class holds debug information for call instructions.
	class CallInstruction: public DebugInfo {
		// attributes
		unsigned int instNo_;
		unsigned int lineNo_;
		const Instruction *inst_;

		// prohibit copy constructor
		CallInstruction(const CallInstruction&);
		CallInstruction& operator=(const CallInstruction*);
	public:
		CallInstruction(unsigned int instNo,unsigned int lineNo):
		  DebugInfo(TCallInstruction), instNo_(instNo), lineNo_(lineNo) { }
		CallInstruction(std::string, const Instruction*);
		virtual bool store(const char *filename) const;
		unsigned int getInstNo() const { return instNo_; }
		unsigned int getLineNo() const { return lineNo_;	}
		const Instruction* getInstruction() const { return inst_; }
	};

  /// The CompCallInstruction functor compares CallInstruction instances with
	/// instruction pointer.
  class CompCallInstruction {
    const Instruction *inst_;
  public:
    CompCallInstruction(const Instruction *inst): inst_(inst) { }
    bool operator()(const CallInstruction *cInst) {
      return (cInst->getInstruction() == inst_);
    }
  };
}
#endif /* PARPOT_DEBUGINFO_CALLINSTRUCTION_H_ */
