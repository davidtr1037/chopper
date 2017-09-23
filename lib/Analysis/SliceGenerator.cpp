#include <stdbool.h>
#include <iostream>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>

#include "llvm/Support/CommandLine.h"

#include "llvm/analysis/PointsTo/PointsTo.h"

#include "klee/Internal/Analysis/AAPass.h"
#include "klee/Internal/Analysis/ModRefAnalysis.h"
#include "klee/Internal/Analysis/Annotator.h"
#include "klee/Internal/Analysis/Cloner.h"
#include "klee/Internal/Analysis/SVFPointerAnalysis.h"
#include "klee/Internal/Analysis/Slicer.h"
#include "klee/Internal/Analysis/SliceGenerator.h"

using namespace std;
using namespace llvm;
using namespace dg;


void SliceGenerator::generate() {
	/* add annotations for slicing */
	annotator = new Annotator(module, mra);
	annotator->annotate();

    /* notes:
       - UNKNOWN_OFFSET: field sensitive (not sure if this flag changes anything...)
       - main: we need the nodes of the whole program
    */
    llvmpta = new LLVMPointerAnalysis(module, UNKNOWN_OFFSET, "main");
    llvmpta->PS->setRoot(llvmpta->builder->buildLLVMPointerSubgraph());

    /* translate the results of SVF to DG */
    SVFPointerAnalysis svfpa(module, llvmpta, aa);
    svfpa.run();

    if (lazyMode) {
        return;
    }

    /* generate all the slices... */
    ModRefAnalysis::SideEffects &sideEffects = mra->getSideEffects();
    for (ModRefAnalysis::SideEffects::iterator i = sideEffects.begin(); i != sideEffects.end(); i++) {
        generateSlice(i->getFunction(), i->id, i->type);
    }
}

void SliceGenerator::generateSlice(Function *f, uint32_t sliceId, ModRefAnalysis::SideEffectType type) {
    std::vector<std::string> criterions;
    std::set<std::string> fnames;

    /* set criterion functions */
    switch (type) {
    case ModRefAnalysis::ReturnValue:
        criterions.push_back("ret");
        break;

    case ModRefAnalysis::Modifier:
        fnames = annotator->getAnnotatedNames(sliceId);
        for (std::set<std::string>::iterator i = fnames.begin(); i != fnames.end(); i++) {
            std::string fname = *i;
            criterions.push_back(fname);
        }
        break;

    default:
        assert(false);
        break;
    }

    /* create the clone (inclusive) */
    cloner->clone(f, sliceId);

    /* generate slice */
    string entryName = f->getName().data();
    Slicer slicer(module, 0, entryName, criterions, llvmpta, cloner);
    slicer.setSliceId(sliceId);
    slicer.run();

    markAsSliced(f, sliceId);
}

void SliceGenerator::markAsSliced(Function *sliceEntry, uint32_t sliceId) {
    set<Function *> &reachable = ra->getReachableFunctions(sliceEntry);

    /* mark all reachable functions as sliced... */
    for (set<Function *>::iterator i = reachable.begin(); i != reachable.end(); i++) {
        Function *f = *i;
        if (f->isDeclaration()) {
            continue;
        }

        Cloner::SliceInfo *sliceInfo = cloner->getSliceInfo(*i, sliceId);
        sliceInfo->isSliced = true;
    }
}

void SliceGenerator::dumpSlice(Function *f, uint32_t sliceId, bool recursively) {
    Cloner::SliceInfo *sliceInfo = cloner->getSliceInfo(f, sliceId);
    if (!sliceInfo) {
        /* slice not found... */
        return;
    }

    set<Function *> functions;
    if (recursively) {
        set<Function *> &reachable = ra->getReachableFunctions(f);
        functions.insert(reachable.begin(), reachable.end());
    } else {
        functions.insert(f);
    }

    for (set<Function *>::iterator i = functions.begin(); i != functions.end(); i++) {
        Function *g = *i;
        if (g->isDeclaration()) {
            continue;
        }

        sliceInfo = cloner->getSliceInfo(g, sliceId);
        if (sliceInfo->isSliced) {
            sliceInfo->f->print(debugs);
        }
    }
}

SliceGenerator::~SliceGenerator() {
    delete llvmpta;
    delete annotator;
}
