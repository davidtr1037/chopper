#include "Patch.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdlib.h>

using namespace std;

bool Patch::Load(const string& path)
{
  ifstream inf(path.c_str());
  if (!inf.is_open())
    return false;

  string file, lines;
  set<int> lineset;

  while (!inf.eof()) {
    getline(inf, file);
    getline(inf, lines);
    if (file.empty())
      continue;

    cerr << "file: " << file << endl;
    stringstream strstr(lines);
    istream_iterator<std::string> it(strstr);
    istream_iterator<std::string> end;
    lineset.clear();
    for (; it != end; ++it) {
      lineset.insert(atoi(it->c_str()));
      cerr << atoi(it->c_str()) << " ";
    }
    cerr << endl;

    patchFiles.push_back(file);
    patchLines.push_back(lineset);
    coveredPatchLines.push_back(set<int>());
    coveredCPPatchLines.push_back(set<int>());
  }
  pathFilesCnt = patchFiles.size();
  return true;
}


bool Patch::Save(const string& path, const string& pathCP)
{
  ofstream outf(path.c_str());
  if (!outf.is_open())
    return false;

  for (size_t i = 0; i < pathFilesCnt; ++i) {
    if (coveredPatchLines[i].size()) {
      outf << patchFiles[i] << endl;
      for (set<int>::const_iterator it = coveredPatchLines[i].begin(), ite = coveredPatchLines[i].end();
          it != ite; ++it)
        outf << *it << " ";
      outf << endl;
    }
  }
  ofstream outfcp(pathCP.c_str());
  if (!outfcp.is_open())
    return false;

  for (size_t i = 0; i < pathFilesCnt; ++i) {
    if (coveredCPPatchLines[i].size()) {
      outfcp << patchFiles[i] << endl;
      for (set<int>::const_iterator it = coveredCPPatchLines[i].begin(), ite = coveredCPPatchLines[i].end();
          it != ite; ++it)
        outfcp << *it << " ";
      outfcp << endl;
    }
  }

  return true;
}

bool Patch::inPatch(const string& file, int line, int assemblyLine /* = 0 */)
{
  if ((!file.size() || !line) && !assemblyLine)
    return false;
  if (!stackAssemblyLines.empty()) {
    return assemblyLine == stackAssemblyLines.top();
  }
  size_t i;
  for (i = 0; i < pathFilesCnt; ++i) {
    if (file.find(patchFiles[i]) != string::npos)
      return (patchLines[i].find(line) != patchLines[i].end());
  }
  return false;
}

void Patch::covered(const string& file, int line, int assemblyLine /* = 0 */)
{
  if ((!file.size() || !line) && !assemblyLine)
    return;
  if (!stackAssemblyLines.empty()) {
    if (assemblyLine == stackAssemblyLines.top()) {
      lineCovered = true;
    }
    return;
  }
  size_t i;
  for (i = 0; i < pathFilesCnt; ++i) {
    if (file.find(patchFiles[i]) != string::npos && patchLines[i].find(line) != patchLines[i].end()) {
      coveredCPPatchLines[i].insert(line);
      lineCovered = true;
    }
  }
  return;
}

bool Patch::anythingCovered()
{
  return lineCovered;
}


