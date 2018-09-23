#ifndef MODREFANALYSIS_H
#define MODREFANALYSIS_H

#include <stdbool.h>
#include <iostream>
#include <set>
#include <map>
#include <vector>

#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Support/raw_ostream.h>

#include "ReachabilityAnalysis.h"
#include "AAPass.h"

class ModRefAnalysis {
public:
  typedef std::set<llvm::Instruction *> InstructionSet;

  typedef std::map<llvm::Function *, PointsTo> ModPtsMap;
  typedef std::map<llvm::Function *, InstructionSet> ModSetMap;

  typedef std::map<llvm::Function *, PointsTo> RefPtsMap;

  typedef std::map<std::pair<llvm::Function *, NodeID>, InstructionSet>
  ObjToStoreMap;
  typedef std::map<std::pair<llvm::Function *, NodeID>, InstructionSet>
  ObjToLoadMap;
  typedef std::map<NodeID, InstructionSet> ObjToOverridingStoreMap;
  typedef std::map<llvm::Instruction *, InstructionSet> LoadToStoreMap;

  typedef std::pair<const llvm::Value *, uint64_t> AllocSite;
  typedef std::pair<llvm::Function *, AllocSite> ModInfo;

  typedef std::map<llvm::Instruction *, std::set<ModInfo> > LoadToModInfoMap;
  typedef std::map<ModInfo, InstructionSet> ModInfoToStoreMap;
  typedef std::map<ModInfo, uint32_t> ModInfoToIdMap;
  typedef std::map<uint32_t, ModInfo> IdToModInfoMap;
  typedef std::map<llvm::Function *, uint32_t> RetSliceIdMap;

  typedef enum {
    Modifier,
    ReturnValue,
  } SideEffectType;

  typedef struct {
    SideEffectType type;
    uint32_t id;
    union {
      ModInfo modInfo;
      llvm::Function *f;
    } info;

    llvm::Function *getFunction() {
      if (type == Modifier) {
        return info.modInfo.first;
      }
      if (type == ReturnValue) {
        return info.f;
      }
      assert(false);
    }

  } SideEffect;

  typedef std::vector<SideEffect> SideEffects;

  ModRefAnalysis(llvm::Module *module, ReachabilityAnalysis *ra, AAPass *aa,
                 std::string entry, std::vector<std::string> targets,
                 llvm::raw_ostream &debugs);

  llvm::Function *getEntry();

  std::vector<llvm::Function *> getTargets();

  void run();

  ModInfoToStoreMap &getModInfoToStoreMap();

  SideEffects &getSideEffects();

  bool hasSideEffects(llvm::Function *f);

  bool getSideEffects(llvm::Function *f, InstructionSet &modSet);

  InstructionSet &getOverridingStores();

  ModInfoToIdMap &getModInfoToIdMap();

  bool mayBlock(llvm::Instruction *load);

  bool mayOverride(llvm::Instruction *store);

  bool getRetSliceId(llvm::Function *f, uint32_t &id);

  void getApproximateModInfos(llvm::Instruction *inst, AllocSite hint,
                              std::set<ModInfo> &result);

  void dumpModSetMap();

  void dumpDependentLoads();

  void dumpLoadToModInfoMap();

  void dumpModInfoToStoreMap();

  void dumpModInfoToIdMap();

  void dumpOverridingStores();

  void dumpInst(llvm::Instruction *load, const char *prefix = "");

  void dumpModInfo(const ModInfo &modInfo, const char *prefix = "");

private:
  typedef std::map<llvm::Function *, bool> ReachabilityCache;

  /* priate methods */

  void computeMod(llvm::Function *entry, llvm::Function *f);

  void collectModInfo(llvm::Function *f);

  void addStore(llvm::Function *f, llvm::Instruction *store);

  bool canIgnoreStackObject(llvm::Function *f, const llvm::Value *value);

  void collectRefInfo(llvm::Function *entry);

  void addLoad(llvm::Function *f, llvm::Instruction *load);

  void addOverridingStore(llvm::Instruction *store);

  void computeModRefInfo();

  void computeModInfoToStoreMap();

  AllocSite getAllocSite(NodeID);

  bool hasReturnValue(llvm::Function *f);

  llvm::AliasAnalysis::Location getLoadLocation(llvm::LoadInst *inst);

  llvm::AliasAnalysis::Location getStoreLocation(llvm::StoreInst *inst);

  /* private members */

  llvm::Module *module;
  ReachabilityAnalysis *ra;
  AAPass *aa;

  std::string entry;
  std::vector<std::string> targets;
  llvm::Function *entryFunction;
  std::vector<llvm::Function *> targetFunctions;

  ModPtsMap modPtsMap;
  ObjToStoreMap objToStoreMap;
  RefPtsMap refPtsMap;
  ObjToLoadMap objToLoadMap;
  ObjToOverridingStoreMap objToOverridingStoreMap;

  ModSetMap modSetMap;

  InstructionSet dependentLoads;

  LoadToModInfoMap loadToModInfoMap;
  ModInfoToStoreMap modInfoToStoreMap;

  ModInfoToIdMap modInfoToIdMap;
  RetSliceIdMap retSliceIdMap;

  SideEffects sideEffects;

  InstructionSet overridingStores;

  ReachabilityCache cache;

  llvm::raw_ostream &debugs;
};

#endif
