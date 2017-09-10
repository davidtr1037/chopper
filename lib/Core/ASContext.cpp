#include "klee/ASContext.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Internal/Support/Debug.h"

#include "llvm/IR/Instruction.h"

#include "klee/Internal/Analysis/Cloner.h"

#include <vector>

using namespace llvm;
using namespace klee;

ASContext::ASContext(Cloner *cloner, std::vector<Instruction *> &callTrace, Instruction *allocInst) {
    refCount = 0;

    for (std::vector<Instruction *>::iterator i = callTrace.begin(); i != callTrace.end(); i++) {
        Instruction *inst = *i;
        trace.push_back(getTranslatedInst(cloner, inst));
    }

    trace.push_back(getTranslatedInst(cloner, allocInst));
}

ASContext::ASContext(ASContext &other) :
    refCount(0),
    trace(other.trace)    
{

}

/* TODO: use the translatedValue API? */
Instruction *ASContext::getTranslatedInst(Cloner *cloner, Instruction *inst) {
    Value *value = cloner->translateValue(inst);
    if (!isa<Instruction>(value)) {
        /* why... */
        llvm_unreachable("Translated value is not an instruction");
    }

    return dyn_cast<Instruction>(value);
}

void ASContext::dump() {
    DEBUG_WITH_TYPE(DEBUG_BASIC, klee_message("allocation site context:"));
    for (std::vector<Instruction *>::iterator i = trace.begin(); i != trace.end(); i++) {
        Instruction *inst = *i;
        Function *f = inst->getParent()->getParent();
        DEBUG_WITH_TYPE(DEBUG_BASIC, errs() << "  -- " << f->getName() << ":");
        DEBUG_WITH_TYPE(DEBUG_BASIC, inst->dump());
    }
}

bool ASContext::operator==(ASContext &other) {
    return trace == other.trace;
}

bool ASContext::operator!=(ASContext &other) {
    return !(*this == other);
}
