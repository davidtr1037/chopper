#ifndef AAPASS_H
#define AAPASS_H

#include "MemoryModel/PointerAnalysis.h"
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Pass.h>

class AAPass : public llvm::ModulePass, public llvm::AliasAnalysis {

public:
  static char ID;

  enum AliasCheckRule {
    Conservative, ///< return MayAlias if any pta says alias
    Veto,         ///< return NoAlias if any pta says no alias
    Precise       ///< return alias result by the most precise pta
  };

  AAPass()
      : llvm::ModulePass(ID), llvm::AliasAnalysis(),
        type(PointerAnalysis::Default_PTA), _pta(0) {}

  ~AAPass();

  virtual inline void getAnalysisUsage(llvm::AnalysisUsage &au) const {
    au.setPreservesAll();
  }

  virtual inline void *getAdjustedAnalysisPointer(llvm::AnalysisID id) {
    return this;
  }

  virtual inline llvm::AliasAnalysis::AliasResult
  alias(const llvm::AliasAnalysis::Location &LocA,
        const llvm::AliasAnalysis::Location &LocB) {
    return alias(LocA.Ptr, LocB.Ptr);
  }

  virtual llvm::AliasAnalysis::AliasResult alias(const llvm::Value *V1,
                                                 const llvm::Value *V2);

  virtual bool runOnModule(llvm::Module &module);

  virtual inline const char *getPassName() const { return "AAPass"; }

  void setPAType(PointerAnalysis::PTATY type) { this->type = type; }

  BVDataPTAImpl *getPTA() { return _pta; }

private:
  void runPointerAnalysis(llvm::Module &module, u32_t kind);

  PointerAnalysis::PTATY type;
  BVDataPTAImpl *_pta;
};

#endif /* AAPASS_H */
