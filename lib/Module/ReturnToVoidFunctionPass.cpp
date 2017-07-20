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

    Function *klee::ReturnToVoidFunctionPass::createWrapperFunction(Function &f, Module &M) {
      // create new function
      Type *returnType = f.getReturnType();
      PointerType *newParamType = PointerType::get(returnType, 0);

      /* build parameter types */
      vector<Type *> paramTypes;
      paramTypes.push_back(newParamType);
      paramTypes.insert(paramTypes.end(), f.getFunctionType()->param_begin(), f.getFunctionType()->param_end());

      FunctionType *newFunctionType = FunctionType::get(Type::getVoidTy(getGlobalContext()), makeArrayRef(paramTypes), f.isVarArg());
      string wrappedName = string("__wrap_") + f.getName().str();
      Function *new_f = cast<Function>(M.getOrInsertFunction(wrappedName, newFunctionType));

      /* set the names of the arguments */
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

      BasicBlock *block = BasicBlock::Create(getGlobalContext(), "entry", new_f);
      IRBuilder<> builder(block);

      /* insert call */
      Value *callInst = builder.CreateCall(&f, makeArrayRef(argsForCall), "__call");
      /* insert store */
      builder.CreateStore(callInst, resultArg);
      /* insert ret */
      builder.CreateRetVoid();

      return new_f;
    }

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
      IRBuilder<> builder(callInst);
      /* insert alloca */
      Value *allocaInst = builder.CreateAlloca(f->getReturnType());

      /* insert call for the wrapped function */
      vector<Value *> argsForCall;
      argsForCall.push_back(allocaInst);
      for (unsigned int i = 0; i < callInst->getNumArgOperands(); i++) {
        argsForCall.push_back(callInst->getArgOperand(i));
      }
      builder.CreateCall(new_f, makeArrayRef(argsForCall));

      /* insert load*/
      Value *load = builder.CreateLoad(allocaInst);

      callInst->replaceAllUsesWith(load);
      callInst->eraseFromParent();
    }

    bool klee::ReturnToVoidFunctionPass::runOnModule(Module &M) {
      bool dirty = false;
      for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f)
        dirty |= runOnFunction(*f,M);

      return dirty;
    }
