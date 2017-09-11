#ifndef SVFPOINTERANALYSIS_H
#define SVFPOINTERANALYSIS_H

#include <llvm/IR/Module.h>
#include <llvm/IR/DataLayout.h>

#include "llvm/analysis/PointsTo/PointerSubgraph.h"
#include "llvm/analysis/PointsTo/PointsTo.h"

#include "AAPass.h"

using namespace dg;

class SVFPointerAnalysis {
public:
  SVFPointerAnalysis(llvm::Module *module, LLVMPointerAnalysis *pta, AAPass *aa)
      : module(module), pta(pta), aa(aa) {}

  ~SVFPointerAnalysis() {}

  void run();
  void handleVirtualCalls();
  void handleNode(PSNode *node);
  void handleLoad(PSNode *node);
  void handleStore(PSNode *node);
  void handleGep(PSNode *node);
  void handleCast(PSNode *node);
  void handleFuncPtr(PSNode *node);
  bool functionPointerCall(PSNode *callsite, PSNode *called);
  void handlePhi(PSNode *node);
  void handleOperand(PSNode *operand);
  void updatePointsTo(PSNode *operand, PAGNode *pagnode);
  PSNode *getAllocNode(ObjPN *node);
  uint64_t getAllocNodeOffset(GepObjPN *node);

  llvm::Module *module;
  LLVMPointerAnalysis *pta;
  AAPass *aa;
};

#endif
