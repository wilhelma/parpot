//===- TimeProfilingUtils.cpp - Helper functions shared by profilers ------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file implements a few helper functions which are used by profile
// instrumentation code to instrument the code.  This allows the profiler pass
// to worry about *what* to insert, and these functions take care of *how* to do
// it.
//
//===----------------------------------------------------------------------===//

#include "Instrumentation/TimeProfilingUtils.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"

void llvm::InsertTimeProfilingInitCall(Function *MainFn, const char *FnName,
                                   GlobalValue *Array) {
  LLVMContext &Context = MainFn->getContext();
  Type *ArgVTy = PointerType::getUnqual(Type::getInt8PtrTy(Context));
  PointerType *UIntPtr = Type::getDoublePtrTy(Context);
  Module &M = *MainFn->getParent();
  Constant *InitFn = M.getOrInsertFunction(FnName, Type::getInt32Ty(Context),
                                           Type::getInt32Ty(Context),
                                           ArgVTy, UIntPtr,
                                           Type::getInt32Ty(Context),
                                           (Type *)0);

  // This could force argc and argv into programs that wouldn't otherwise have
  // them, but instead we just pass null values in.
  std::vector<Value*> Args(4);
  Args[0] = Constant::getNullValue(Type::getInt32Ty(Context));
  Args[1] = Constant::getNullValue(ArgVTy);


  BasicBlock *Entry = MainFn->begin();
  BasicBlock::iterator InsertPos = Entry->begin();
  // Skip over any allocas in the entry block.
  while (isa<AllocaInst>(InsertPos)) ++InsertPos;

  std::vector<Constant*> GEPIndices(2,
                            Constant::getNullValue(Type::getInt32Ty(Context)));
  unsigned NumElements = 0;

  if (Array) {
    Args[2] = ConstantExpr::getGetElementPtr(Array,
    		makeArrayRef(&GEPIndices[0], GEPIndices.size()));
    NumElements =
      cast<ArrayType>(Array->getType()->getElementType())->getNumElements();
  } else {
    // If this profiling instrumentation doesn't have a constant array, just
    // pass null.
    Args[2] = ConstantPointerNull::get(UIntPtr);
  }
  Args[3] = ConstantInt::get(Type::getInt32Ty(Context), NumElements);

  Instruction *InitCall = CallInst::Create(InitFn,
  		makeArrayRef(Args), "newargc", InsertPos);

  // If argc or argv are not available in main, just pass null values in.
  Function::arg_iterator AI;
  switch (MainFn->arg_size()) {
  default:
  case 2:
    AI = MainFn->arg_begin(); ++AI;
    if (AI->getType() != ArgVTy) {
      Instruction::CastOps opcode = CastInst::getCastOpcode(AI, false, ArgVTy,
                                                            false);
      InitCall->setOperand(2,
          CastInst::Create(opcode, AI, ArgVTy, "argv.cast", InitCall));
    } else {
      InitCall->setOperand(2, AI);
    }
    /* FALL THROUGH */

  case 1:
    AI = MainFn->arg_begin();
    // If the program looked at argc, have it look at the return value of the
    // init call instead.
    if (!AI->getType()->isIntegerTy(32)) {
      Instruction::CastOps opcode;
      if (!AI->use_empty()) {
        opcode = CastInst::getCastOpcode(InitCall, true, AI->getType(), true);
        AI->replaceAllUsesWith(
          CastInst::Create(opcode, InitCall, AI->getType(), "", InsertPos));
      }
      opcode = CastInst::getCastOpcode(AI, true,
                                       Type::getInt32Ty(Context), true);
      InitCall->setOperand(1,
          CastInst::Create(opcode, AI, Type::getInt32Ty(Context),
                           "argc.cast", InitCall));
    } else {
      AI->replaceAllUsesWith(InitCall);
      InitCall->setOperand(1, AI);
    }

  case 0: break;
  }
}

void llvm::AddGetTimesInFunction(Function *f, const char *FnName,
                              unsigned CounterNum, GlobalValue *TimerArray) {

  // Insert the getTime as first instructions...
  BasicBlock::iterator insertPos = f->begin()->getFirstNonPHI();
  Module &M = *f->getParent();
  LLVMContext &context = f->begin()->getContext();

  // Create the getelementptr constant expression
  std::vector<Constant*> Indices(2);
  Indices[0] = Constant::getNullValue(Type::getInt32Ty(context));
  Indices[1] = ConstantInt::get(Type::getInt32Ty(context), CounterNum);
  Constant *ElementPtr = ConstantExpr::getGetElementPtr(TimerArray,
  		makeArrayRef(&Indices[0], Indices.size()));

  // get time at the beginning
  Value *val = new LoadInst(ElementPtr, "OldFuncTimer", insertPos);
  Constant *GetFn = M.getOrInsertFunction(FnName, Type::getDoubleTy(context),
      (Type *)0);
  Instruction *before = CallInst::Create(GetFn, "timebefore", insertPos);

  // get time at the ends
  for (inst_iterator it = inst_begin(f), e = inst_end(f); it != e; ++it) {
    if (isa<ReturnInst>(&*it) || isa<UnreachableInst>(&*it)) {
      insertPos = &*it;
      LLVMContext &context2 = (*it).getContext();

      // Create the getelementptr constant expression
      std::vector<Constant*> Indices(2);
      Indices[0] = Constant::getNullValue(Type::getInt32Ty(context2));
      Indices[1] = ConstantInt::get(Type::getInt32Ty(context2), CounterNum);
      Constant *ElementPtr = ConstantExpr::getGetElementPtr(TimerArray,
                        makeArrayRef(&Indices[0], Indices.size()));
      Constant *GetFn = M.getOrInsertFunction(FnName, Type::getDoubleTy(context2),
          (Type *)0);
      Instruction *GetTimeAfter = CallInst::Create(GetFn, "timeafter", insertPos);
      Value *Delay = BinaryOperator::Create(Instruction::FSub, GetTimeAfter,
                                  before, "Delay", insertPos);
      Value *NewVal = BinaryOperator::Create(Instruction::FAdd, val,
                                  Delay, "NewFuncTimer", insertPos);
      new StoreInst(NewVal, ElementPtr, insertPos);
    }
  }
}
