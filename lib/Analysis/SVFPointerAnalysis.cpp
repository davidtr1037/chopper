#include <stdio.h>

#include <iostream>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>

#include "analysis/PointsTo/PointerSubgraph.h"
#include "analysis/Offset.h"

#include "klee/Internal/Analysis/AAPass.h"
#include "klee/Internal/Analysis/SVFPointerAnalysis.h"

using namespace llvm;
using namespace dg::analysis::pta;

void SVFPointerAnalysis::run() {
    /* update virtual call related nodes */
    handleVirtualCalls();

    for (auto &v : pta->getNodesMap()) {
        PSNode *node = v.second.second;
        handleNode(node);
    }
}

void SVFPointerAnalysis::handleNode(PSNode *node) {
    switch (node->getType()) {
    case LOAD:
        handleLoad(node);
        break;

    case STORE:
        handleStore(node);
        break;

    case GEP:
        handleGep(node);
        break;

    case CAST:
        handleCast(node);
        break;

    case CONSTANT:
        break;

    case CALL_RETURN:
    case RETURN:
    case PHI:
        handlePhi(node);
        break;

    case CALL_FUNCPTR:
        break;

    case MEMCPY:
        /* TODO: handle? */
        break;

    case ALLOC:
    case DYN_ALLOC:
    case FUNCTION:
        assert(node->doesPointsTo(node, 0));
        assert(node->pointsTo.size() == 1);
    case CALL:
    case ENTRY:
    case NOOP:
        break;

    default:
        assert(false);
    }
}

void SVFPointerAnalysis::handleVirtualCalls() {
    std::set<PSNode *> visited;
    bool changed = true;

    while (changed) {
        std::set<PSNode *> discovered;
        changed = false;

        /* first, get all relevant nodes */
        for (auto &v : pta->getNodesMap()) {
            PSNode *node = v.second.second;
            if (node->getType() != CALL_FUNCPTR) {
                continue;
            }

            if (visited.find(node) == visited.end()) {
                discovered.insert(node);
                changed = true;
            }
        }

        for (PSNode *node : discovered) {
            handleFuncPtr(node);
            visited.insert(node);
        }
    }
}

void SVFPointerAnalysis::handleLoad(PSNode *node) {
    handleOperand(node);
    handleOperand(node->getOperand(0));
}

void SVFPointerAnalysis::handleStore(PSNode *node) {
    handleOperand(node);
    handleOperand(node->getOperand(1));
}

void SVFPointerAnalysis::handleGep(PSNode *node) {
    handleOperand(node);
    handleOperand(node->getOperand(0));
}

void SVFPointerAnalysis::handleCast(PSNode *node) {
    handleOperand(node);
    handleOperand(node->getOperand(0));
}

void SVFPointerAnalysis::handlePhi(PSNode *node) {
    /* TODO: check if required! */
    handleOperand(node);
    for (PSNode *op : node->getOperands()) {
        handleOperand(op);
    }
}

void SVFPointerAnalysis::handleFuncPtr(PSNode *node) {
    PSNode *operand = node->getOperand(0);
    handleOperand(operand);

    /* now, operand->pointsTo is updated */
    for (const Pointer& ptr : operand->pointsTo) {
        if (ptr.isValid()) {
            functionPointerCall(node, ptr.target);
        }
    }
}

/* based on the code from DG */
bool SVFPointerAnalysis::functionPointerCall(PSNode *callsite, PSNode *called) {
    if (!isa<Function>(called->getUserData<Value>())) {
        return false;
    }

    const Function *f = called->getUserData<Function>();
    const CallInst *callInst = callsite->getUserData<CallInst>();

    /* TODO: make a partial compatability check */
    //if (!llvmutils::callIsCompatible(F, CI))
    //    return false;

    if (f->size() == 0) {
        return callsite->getPairedNode()->addPointsTo(analysis::pta::PointerUnknown);
    }

    PSNodesSeq seq = pta->builder->createFuncptrCall(callInst, f);
    assert(seq.first && seq.second);

    PSNode *paired = callsite->getPairedNode();
    paired->addOperand(seq.second);

    if (callsite->successorsNum() == 1 && callsite->getSingleSuccessor() == paired) {
        callsite->replaceSingleSuccessor(seq.first);
    } else {
        callsite->addSuccessor(seq.first);
    }

    seq.second->addSuccessor(paired);

    return true;
}

void SVFPointerAnalysis::handleOperand(PSNode *operand) {
    Value *value = operand->getUserData<Value>();
    if (!value) {
        return;
    }

    if (!aa->getPTA()->getPAG()->hasValueNode(value)) {
        /* TODO: not a pointer? */
        return;
    }

    NodeID id = aa->getPTA()->getPAG()->getValueNode(value);
    PointsTo &pts = aa->getPTA()->getPts(id);

    if (pts.empty()) {
        operand->addPointsTo(NULLPTR);
        return;
    }

    for (PointsTo::iterator i = pts.begin(); i != pts.end(); ++i) {
        NodeID node_id = *i;
        PAGNode *pagnode = aa->getPTA()->getPAG()->getPAGNode(node_id);
        if (isa<ObjPN>(pagnode)) {
            updatePointsTo(operand, pagnode);
        }
    }
}

void SVFPointerAnalysis::updatePointsTo(PSNode *operand, PAGNode *pagnode) {
    int kind = pagnode->getNodeKind();
    ObjPN *obj_node = NULL;
    GepObjPN *gepobj_node = NULL;
    PSNode *alloc_node = NULL;
    uint64_t offset = 0;

    switch (kind) {
    case PAGNode::ObjNode:
    case PAGNode::FIObjNode:
        /* TODO: handle FIObjNode */
        obj_node = dyn_cast<ObjPN>(pagnode);
        alloc_node = getAllocNode(obj_node);
        offset = 0;
        break;

    case PAGNode::GepObjNode:
        gepobj_node = dyn_cast<GepObjPN>(pagnode);
        alloc_node = getAllocNode(gepobj_node);
        offset = getAllocNodeOffset(gepobj_node);
        break;

    case PAGNode::DummyObjNode:
        /* TODO: are we supposed to do something? */
        return;

    default:
        assert(false);
        return;
    }

    if (!alloc_node) {
        return;
    }

    /* add to PointsTo set */
    operand->addPointsTo(Pointer(alloc_node, offset));
}

PSNode *SVFPointerAnalysis::getAllocNode(ObjPN *node) {
    /* get SVF memory object (allocation site) */
    const MemObj *mo = node->getMemObj();    

    /* get corresponding DG node */
    PSNode *ref_node = pta->builder->getNode(mo->getRefVal());
    if (!ref_node) {
        /* TODO: check why DG does not have this allocation site */
        //assert(false);
    }

    return ref_node;
}

uint64_t SVFPointerAnalysis::getAllocNodeOffset(GepObjPN *node) {
    LocationSet ls = node->getLocationSet();
    assert(ls.isConstantOffset());

    /* offset in bytes */
    unsigned offsetInBytes = ls.getAccOffset();

    const MemObj *mo = node->getMemObj();
    if (mo->isArray()) {
        /* arrays are handled insensitively */
        return UNKNOWN_OFFSET;
    }

    return offsetInBytes;
}
