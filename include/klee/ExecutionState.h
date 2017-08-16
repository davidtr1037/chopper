//===-- ExecutionState.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTIONSTATE_H
#define KLEE_EXECUTIONSTATE_H

#include <llvm/IR/Instruction.h>

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/AllocationRecord.h"
#include "klee/Internal/ADT/TreeStream.h"

// FIXME: We do not want to be exposing these? :(
#include "../../lib/Core/AddressSpace.h"
#include "klee/Internal/Module/KInstIterator.h"
#include "klee/Internal/Module/KModule.h"

#include <map>
#include <set>
#include <vector>

namespace klee {
class Array;
class CallPathNode;
struct Cell;
struct KFunction;
struct KInstruction;
class MemoryObject;
class PTreeNode;
struct InstructionInfo;

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryMap &mm);

struct StackFrame {
  KInstIterator caller;
  KFunction *kf;
  CallPathNode *callPathNode;

  std::vector<const MemoryObject *> allocas;
  Cell *locals;

  /// Minimum distance to an uncovered instruction once the function
  /// returns. This is not a good place for this but is used to
  /// quickly compute the context sensitive minimum distance to an
  /// uncovered instruction. This value is updated by the StatsTracker
  /// periodically.
  unsigned minDistToUncoveredOnReturn;

  // For vararg functions: arguments not passed via parameter are
  // stored (packed tightly) in a local (alloca) memory object. This
  // is setup to match the way the front-end generates vaarg code (it
  // does not pass vaarg through as expected). VACopy is lowered inside
  // of intrinsic lowering.
  MemoryObject *varargs;

  StackFrame(KInstIterator caller, KFunction *kf);
  StackFrame(const StackFrame &s);
  ~StackFrame();
};

#define NORMAL_STATE (1 << 0)
#define RECOVERY_STATE (1 << 1)

struct RecoveryInfo {
    unsigned int refCount;

    /* TODO: is it required? */
    llvm::Instruction *loadInst;
    uint64_t loadAddr;
    uint64_t loadSize;
    /* TODO: a bit strange that it is here, will be fixed later */
    llvm::Function *f;
    uint32_t sliceId;
    /* TODO: a bit strange that it is here, will be fixed later */
    ref<ExecutionState> snapshotState;
    unsigned int snapshotIndex;

    RecoveryInfo() :
        refCount(0),
        loadInst(0),
        loadAddr(0),
        loadSize(0),
        f(0),
        sliceId(0),
        snapshotState(0),
        snapshotIndex(0)
    {

    }

};

struct Snapshot {
    /* TODO: should be ref<ExecutionState *> */
    ref<ExecutionState> state;
    llvm::Function *f;

    /* TODO: is it required? */
    Snapshot() :
        state(0),
        f(0)
    {

    };

    Snapshot(ref<ExecutionState> state, llvm::Function *f) :
        state(state),
        f(f)
    {

    };
};

struct WrittenAddressInfo {
    std::set<size_t> sizes;
    unsigned int snapshotIndex;
};

/* recovery state result information */
struct RecoveryResult {
    /* did the recovery state wrote to the blocking load address */
    bool modified;
};

/// @brief ExecutionState representing a path under exploration
class ExecutionState {
public:
  typedef std::vector<StackFrame> stack_ty;
  /* the reference count is used only for snapshot states */
  unsigned int refCount;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState &);

  std::map<std::string, std::string> fnAliases;

  unsigned int type;

  /* normal state properties */

  typedef std::map<uint32_t, RecoveryResult> SnapshotCache;
  typedef std::map<uint32_t, SnapshotCache> RecoveryCache;
  typedef std::map<uint64_t, WrittenAddressInfo> WrittenAddresses;

  /* a normal state has a suspend status */
  bool suspendStatus;
  /* history of taken snapshots, which are uses to create recovery states */
  std::vector<Snapshot> snapshots;
  /* a normal state has a unique recovery state */
  ExecutionState *recoveryState;
  /* TODO: rename/re-implement */
  bool blockingLoadStatus;
  /* resloved load addresses */
  std::set<uint64_t> recoveredLoads;
  /* TODO: add doc... */
  RecoveryCache recoveryCache;
  /* we have to remember which allocations were executed */
  AllocationRecord allocationRecord;
  /* used for guiding multiple recovery states */
  std::set< ref<Expr> > guidingConstraints;
  /* we need to know if an address was written  */
  WrittenAddresses writtenAddresses;
  /* we use this to determine which recovery states must be run */
  std::list< ref<RecoveryInfo> > pendingRecoveryInfos;

  /* recovery state properties */

  /* a recovery state must stop when reaching this instruction */
  llvm::Instruction *exitInst;
  /* a recovery state has its own dependent state */
  ExecutionState *dependentState;
  /* a reference to the originating state */
  ExecutionState *originatingState;
  /* TODO: should be ref<RecoveryInfo> */
  ref<RecoveryInfo> recoveryInfo;
  /* we use this record while executing a recovery state  */
  AllocationRecord guidingAllocationRecord;

