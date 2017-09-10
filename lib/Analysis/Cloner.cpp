#include <stdbool.h>
#include <iostream>
#include <set>
#include <map>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/ValueHandle.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Support/raw_ostream.h>

#include "klee/Internal/Analysis/ReachabilityAnalysis.h"
#include "klee/Internal/Analysis/Cloner.h"

using namespace std;
using namespace llvm;

Cloner::Cloner(llvm::Module *module, ReachabilityAnalysis *ra, raw_ostream &debugs) :
    module(module),
    ra(ra),
    debugs(debugs)
{
}

void Cloner::clone(Function *f, uint32_t sliceId) {
    /* compute reachable functions only once */
    set<Function *> &reachable = ra->getReachableFunctions(f);
    debugs << f->getName() << ": " << reachable.size() << " reachable functions\n";

    for (set<Function *>::iterator j = reachable.begin(); j != reachable.end(); j++) {
        Function *f = *j;
        if (f->isDeclaration()) {
            continue;
        }

        debugs << "cloning: " << f->getName() << "\n";
        cloneFunction(f, sliceId);
    }
}

void Cloner::cloneFunction(Function *f, uint32_t sliceId) {
    /* TODO: check the last parameter! */
    ValueToValueMapTy *v2vmap = new ValueToValueMapTy();
    Function *cloned = CloneFunction(f, *v2vmap, true);

    /* set function name */
    string clonedName = f->getName().str() + string("_clone_") + to_string(sliceId);
    cloned->setName(StringRef(clonedName));

    /* update map */
    SliceInfo sliceInfo = {
        .f = cloned,
        .isSliced = false,
        .v2vmap = v2vmap
    };
    functionMap[f][sliceId] = sliceInfo;

    /* update map */
    cloneInfoMap[cloned] = buildReversedMap(v2vmap);
}

Cloner::ValueTranslationMap *Cloner::buildReversedMap(ValueToValueMapTy *v2vmap) {
    ValueTranslationMap *map = new ValueTranslationMap();
    for (ValueToValueMapTy::iterator i = v2vmap->begin(); i != v2vmap->end(); i++) {
        /* TODO: should be const Value... */
        Value *value = (Value *)(i->first);
        WeakVH &wvh = i->second;
        Value *mappedValue  = &*wvh;

        /* map only instructions */
        if (!dyn_cast<Instruction>(value)) {
            continue;
        }

        map->insert(make_pair(mappedValue, value));
    }

    return map;
}

Cloner::SliceMap *Cloner::getSlices(llvm::Function *function) {
    FunctionMap::iterator i = functionMap.find(function);
    if (i == functionMap.end()) {
        return 0;
    }

    SliceMap &sliceMap = i->second;
    return &sliceMap;
}

Cloner::SliceInfo *Cloner::getSliceInfo(llvm::Function *function, uint32_t sliceId) {
    SliceMap *sliceMap = getSlices(function);
    if (!sliceMap) {
        return 0;
    }

    SliceMap::iterator i = sliceMap->find(sliceId);
    if (i == sliceMap->end()) {
        return 0;
    }

    SliceInfo &sliceInfo = i->second;
    return &sliceInfo;
}

/* translate a cloned value to it's original one */
Value *Cloner::translateValue(Value *value) {
    Instruction *inst = dyn_cast<Instruction>(value);
    if (!inst) {
        /* TODO: do we clone only instructions? */
        return value;
    }

    Function *f = inst->getParent()->getParent();
    CloneInfoMap::iterator entry = cloneInfoMap.find(f);
    if (entry == cloneInfoMap.end()) {
        /* the value is not contained in a cloned function */
        return value;
    }

    ValueTranslationMap *map = entry->second;
    ValueTranslationMap::iterator i = map->find(value);
    if (i == map->end()) {
        /* TODO: add assert instead? */
        return NULL;
    }

    return i->second;
}

Cloner::~Cloner() {
    for (FunctionMap::iterator i = functionMap.begin(); i != functionMap.end(); i++) {
        SliceMap &sliceMap = i->second;
        for (SliceMap::iterator j = sliceMap.begin(); j != sliceMap.end(); j++) {
            SliceInfo &sliceInfo = j->second;
            /* TODO: refactor? */
            Function *cloned = sliceInfo.f;
            delete cloned;
            ValueToValueMapTy *v2vmap = sliceInfo.v2vmap;
            delete v2vmap;
        }
    }
}
