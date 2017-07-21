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

bool klee::ReturnToVoidFunctionPass::runOnFunction(Function &f, Module &M) {
	  // skip void functions
      if (f.getReturnType()->isVoidTy()) {
    	return false;
      }

      bool changed = false;
      for (std::vector<Interpreter::SkippedFunctionOption>::const_iterator i = skippedFunctions.begin(); i != skippedFunctions.end(); i++) {
    	if (string("__wrap_") + f.getName().str() == i->name) {
    	  Function *new_f = createWrapperFunction(f, M);
		  replaceCalls(&f, new_f, i->line);
		  changed = true;
    	}
      }
      return changed;
    }

    /// We replace a returning function f with a void __wrap_f function that:
    ///  1- takes as first argument a variable __result that will contain the result
    ///  2- calls f and stores the return value in __result
    Function *klee::ReturnToVoidFunctionPass::createWrapperFunction(Function &f, Module &M) {
      // create new function parameters: *return_var + original function's parameters
      vector<Type *> paramTypes;
      Type *returnType = f.getReturnType();
      paramTypes.push_back(PointerType::get(returnType, 0));
      paramTypes.insert(paramTypes.end(), f.getFunctionType()->param_begin(), f.getFunctionType()->param_end());

      // create new void function
      FunctionType *newFunctionType = FunctionType::get(Type::getVoidTy(getGlobalContext()), makeArrayRef(paramTypes), f.isVarArg());
      string wrappedName = string("__wrap_") + f.getName().str();
      Function *new_f = cast<Function>(M.getOrInsertFunction(wrappedName, newFunctionType));

      // set the arguments' name: __result + original parameters' name
      vector<Value *> argsForCall;
      Function::arg_iterator i = new_f->arg_begin();
      Value *resultArg = i++;
      resultArg->setName("__result");
      for (Function::arg_iterator j = f.arg_begin(); j != f.arg_end(); j++) {
        Value *origArg = j;
        Value *arg = i++;
        arg->setName(origArg->getName());
        argsForCall.push_back(arg);
      }

      // create basic block 'entry' in the new function
      BasicBlock *block = BasicBlock::Create(getGlobalContext(), "entry", new_f);
      IRBuilder<> builder(block);

      // insert call to the original function
      Value *callInst = builder.CreateCall(&f, makeArrayRef(argsForCall), "__call");
      // insert store for the return value to __result parameter
      builder.CreateStore(callInst, resultArg);
      // terminate function with void return
      builder.CreateRetVoid();

      return new_f;
    }

    /// Replaces calls to f with the wrapper function __wrap_f
    /// The replacement will occur at all call sites only if the user has not specified a given line in the '-skip-functions' options
    void klee::ReturnToVoidFunctionPass::replaceCalls(Function *f, Function *new_f, unsigned int line) {
      for (auto ui = f->use_begin(), ue = f->use_end(); ui != ue; ui++) {
        if (Instruction *inst = dyn_cast<Instruction>(*ui)) {
          if (inst->getParent()->getParent() == new_f) {
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
            replaceCall(dyn_cast<CallInst>(inst), f, new_f);
          }
        }
      }
    }

    void klee::ReturnToVoidFunctionPass::replaceCall(CallInst *callInst, Function *f, Function *new_f) {
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
      builder.CreateCall(new_f, makeArrayRef(argsForCall));

      if (prevStoreInst) {
        prevStoreInst->eraseFromParent();
      } else {
        Value *load = builder.CreateLoad(allocaInst);
        callInst->replaceAllUsesWith(load);
      }

      callInst->eraseFromParent();
    }

    bool klee::ReturnToVoidFunctionPass::runOnModule(Module &M) {
      // we assume to have everything linked inside the single .bc file
      bool dirty = false;
      for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f)
        dirty |= runOnFunction(*f,M);

      return dirty;
    }
