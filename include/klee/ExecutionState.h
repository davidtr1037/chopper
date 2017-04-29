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

typedef enum {
    NORMAL_STATE,
    RECOVERY_STATE,
} ExecutionStateType;

struct RecoveryInfo {
    llvm::Instruction *loadInst;
    uint64_t loadAddr;
    uint64_t loadSize;
    uint32_t sliceId;

    RecoveryInfo() :
        loadInst(0),
        loadAddr(0),
        loadSize(0),
        sliceId(0)
    {

    }

};

/// @brief ExecutionState representing a path under exploration
class ExecutionState {
public:
  typedef std::vector<StackFrame> stack_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState &);

  std::map<std::string, std::string> fnAliases;

  ExecutionStateType type;

  /* normal state properties */

  /* a normal state has a suspend status */
  bool suspendStatus;
  /* all recovery states must be derived from this state */
  /* TODO: should be ref<ExecutionState *> */
  ExecutionState *snapshot;
  /* a normal state has a unique recovery state */
  ExecutionState *recoveryState;
  /* we should know of the current load inst */
  bool blockingLoadStatus;
  /* resloved load addresses */
  std::set<uint64_t> resolvedLoads;
  /* we have to remember which allocations were executed */
  AllocationRecord allocationRecord;
  /* used for guiding multiple recovery states */
  std::vector<ref<Expr>> accumulatingConstraints;
  /* we need to know if an address was written  */
  typedef std::map<uint64_t, std::set<size_t> > WrittenAddresses;
  WrittenAddresses writtenAddresses;
  /* TODO: will be removed later... */
  unsigned int skippedCount;
  /* TODO: will be removed later... */
  unsigned int recoveryCount;

  /* recovery state properties */

  /* a recovery state must stop when reaching this instruction */
  llvm::Instruction *exitInst;
  /* a recovery state has its own depended state */
  ExecutionState *dependedState;
  /* TODO: should be ref<RecoveryInfo> */
  RecoveryInfo *recoveryInfo;
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
  ExecutionState() : ptreeNode(0) {}

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
  void addConstraint(ref<Expr> e) { constraints.addConstraint(e); }

  bool merge(const ExecutionState &b);
  void dumpStack(llvm::raw_ostream &out) const;

  ExecutionStateType getType() {
    return type;
  }

  void setType(ExecutionStateType type) {
    this->type = type;
  }

  bool isNormalState() {
    return type == NORMAL_STATE;
  }

  bool isRecoveryState() {
    return type == RECOVERY_STATE;
  }

  bool isSuspended() {
    assert(isNormalState());
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

  ExecutionState *getSnapshot() {
    assert(isNormalState());
    return snapshot;
  }

  void setSnapshot(ExecutionState *state) {
    snapshot = state;
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

  bool isBlockingLoadResolved() {
    assert(isNormalState());
    return blockingLoadStatus;
  }

  void markLoadAsUnresolved() {
    assert(isNormalState());
    blockingLoadStatus = false;
  }

  void markLoadAsResolved() {
    assert(isNormalState());
    blockingLoadStatus = true;
  }

  std::set<uint64_t> &getResolvedLoads() {
    assert(isNormalState());
    return resolvedLoads;
  }

  void addResolvedAddress(uint64_t address) {
    assert(isNormalState());
    resolvedLoads.insert(address);
  }

  llvm::Instruction *getExitInst() {
    assert(isRecoveryState());
    return exitInst;
  }

  void setExitInst(llvm::Instruction *exitInst) {
    assert(isRecoveryState());
    this->exitInst = exitInst;
  }

  ExecutionState *getDependedState() {
    assert(isRecoveryState());
    return dependedState;
  }

  void setDependedState(ExecutionState *state) {
    assert(isRecoveryState());
    assert(state->isNormalState());
    dependedState = state;
  }

  RecoveryInfo *getRecoveryInfo() {
    assert(isRecoveryState());
    return recoveryInfo;
  }

  void setRecoveryInfo(RecoveryInfo *recoveryInfo) {
    assert(isRecoveryState());
    this->recoveryInfo = recoveryInfo;
  }

  void getCallTrace(std::vector<llvm::Instruction *> &callTrace);

  AllocationRecord &getAllocationRecord() {
    assert(isNormalState());
    return allocationRecord;
  }

  AllocationRecord &getGuidingAllocationRecord() {
    assert(isRecoveryState());
    return guidingAllocationRecord;
  }

  void setGuidingAllocationRecord(AllocationRecord &record) {
    assert(isRecoveryState());
    guidingAllocationRecord = record;
  }

  std::vector<ref<Expr>> &getAccumulatingConstraints() {
    return accumulatingConstraints;
  }

  void addAccumulatingConstraint(ref<Expr> condition) {
    accumulatingConstraints.push_back(condition);
  }

  void addWrittenAddress(uint64_t address, size_t size) {
    assert(isNormalState());
    writtenAddresses[address].insert(size);
  }

  bool isAddressWritten(uint64_t address, size_t size) {
    assert(isNormalState());
    WrittenAddresses::iterator i = writtenAddresses.find(address);
    if (i == writtenAddresses.end()) {
      return false;
    }

    std::set<size_t> &writtenSizes = i->second;
    if (writtenSizes.size() != 1) {
      /* TODO: something is wrong.... */
      assert(false);
    }

    size_t writtenSize = *(writtenSizes.begin());
    if (writtenSize != size) {
      /* TODO: handle... */
      assert(false);
    }

    /* cleanup... */
    writtenAddresses.erase(i);

    return true;
  }

  unsigned int getSkippedCount() {
    assert(isNormalState());
    return skippedCount;
  }

  void incSkippedCount() {
    assert(isNormalState());
    skippedCount++;
  }

  unsigned int getRecoveryCount() {
    assert(isNormalState());
    return recoveryCount;
  }

  void incRecoveryCount() {
    assert(isNormalState());
    recoveryCount++;
  }

  bool isExecutingRetSlice() {
    assert(isNormalState());
    StackFrame &sf = stack.back();
    return sf.kf->isCloned;
  }

  bool hasSkippedCalls() {
    assert(isNormalState());
    return getSnapshot() != 0 && !isExecutingRetSlice();
  }

};
}

#endif