public:
  // Execution - Control Flow specific

  /// @brief Pointer to instruction to be executed after the current
  /// instruction
  KInstIterator pc;

  /// @brief Pointer to instruction which is currently executed
  KInstIterator prevPC;

  /// @brief Stack representing the current instruction stream
  stack_ty stack;

  /// @brief Remember from which Basic Block control flow arrived
  /// (i.e. to select the right phi values)
  unsigned incomingBBIndex;

  // Overall state of the state - Data specific

  /// @brief Address space used by this state (e.g. Global and Heap)
  AddressSpace addressSpace;

  /// @brief Constraints collected so far
  ConstraintManager constraints;

  /// Statistics and information

  /// @brief Costs for all queries issued for this state, in seconds
  mutable double queryCost;

  /// @brief Weight assigned for importance of this state.  Can be
  /// used for searchers to decide what paths to explore
  double weight;

  /// @brief Exploration depth, i.e., number of times KLEE branched for this state
  unsigned depth;

  /// @brief History of complete path: represents branches taken to
  /// reach/create this state (both concrete and symbolic)
  TreeOStream pathOS;

  /// @brief History of symbolic path: represents symbolic branches
  /// taken to reach/create this state
  TreeOStream symPathOS;

  /// @brief Counts how many instructions were executed since the last new
  /// instruction was covered.
  unsigned instsSinceCovNew;

  /// @brief Whether a new instruction was covered in this state
  bool coveredNew;

  /// @brief Disables forking for this state. Set by user code
  bool forkDisabled;

  /// @brief Set containing which lines in which files are covered by this state
  std::map<const std::string *, std::set<unsigned> > coveredLines;

  /// @brief Pointer to the process tree of the current state
  PTreeNode *ptreeNode;

  /// @brief Ordered list of symbolics: used to generate test cases.
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector<std::pair<const MemoryObject *, const Array *> > symbolics;

  /// @brief Set of used array names for this state.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  std::string getFnAlias(std::string fn);
  void addFnAlias(std::string old_fn, std::string new_fn);
  void removeFnAlias(std::string fn);

private:
  ExecutionState() : refCount(0), ptreeNode(0) {}

