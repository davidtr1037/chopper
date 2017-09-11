#ifndef ANNOTATOR_H
#define ANNOTATOR_H

#include <stdbool.h>
#include <iostream>
#include <map>
#include <set>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

#include "ModRefAnalysis.h"

class Annotator {
public:
  struct AnnotationInfo {
    uint32_t subId;
    std::set<std::string> fnames;

    AnnotationInfo() : subId(0) {}
  };

  typedef std::map<uint32_t, AnnotationInfo> AnnotationsMap;

  Annotator(llvm::Module *module, ModRefAnalysis *mra)
      : module(module), mra(mra), argId(0) {}

  void annotate();

  std::set<std::string> &getAnnotatedNames(uint32_t sliceId);

private:
  void annotateStores(std::set<llvm::Instruction *> &stores, uint32_t sliceId);

  void annotateStore(llvm::Instruction *inst, uint32_t sliceId);

  static std::string getAnnotatedName(uint32_t sliceId, uint32_t subId);

  llvm::Function *getCriterionFunction(llvm::Value *pointer, uint32_t sliceId);

  llvm::Module *module;
  ModRefAnalysis *mra;
  AnnotationsMap annotationsMap;
  uint32_t argId;
};

#endif
