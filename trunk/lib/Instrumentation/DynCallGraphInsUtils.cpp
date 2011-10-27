//===- DynCallGraphInsUtils.cpp - Helper functions for dyn. callgraph ins. ===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file implements a few helper functions which are used by dynamic call
// graph instrumentation code to instrument the code.
//
//===----------------------------------------------------------------------===//

#include "Instrumentation/DynCallGraphInsUtils.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Instruction.h"
#include "llvm/LLVMContext.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"

void llvm::insertPrepareCallGraph(Function *mainFn, const char *fnName) {
  LLVMContext &context = mainFn->getContext();
  BasicBlock *entry = mainFn->begin();
  BasicBlock::iterator insertPos = entry->begin();

  Type *ArgVTy = PointerType::getUnqual(Type::getInt8PtrTy(context));
  Module &M = *mainFn->getParent();
  Constant *InitFn = M.getOrInsertFunction(fnName, Type::getVoidTy(context),
                                           Type::getInt32Ty(context),
                                           ArgVTy, (Type *)0);
  // This could force argc and argv into programs that wouldn't otherwise have
  // them, but instead we just pass null values in.
  std::vector<Value*> Args(2);
  Args[0] = Constant::getNullValue(Type::getInt32Ty(context));
  Args[1] = Constant::getNullValue(ArgVTy);

  std::vector<Constant*> GEPIndices(2,
                            Constant::getNullValue(Type::getInt32Ty(context)));

  Instruction *InitCall =
  		CallInst::Create(InitFn, makeArrayRef(Args), "", insertPos);


  // If argc or argv are not available in main, just pass null values in.
  Function::arg_iterator AI;
  switch (mainFn->arg_size()) {
  default:
  case 2:
    AI = mainFn->arg_begin(); ++AI;
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
    AI = mainFn->arg_begin();
    // If the program looked at argc, have it look at the return value of the
    // init call instead.
    if (!AI->getType()->isIntegerTy(32)) {
      Instruction::CastOps opcode;
      opcode = CastInst::getCastOpcode(AI, true,
                                       Type::getInt32Ty(context), true);
      InitCall->setOperand(1,
          CastInst::Create(opcode, AI, Type::getInt32Ty(context),
                           "argc.cast", InitCall));
    } else {
      InitCall->setOperand(1, AI);
    }

  case 0: break;
  }
}

void llvm::addNotifyCall(CallSite *cs, BasicBlock *bb, const char *fnNameBefore,
    const char *fnNameAfter, GlobalVariable *gv, unsigned fnNum) {
  // get corresponding module and context
  Module &mod = *bb->getParent()->getParent();
  LLVMContext &context = bb->getContext();

  // insert instruction before call site
  BasicBlock::iterator insertPos = cs->getInstruction();

  //call i32 @fnNameBefore(i8 *getelementptr([%d x i8] *@gv, i32 0, i32 0), i)
  {
    // call function before
    const Type *Int8Ptr = PointerType::getUnqual(IntegerType::getInt8Ty(context));
    Constant *callFn = mod.getOrInsertFunction(fnNameBefore,
                                  Type::getVoidTy(context),
                                  Int8Ptr, Type::getInt32Ty(context),
                                  (Type *)0);

    Constant *zero = Constant::getNullValue(IntegerType::getInt32Ty(context));

    Constant *gep_params[] = {
      zero,
      zero
    };

    std::vector<Value*> Args(2);
    Args[0] = ConstantExpr::getGetElementPtr(gv, gep_params, 2);
    Args[1] = ConstantInt::get(Type::getInt32Ty(context), fnNum);

    CallInst::Create(callFn, makeArrayRef(Args), "", insertPos);
  }

  //call i32 @fnNameAfter(i8 *getelementptr([%d x i8] *@gv, i32 0, i32 0), i)
  {
    // call function after
    const Type *Int8Ptr = PointerType::getUnqual(IntegerType::getInt8Ty(context));
    Constant *callFn = mod.getOrInsertFunction(fnNameAfter,
                                  Type::getVoidTy(context),
                                  Int8Ptr, Type::getInt32Ty(context),
                                  (Type *)0);

    Constant *zero = Constant::getNullValue(IntegerType::getInt32Ty(context));

    Constant *gep_params[] = {
      zero,
      zero
    };

    std::vector<Value*> Args(2);
    Args[0] = ConstantExpr::getGetElementPtr(gv, gep_params, 2);
    Args[1] = ConstantInt::get(Type::getInt32Ty(context), fnNum);

    if (cs->isInvoke()) {
      InvokeInst *invokeInst = dyn_cast<InvokeInst>(cs->getInstruction());
      insertPos = invokeInst->getNormalDest()->getFirstNonPHI();
      CallInst::Create(callFn, makeArrayRef(Args), "", insertPos);
      insertPos = invokeInst->getUnwindDest()->getFirstNonPHI();
      CallInst::Create(callFn, makeArrayRef(Args), "", insertPos);
    } else {
      insertPos++;
      CallInst::Create(callFn, makeArrayRef(Args), "", insertPos);
    }
  }
}

void llvm::addNotifyFnCalled(Function *fn, const char *fnName,
    GlobalVariable *gv) {
  LLVMContext &context = fn->getContext();
  BasicBlock::iterator insertPos = fn->begin()->getFirstNonPHI();

  //call i32 @fnName(i8 *getelementptr([%d x i8] *@gv, i32 0, i32 0), i)
  {
    // call function before
    const Type *Int8Ptr = PointerType::getUnqual(
        IntegerType::getInt8Ty(context));
    Constant *callFn = fn->getParent()->getOrInsertFunction(fnName,
                                  Type::getVoidTy(context),
                                  Int8Ptr, Type::getInt32Ty(context),
                                  (Type *)0);

    Constant *zero = Constant::getNullValue(IntegerType::getInt32Ty(context));

    Constant *gep_params[] = {
      zero,
      zero
    };

    std::vector<Value*> Args(2);
    Args[0] = ConstantExpr::getGetElementPtr(gv, gep_params, 2);
    Args[1] = ConstantInt::get(Type::getInt32Ty(context), 0);

    CallInst::Create(callFn, makeArrayRef(Args), "", insertPos);
  }
}
