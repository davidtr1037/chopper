//===-- PhiCleaner.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/InstVisitor.h"
#include "llvm/DebugInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;
using namespace std;

char klee::ReturnToVoidFunctionPass::ID = 0;

bool klee::ReturnToVoidFunctionPass::runOnFunction(Function &f, Module &module) {
  // skip void functions
  if (f.getReturnType()->isVoidTy()) {
    return false;
  }

  bool changed = false;
  for (std::vector<Interpreter::SkippedFunctionOption>::const_iterator i = skippedFunctions.begin(); i != skippedFunctions.end(); i++) {
    if (string("__wrap_") + f.getName().str() == i->name) {
      Function *wrapper = createWrapperFunction(f, module);
      replaceCalls(&f, wrapper, i->line);
      changed = true;
    }
  }

  return changed;
}

Function *klee::ReturnToVoidFunctionPass::createWrapperFunction(Function &f, Module &module) {
  // create new function parameters: *return_var + original function's parameters
  vector<Type *> paramTypes;
  Type *returnType = f.getReturnType();
  paramTypes.push_back(PointerType::get(returnType, 0));
  paramTypes.insert(paramTypes.end(), f.getFunctionType()->param_begin(), f.getFunctionType()->param_end());

  // create new void function
  FunctionType *newFunctionType = FunctionType::get(Type::getVoidTy(getGlobalContext()), makeArrayRef(paramTypes), f.isVarArg());
  string wrappedName = string("__wrap_") + f.getName().str();
  Function *wrapper = cast<Function>(module.getOrInsertFunction(wrappedName, newFunctionType));

  // set the arguments' name: __result + original parameters' name
  vector<Value *> argsForCall;
  Function::arg_iterator i = wrapper->arg_begin();
  Value *resultArg = i++;
  resultArg->setName("__result");
  for (Function::arg_iterator j = f.arg_begin(); j != f.arg_end(); j++) {
    Value *origArg = j;
    Value *arg = i++;
    arg->setName(origArg->getName());
    argsForCall.push_back(arg);
  }

  // create basic block 'entry' in the new function
  BasicBlock *block = BasicBlock::Create(getGlobalContext(), "entry", wrapper);
  IRBuilder<> builder(block);

  // insert call to the original function
  Value *callInst = builder.CreateCall(&f, makeArrayRef(argsForCall), "__call");
  // insert store for the return value to __result parameter
  builder.CreateStore(callInst, resultArg);
  // terminate function with void return
  builder.CreateRetVoid();

  return wrapper;
}

void klee::ReturnToVoidFunctionPass::replaceCalls(Function *f, Function *wrapper, unsigned int line) {
  for (auto ui = f->use_begin(), ue = f->use_end(); ui != ue; ui++) {
    if (Instruction *inst = dyn_cast<Instruction>(*ui)) {
      if (inst->getParent()->getParent() == wrapper) {
        continue;
      }

      if (line != 0) {
        if (MDNode *N = inst->getMetadata("dbg")) {
          DILocation Loc(N);
          if (Loc.getLineNumber() != line) {
            continue;
          }
        }
      }

      if (isa<CallInst>(inst)) {
        replaceCall(dyn_cast<CallInst>(inst), f, wrapper);
      }
    }
  }
}

void klee::ReturnToVoidFunctionPass::replaceCall(CallInst *callInst, Function *f, Function *wrapper) {
  Value *allocaInst = NULL;
  StoreInst *prevStoreInst = NULL;
  for (auto ui = callInst->use_begin(), ue = callInst->use_end(); ui != ue; ui++) {
    if (StoreInst *storeInst = dyn_cast<StoreInst>(*ui)) {
      if (storeInst->getOperand(0) != callInst && isa<AllocaInst>(storeInst->getOperand(0))) {
        allocaInst = storeInst->getOperand(0);
        prevStoreInst = storeInst;
      } else if (storeInst->getOperand(1) != callInst && isa<AllocaInst>(storeInst->getOperand(1))) {
        allocaInst = storeInst->getOperand(1);
        prevStoreInst = storeInst;
      }
    }
  }

  IRBuilder<> builder(callInst);
  // insert alloca for return value
  if (!allocaInst)
    allocaInst = builder.CreateAlloca(f->getReturnType());

  // insert call for the wrapper function
  vector<Value *> argsForCall;
  argsForCall.push_back(allocaInst);
  for (unsigned int i = 0; i < callInst->getNumArgOperands(); i++) {
    argsForCall.push_back(callInst->getArgOperand(i));
  }
  builder.CreateCall(wrapper, makeArrayRef(argsForCall));

  if (prevStoreInst) {
    prevStoreInst->eraseFromParent();
  } else {
    Value *load = builder.CreateLoad(allocaInst);
    callInst->replaceAllUsesWith(load);
  }

  callInst->eraseFromParent();
}

bool klee::ReturnToVoidFunctionPass::runOnModule(Module &module) {
  // we assume to have everything linked inside the single .bc file
  bool dirty = false;
  for (Module::iterator f = module.begin(), fe = module.end(); f != fe; ++f)
    dirty |= runOnFunction(*f, module);

  return dirty;
}
