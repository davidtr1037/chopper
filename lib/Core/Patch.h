#ifndef KLEE_PATCH_H
#define KLEE_PATCH_H

#include <vector>
#include <set>
#include <stack>
#include <string>
#include <cassert>

class Patch
{
public:
  Patch() : pathFilesCnt(0), lineCovered(false) { };
  ~Patch() { };
  bool Load(const std::string& path);
  bool Save(const std::string& path, const std::string& pathCP);
  bool inPatch(const std::string& file, int line, int assemblyLine = 0);
  void covered(const std::string& file, int line, int assemblyLine = 0);
  bool anythingCovered();

  void push(int assemblyLine) { stackAssemblyLines.push(assemblyLine); stackLineCovered.push(lineCovered); lineCovered = false; }
  void pop() { stackAssemblyLines.pop(); lineCovered = stackLineCovered.top(); stackLineCovered.pop(); }
  int top() { assert(stackAssemblyLines.size()); return stackAssemblyLines.top(); }
  int size() { return stackAssemblyLines.size(); }
private:
  size_t pathFilesCnt;
  std::vector<std::string> patchFiles;
  std::vector<std::set<int> > patchLines;
  std::vector<std::set<int> > coveredPatchLines;
  std::vector<std::set<int> > coveredCPPatchLines;
  bool lineCovered;

  std::stack<int> stackAssemblyLines;
  std::stack<bool> stackLineCovered;
  // covered lines are of no interest right now
};

#endif
