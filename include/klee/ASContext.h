#ifndef KLEE_ASCONTEXT_H
#define KLEE_ASCONTEXT_H

#include "llvm/IR/Instruction.h"

#include "klee/Internal/Analysis/Cloner.h"

#include <vector>

namespace klee {

class ExecutionState;

class ASContext {
public:

    ASContext() : refCount(0) {

    }

    ASContext(Cloner *cloner, std::vector<llvm::Instruction *> &callTrace, llvm::Instruction *inst);
    
    ASContext(ASContext &other);

    bool operator==(ASContext &other);

    bool operator!=(ASContext &other);

    void dump();

    unsigned int refCount;

private:

    llvm::Instruction *getTranslatedInst(Cloner *cloner, llvm::Instruction *inst);

    std::vector<llvm::Instruction *> trace;
};

}

#endif
