#include "klee/ASContext.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"

#include "llvm/IR/Instruction.h"

#include "Cloner.h"

#include <vector>

using namespace llvm;
using namespace klee;

ASContext::ASContext(Cloner *cloner, std::vector<Instruction *> &callTrace, Instruction *allocInst) {
    for (std::vector<Instruction *>::iterator i = callTrace.begin(); i != callTrace.end(); i++) {
        Instruction *inst = *i;
        trace.push_back(getTranslatedInst(cloner, inst));
    }

    trace.push_back(getTranslatedInst(cloner, allocInst));
}

ASContext::ASContext(ASContext &other) :
    trace(other.trace)    
{
    
}

Instruction *ASContext::getTranslatedInst(Cloner *cloner, Instruction *inst) {
    /* get function */
    Function *f = inst->getParent()->getParent();
    /* get translation map */
    Cloner::ValueTranslationMap *map = cloner->getCloneInfo(f);
    /* translate if required... */
    if (map) {
        Value *clonedValue = dyn_cast<Value>(inst);
        Cloner::ValueTranslationMap::iterator entry = map->find(clonedValue);
        if (entry == map->end()) {
            assert(false);
        }

        Value *originalValue = entry->second;
        inst = dyn_cast<Instruction>(originalValue);
    }

    return inst;
}

void ASContext::dump() {
    errs() << "allocation site context:\n";
    for (std::vector<Instruction *>::iterator i = trace.begin(); i != trace.end(); i++) {
        Instruction *inst = *i;
        Function *f = inst->getParent()->getParent();
        errs() << "  -- " << f->getName() << ":";
        inst->dump();        
    }
}

bool ASContext::operator==(ASContext &other) {
    return trace == other.trace;
}

bool ASContext::operator!=(ASContext &other) {
    return !(*this == other);
}
