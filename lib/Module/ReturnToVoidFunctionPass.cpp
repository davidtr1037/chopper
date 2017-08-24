//===-- ReturnToVoidFunctionPass.cpp --------------------------------------===//
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
#include "llvm/Support/CFG.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
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
      replaceCalls(&f, wrapper, i->lines);
      changed = true;
    }
  }

  return changed;
}

/// We replace a returning function f with a void __wrap_f function that:
///  1- takes as first argument a variable __result that will contain the result
///  2- calls f and stores the return value in __result
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

/// Replaces calls to f with the wrapper function __wrap_f
/// The replacement will occur at all call sites only if the user has not specified a given line in the '-skip-functions' options
void klee::ReturnToVoidFunctionPass::replaceCalls(Function *f, Function *wrapper, const vector<unsigned int> &lines) {
  vector<CallInst*> to_remove;
  for (auto ui = f->use_begin(), ue = f->use_end(); ui != ue; ui++) {
    if (Instruction *inst = dyn_cast<Instruction>(*ui)) {
      if (inst->getParent()->getParent() == wrapper) {
        continue;
      }

      if (!lines.empty()) {
        if (MDNode *N = inst->getMetadata("dbg")) {
          DILocation Loc(N);
          if (find(lines.begin(), lines.end(), Loc.getLineNumber()) == lines.end()) {
            continue;
          }
        }
      }

      if (CallInst *call = dyn_cast<CallInst>(inst)) {
        replaceCall(call, f, wrapper);
        to_remove.push_back(call);
      }
    }
  }
  for (auto ci = to_remove.begin(), ce = to_remove.end(); ci != ce; ci++) {
    (*ci)->eraseFromParent();
  }
}

/// We replace a given CallInst to f with a new CallInst to __wrap_f
/// If the original return value was used in a StoreInst, we use directly such variable, instead of creating a new one
void klee::ReturnToVoidFunctionPass::replaceCall(CallInst *origCallInst, Function *f, Function *wrapper) {
  Value *allocaInst = NULL;
  StoreInst *prevStoreInst = NULL;
  bool hasPhiUse = false;

  // We can perform this optimization only when the return value is stored, and
  // that is the _only_ use
  if (origCallInst->getNumUses() == 1) {
    for (auto ui = origCallInst->use_begin(), ue = origCallInst->use_end();
         ui != ue; ui++) {
      if (StoreInst *storeInst = dyn_cast<StoreInst>(*ui)) {
        if (storeInst->getOperand(0) == origCallInst &&
            isa<AllocaInst>(storeInst->getOperand(1))) {
          allocaInst = storeInst->getOperand(1);
          prevStoreInst = storeInst;
        }
      }
    }
  }

  /* check if we have a PHI use */
  for (auto ui = origCallInst->use_begin(), ue = origCallInst->use_end(); ui != ue; ui++) {
    if (isa<PHINode>(*ui)) {
      hasPhiUse = true;
    }
  }

  IRBuilder<> builder(origCallInst);
  // insert alloca for return value
  if (!allocaInst)
    allocaInst = builder.CreateAlloca(f->getReturnType());

  // insert call for the wrapper function
  vector<Value *> argsForCall;
  argsForCall.push_back(allocaInst);
  for (unsigned int i = 0; i < origCallInst->getNumArgOperands(); i++) {
    argsForCall.push_back(origCallInst->getArgOperand(i));
  }
  CallInst *callInst = builder.CreateCall(wrapper, makeArrayRef(argsForCall));
  callInst->setDebugLoc(origCallInst->getDebugLoc());

  // if there was a StoreInst, we remove it
  if (prevStoreInst) {
    prevStoreInst->eraseFromParent();
  } else {
    // otherwise, we create a LoadInst for the return value at each use
    if (hasPhiUse) {
      // FIXME: phi nodes are not easy to handle: 1) we can't add the load as
      // first instruction of the basic block, 2) we need to find a
      // precedessor which dominates all the uses.
      // now relying on unoptimized creation of load
      Value *load = builder.CreateLoad(allocaInst);
      origCallInst->replaceAllUsesWith(load);
    } else {
      while (origCallInst->getNumUses() > 0) {
        llvm::Instruction *II = cast<llvm::Instruction>(*origCallInst->use_begin());
        IRBuilder<> builder_use(II);
        Value *load = builder_use.CreateLoad(allocaInst);
        II->replaceUsesOfWith(origCallInst, load);
      }
    }
  }
}

bool klee::ReturnToVoidFunctionPass::runOnModule(Module &module) {
  // we assume to have everything linked inside the single .bc file
  bool dirty = false;
  for (Module::iterator f = module.begin(), fe = module.end(); f != fe; ++f)
    dirty |= runOnFunction(*f, module);

  return dirty;
}
