#include <stdio.h>
#include <iostream>
#include <vector>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include "klee/Internal/Analysis/ReachabilityAnalysis.h"
#include "klee/Internal/Analysis/Inliner.h"

using namespace std;
using namespace llvm;

void Inliner::run() {
    if (functions.empty()) {
        return;
    }

    for (vector<string>::iterator i = targets.begin(); i != targets.end(); i++) {
        Function *entry = module->getFunction(*i);
        assert(entry);

        /* we can't use pointer analysis at this point... */
        set<Function *> reachable;
        ra->computeReachableFunctions(entry, false, reachable);
        for (set<Function *>::iterator i = reachable.begin(); i != reachable.end(); i++) {
            Function *f = *i;
            if (f->isDeclaration()) {
                continue;
            }

            inlineCalls(f, functions);
        }
    }
}

void Inliner::inlineCalls(Function *f, vector<string> functions) {
    vector<CallInst *> calls;

    for (inst_iterator i = inst_begin(f); i != inst_end(f); i++) {
        Instruction *inst = &*i;
        if (inst->getOpcode() != Instruction::Call) {
            continue;
        }

        CallInst *callInst = dyn_cast<CallInst>(inst);
        Function *calledFunction = callInst->getCalledFunction();
        if (!calledFunction) {
            /* TODO: handle aliases, ... */
            continue;
        }

        if (find(functions.begin(), functions.end(), calledFunction->getName().str()) == functions.end()) {
            continue;
        }

        calls.push_back(callInst);
    }

    for (vector<CallInst *>::iterator i = calls.begin(); i != calls.end(); i++) {
        CallInst *callInst = *i;

        /* inline function call */
        InlineFunctionInfo ifi;
        assert(InlineFunction(callInst, ifi)); 
    }
}
