//===-- TargetFinderPass.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "llvm/DebugInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

char klee::TargetFinderPass::ID = 0;

bool klee::TargetFinderPass::runOnBasicBlock(BasicBlock &BB) {
  for (BasicBlock::iterator I = BB.begin(), E = BB.end(); I != E; ++I) {
	  Instruction& inst = *I;
	  if (MDNode *N = inst.getMetadata("dbg")) {
		DILocation Loc(N);
		const std::map<std::string, std::vector<unsigned> >::const_iterator match = targetLocation.find(Loc.getFilename().substr(Loc.getFilename().find_last_of("/\\") + 1));
		if (match != targetLocation.end()) {
		  std::vector<unsigned> lines = (*match).second;
		  if (find(lines.begin(), lines.end(), Loc.getLineNumber()) != lines.end()) {
			  targetInstructions[inst.getParent()->getParent()].push_back(&inst);
		  }
		}
	  }
  }
  return false;
}
