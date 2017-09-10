#include <MemoryModel/PointerAnalysis.h>
#include <WPA/Andersen.h>
#include <WPA/FlowSensitive.h>

#include "klee/Internal/Analysis/AAPass.h"

using namespace llvm;

char AAPass::ID = 0;

static RegisterPass<AAPass> WHOLEPROGRAMPA("AAPass",
        "Whole Program Pointer Analysis Pass");

AAPass::~AAPass() {
    delete _pta;
}

bool AAPass::runOnModule(llvm::Module& module) {
    runPointerAnalysis(module, type);
    return false;
}

void AAPass::runPointerAnalysis(llvm::Module& module, u32_t kind) {
    switch (kind) {
    case PointerAnalysis::Andersen_WPA:
        _pta = new Andersen();
        break;
    case PointerAnalysis::AndersenLCD_WPA:
        _pta = new AndersenLCD();
        break;
    case PointerAnalysis::AndersenWave_WPA:
        _pta = new AndersenWave();
        break;
    case PointerAnalysis::AndersenWaveDiff_WPA:
        _pta = new AndersenWaveDiff();
        break;
    case PointerAnalysis::FSSPARSE_WPA:
        _pta = new FlowSensitive();
        break;
    default:
        llvm::errs() << "This pointer analysis has not been implemented yet.\n";
        break;
    }

    _pta->analyze(module);
}

llvm::AliasAnalysis::AliasResult AAPass::alias(const Value* V1, const Value* V2) {
    llvm::AliasAnalysis::AliasResult result = MayAlias;

    PAG* pag = _pta->getPAG();
    if (pag->hasValueNode(V1) && pag->hasValueNode(V2)) {
        result = _pta->alias(V1, V2);
    }

    return result;
}
