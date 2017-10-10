#ifndef REACHABILITYANALYSIS_H
#define REACHABILITYANALYSIS_H

#include <stdio.h>
#include <vector>
#include <set>
#include <map>
#include <bits/stdc++.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>

#include "AAPass.h"

class ReachabilityAnalysis {
public:
  typedef llvm::Function* vertex_t;
  typedef std::pair<vertex_t, llvm::Instruction *> augmented_vertex_t;
  typedef double weight_t;

  struct neighbor {
    vertex_t target;
    llvm::Instruction *edge;
    weight_t weight;
    neighbor(vertex_t arg_target, llvm::Instruction *_edge, weight_t arg_weight)
        : target(arg_target), edge(_edge), weight(arg_weight) {}
  };

  typedef std::map<vertex_t, std::vector<neighbor> > adjacency_list_t;
  typedef std::pair<weight_t, vertex_t> weight_vertex_pair_t;

  typedef std::set<llvm::Function *> FunctionSet;
  typedef std::set<llvm::Instruction *> InstructionSet;
  typedef std::map<llvm::Function *, FunctionSet> ReachabilityMap;
  typedef std::map<llvm::FunctionType *, FunctionSet> FunctionTypeMap;
  typedef std::map<llvm::Instruction *, FunctionSet> CallMap;
  typedef std::map<llvm::Function *, InstructionSet> RetMap;

  ReachabilityAnalysis(llvm::Module *module, std::string entry,
                       std::vector<std::string> targets,
                       llvm::raw_ostream &debugs, llvm::raw_ostream &callgraph)
      : module(module), entry(entry), targets(targets), entryFunction(NULL),
        aa(NULL), debugs(debugs), callgraph(callgraph) {}

  ~ReachabilityAnalysis() {};

  /* must be called before making any reachability analysis */
  void prepare();

  void usePA(AAPass *aa) { this->aa = aa; }

  bool run(bool usePA);
  bool runOnTargets(bool usePA, std::vector<std::string> &targets);

  void computeReachableFunctions(llvm::Function *entry, bool usePA,
                                 FunctionSet &results);

  void computeShortestPath(
      llvm::Function *entry, llvm::Function *target,
      std::list<std::pair<llvm::Function *, llvm::Instruction *> > &result);

  FunctionSet &getReachableFunctions(llvm::Function *f);

  void getReachableInstructions(std::vector<llvm::CallInst *> &callSites,
                                InstructionSet &result);

  void getCallTargets(llvm::Instruction *inst, FunctionSet &result);

  void dumpReachableFunctions();
  void dumpCallGraph();
  void dumpFunctionToCallGraph(llvm::Function *f);

  void setTargets(std::vector<std::string>& _targets) { targets = _targets; }

  std::set<llvm::Function*> functions;

private:
  void addReachableFunctions(bool usePA, std::vector<llvm::Function *> &all);

  void removeUnusedValues();

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

  const weight_t max_weight = std::numeric_limits<double>::infinity();
  llvm::Module *module;
  std::string entry;
  std::vector<std::string> targets;
  llvm::Function *entryFunction;
  AAPass *aa;
  FunctionTypeMap functionTypeMap;
  ReachabilityMap reachabilityMap;
  CallMap callMap;
  RetMap retMap;
  llvm::raw_ostream &debugs;
  llvm::raw_ostream &callgraph;
};

#endif /* REACHABILITYANALYSIS_H */
