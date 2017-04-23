#ifndef KLEE_ALLOCATION_RECORD_H
#define KLEE_ALLOCATION_RECORD_H

#include "klee/ASContext.h"

#include <map>
#include <queue>

namespace klee {

class MemoryObject;

class AllocationRecord {
public:

    AllocationRecord() {

    }

    AllocationRecord(const AllocationRecord &other);

    ~AllocationRecord();

    void addAddr(ASContext &context, MemoryObject *mo);
    
    MemoryObject *getAddr(ASContext &context);

    bool exists(ASContext &context);

    void dump();

private:
    typedef std::pair<ASContext *, std::list<MemoryObject *> > Entry;
    typedef std::vector<Entry> Record;

    Entry *find(ASContext &context);

    Record record;
};

}

#endif
