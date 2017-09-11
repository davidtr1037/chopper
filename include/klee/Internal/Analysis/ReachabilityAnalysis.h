#ifndef REACHABILITYANALYSIS_H
#define REACHABILITYANALYSIS_H

#include <stdio.h>
#include <vector>
#include <set>
#include <map>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include "AAPass.h"

class ReachabilityAnalysis {
public:
  typedef std::set<llvm::Function *> FunctionSet;
  typedef std::set<llvm::Instruction *> InstructionSet;
  typedef std::map<llvm::Function *, FunctionSet> ReachabilityMap;
  typedef std::map<llvm::FunctionType *, FunctionSet> FunctionTypeMap;
  typedef std::map<llvm::Instruction *, FunctionSet> CallMap;
  typedef std::map<llvm::Function *, InstructionSet> RetMap;

  ReachabilityAnalysis(llvm::Module *module, std::string entry,
                       std::vector<std::string> targets,
                       llvm::raw_ostream &debugs)
      : module(module), entry(entry), targets(targets), entryFunction(NULL), aa(NULL),
        debugs(debugs) {}

  ~ReachabilityAnalysis() {};

  /* must be called before making any reachability analysis */
  void prepare();

  void usePA(AAPass *aa) { this->aa = aa; }

  bool run(bool usePA);

  void computeReachableFunctions(llvm::Function *entry, bool usePA,
                                 FunctionSet &results);

  FunctionSet &getReachableFunctions(llvm::Function *f);

  void getReachableInstructions(std::vector<llvm::CallInst *> &callSites,
                                InstructionSet &result);

  void getCallTargets(llvm::Instruction *inst, FunctionSet &result);

  void dumpReachableFunctions();

private:
  void removeUnusedValues();

  void removeUnusedValues(bool &changed);

  void computeFunctionTypeMap();

  void updateReachabilityMap(llvm::Function *f, bool usePA);

  bool isVirtual(llvm::Function *f);

  void resolveCallTargets(llvm::CallInst *callInst, bool usePA,
                          FunctionSet &targets);

  void resolveIndirectCallByType(llvm::Type *calledType, FunctionSet &targets);

  void resolveIndirectCallByPA(llvm::Value *calledValue, FunctionSet &targets);

  void updateCallMap(llvm::Instruction *callInst, FunctionSet &targets);

  void updateRetMap(llvm::Instruction *callInst, FunctionSet &targets);

  llvm::Function *extractFunction(llvm::ConstantExpr *ce);

  llvm::Module *module;
  std::string entry;
  std::vector<std::string> targets;
  llvm::Function *entryFunction;
  std::vector<llvm::Function *> targetFunctions;
  AAPass *aa;
  FunctionTypeMap functionTypeMap;
  ReachabilityMap reachabilityMap;
  CallMap callMap;
  RetMap retMap;
  llvm::raw_ostream &debugs;
};

#endif /* REACHABILITYANALYSIS_H */