public:
  ExecutionState(KFunction *kf);

  // XXX total hack, just used to make a state so solver can
  // use on structure
  ExecutionState(const std::vector<ref<Expr> > &assumptions);

  ExecutionState(const ExecutionState &state);

  ~ExecutionState();

  ExecutionState *branch();

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array);
  void addConstraint(ref<Expr> e) {
    constraints.addConstraint(e);

    if (isNormalState() && !isRecoveryState()) {
      if (!getSnapshots().empty()) {
        addGuidingConstraint(e);
      }
    }
  }

  bool merge(const ExecutionState &b);
  void dumpStack(llvm::raw_ostream &out) const;

  void setType(int type) {
    this->type = type;
  }

  bool isNormalState() {
    return (type & NORMAL_STATE) != 0;
  }

  bool isRecoveryState() {
    return (type & RECOVERY_STATE) != 0;
  }

  bool isSuspended() {
    return suspendStatus;
  }

  bool isResumed() {
    return !isSuspended();
  }

  void setSuspended() {
    assert(isNormalState());
    suspendStatus = true;
  }

  void setResumed() {
    assert(isNormalState());
    suspendStatus = false;
  }

  std::vector<Snapshot> &getSnapshots() {
    assert(isNormalState());
    return snapshots;
  }

  void addSnapshot(Snapshot snapshot) {
    assert(isNormalState());
    snapshots.push_back(snapshot);
  }

  unsigned int getCurrentSnapshotIndex() {
    assert(isNormalState());
    assert(!snapshots.empty());
    return snapshots.size() - 1;
  }

  ExecutionState *getRecoveryState() {
    assert(isNormalState());
    return recoveryState;
  }

  void setRecoveryState(ExecutionState *state) {
    assert(isNormalState());
    if (state) {
      assert(state->isRecoveryState());
    }
    recoveryState = state;
  }

  /* TODO: rename/re-implement */
  bool isBlockingLoadRecovered() {
    assert(isNormalState());
    return blockingLoadStatus;
  }

  /* TODO: rename/re-implement */
  void markLoadAsNotRecovered() {
    assert(isNormalState());
    blockingLoadStatus = false;
  }

  /* TODO: rename/re-implement */
  void markLoadAsRecovered() {
    assert(isNormalState());
    blockingLoadStatus = true;
  }

  std::set<uint64_t> &getRecoveredLoads() {
    assert(isNormalState());
    return recoveredLoads;
  }

  void addRecoveredAddress(uint64_t address) {
    assert(isNormalState());
    recoveredLoads.insert(address);
  }

  bool isAddressRecovered(uint64_t address) {
    assert(isNormalState());
    return recoveredLoads.find(address) != recoveredLoads.end();
  }

  void clearRecoveredAddresses() {
    assert(isNormalState());
    recoveredLoads.clear();
  }

  RecoveryCache &getRecoveryCache() {
    assert(isNormalState());
    return recoveryCache;
  }

  void setRecoveryCache(RecoveryCache &cache) {
    assert(isNormalState());
    recoveryCache = cache;
  }

  void updateRecoveryCache(unsigned int snapshotIndex, unsigned int sliceId) {
    assert(isNormalState());
    SnapshotCache &snapshotCache = recoveryCache[snapshotIndex];
    RecoveryResult &result = snapshotCache[sliceId];
    result.modified = true;
  }

  bool getRecoveryResult(
    unsigned int snapshotIndex,
    unsigned int sliceId,
    RecoveryResult &result
  ) {
    RecoveryCache::iterator i = recoveryCache.find(snapshotIndex);
    if (i == recoveryCache.end()) {
      return false;
    }

    SnapshotCache &snapshotCache = i->second;
    SnapshotCache::iterator j = snapshotCache.find(sliceId);
    if (j == snapshotCache.end()) {
      return false;
    }

    result = j->second;
    return true;
  }

  llvm::Instruction *getExitInst() {
    assert(isRecoveryState());
    return exitInst;
  }

  void setExitInst(llvm::Instruction *exitInst) {
    assert(isRecoveryState());
    this->exitInst = exitInst;
  }

  ExecutionState *getDependentState() {
    assert(isRecoveryState());
    return dependentState;
  }

  void setDependentState(ExecutionState *state) {
    assert(isRecoveryState());
    assert(state->isNormalState());
    dependentState = state;
  }

  ExecutionState *getOriginatingState() {
    assert(isRecoveryState());
    return originatingState;
  }

  void setOriginatingState(ExecutionState *state) {
    assert(isRecoveryState());
    assert(state->isNormalState());
    originatingState = state;
  }

  ref<RecoveryInfo> getRecoveryInfo() {
    assert(isRecoveryState());
    return recoveryInfo;
  }

  void setRecoveryInfo(ref<RecoveryInfo> recoveryInfo) {
    assert(isRecoveryState());
    this->recoveryInfo = recoveryInfo;
  }

  void getCallTrace(std::vector<llvm::Instruction *> &callTrace);

  AllocationRecord &getAllocationRecord() {
    assert(isNormalState());
    return allocationRecord;
  }

  void setAllocationRecord(AllocationRecord &record) {
    assert(isNormalState());
    allocationRecord = record;
  }

  AllocationRecord &getGuidingAllocationRecord() {
    assert(isRecoveryState());
    return guidingAllocationRecord;
  }

  void setGuidingAllocationRecord(AllocationRecord &record) {
    assert(isRecoveryState());
    guidingAllocationRecord = record;
  }

  std::set <ref<Expr> > &getGuidingConstraints() {
    assert(isNormalState());
    return guidingConstraints;
  }

  void setGuidingConstraints(std::set< ref<Expr> > &constraints) {
    assert(isNormalState());
    guidingConstraints = constraints;
  }

  void addGuidingConstraint(ref<Expr> condition) {
    assert(isNormalState());
    guidingConstraints.insert(condition);
  }

  void clearGuidingConstraints() {
    assert(isNormalState());
    guidingConstraints.clear();
  }

  void addWrittenAddress(uint64_t address, size_t size, unsigned int snapshotIndex) {
    assert(isNormalState());
    WrittenAddressInfo &info = writtenAddresses[address];
    info.sizes.insert(size);
    info.snapshotIndex = snapshotIndex;
  }

  /* TODO: check size */
  bool getWrittenAddressInfo(uint64_t address, size_t size, WrittenAddressInfo &info) {
    assert(isNormalState());
    WrittenAddresses::iterator i = writtenAddresses.find(address);
    if (i == writtenAddresses.end()) {
      return false;
    }

    std::set<size_t> &writtenSizes = i->second.sizes;
    if (writtenSizes.size() != 1) {
      /* TODO: handle.... */
      llvm::llvm_unreachable_internal("Size written during recovery is greater than 1");
    }

    size_t writtenSize = *(writtenSizes.begin());
    if (writtenSize > size) {
      /* TODO: handle... */
      llvm::llvm_unreachable_internal("Size written during recovery is greater than expected size");
    }

    info = i->second;
    return true;
  }

  unsigned int getStartingIndex(uint64_t address, size_t size) {
    WrittenAddressInfo info;
    if (!getWrittenAddressInfo(address, size, info)) {
      /* there was no overwrite... */
      return 0;
    }

    return info.snapshotIndex + 1;
  }

  bool isInDependentMode() {
    assert(isNormalState());
    /* TODO: add doc... */
    return !getSnapshots().empty();
  }

  std::list< ref<RecoveryInfo> > &getPendingRecoveryInfos() {
    return pendingRecoveryInfos;
  }

  ref<RecoveryInfo> getPendingRecoveryInfo() {
    ref<RecoveryInfo> ri = pendingRecoveryInfos.front();
    pendingRecoveryInfos.pop_front();
    return ri;
  }

  bool hasPendingRecoveryInfo() {
    return !pendingRecoveryInfos.empty();
  }

};

}

#endif
