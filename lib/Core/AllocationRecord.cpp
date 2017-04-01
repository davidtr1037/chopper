#include "klee/AllocationRecord.h"
#include "klee/ASContext.h"

#include <map>
#include <queue>

using namespace llvm;
using namespace klee;

void AllocationRecord::addAddr(ASContext &context, MemoryObject *mo) {
    Entry *entry = find(context);
    if (!entry) {
        ASContext *c = new ASContext(context);
        std::queue<MemoryObject *> q;
        q.push(mo);
        record.push_back(std::make_pair(c, q));
    } else {
        std::queue<MemoryObject *> &q = entry->second;
        q.push(mo);
    }
}

MemoryObject *AllocationRecord::getAddr(ASContext &context) {
    Entry *entry = find(context);
    if (entry == NULL) {
        assert(false);
    }
    
    std::queue<MemoryObject *> &q = entry->second;
    if (q.empty()) {
        assert(false);
    }

    MemoryObject *mo = q.front();
    q.pop();

    return mo;
}

bool AllocationRecord::exists(ASContext &context) {
    return find(context) != NULL; 
}

AllocationRecord::Entry *AllocationRecord::find(ASContext &context) {
    for (Record::iterator i = record.begin(); i != record.end(); i++) {
        Entry &entry = *i;
        if (*entry.first == context) {
            return &entry;
        }
    }

    return NULL;
}

void AllocationRecord::dump() {
    errs() << "allocation record:\n";
    for (Record::iterator i = record.begin(); i != record.end(); i++) {
        Entry &entry = *i;
        ASContext *c = entry.first;
        c->dump();
        errs() << "size: " << entry.second.size() << "\n";
    }
}
