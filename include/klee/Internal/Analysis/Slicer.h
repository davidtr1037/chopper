#ifndef SLICER_H
#define SLICER_H

#include <stdio.h>

#include <llvm/IR/Module.h>

#include "llvm/LLVMDependenceGraph.h"
#include "llvm/Slicer.h"
#include "llvm/analysis/PointsTo/PointsTo.h"
#include "llvm/analysis/ReachingDefinitions/ReachingDefinitions.h"

#include "Cloner.h"

using namespace dg;
using namespace dg::analysis::rd;

class Slicer {
private:
  uint32_t slice_id = 0;
  bool got_slicing_criterion = true;

protected:
  llvm::Module *M;
  uint32_t opts = 0;
  std::string entryFunction;
  std::vector<std::string> criterions;
  LLVMPointerAnalysis *PTA;
  std::unique_ptr<LLVMReachingDefinitions> RD;
  LLVMDependenceGraph dg;
  LLVMSlicer slicer;

public:
  Slicer(llvm::Module *mod, uint32_t o, std::string entryFunction,
         std::vector<std::string> criterions, LLVMPointerAnalysis *llvmpta,
         Cloner *cloner);
  ~Slicer();

  int run();
  bool buildDG();
  bool mark();
  void computeEdges();
  bool slice();
  void remove_unused_from_module_rec();
  bool remove_unused_from_module();
  void make_declarations_external();
  const LLVMDependenceGraph &getDG() const { return dg; }
  LLVMDependenceGraph &getDG() { return dg; }
  void setSliceId(uint32_t id) { slice_id = id; }
};

#endif /* SLICER_H */
