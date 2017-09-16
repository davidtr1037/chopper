#ifndef KLEE_ALLOCATION_RECORD_H
#define KLEE_ALLOCATION_RECORD_H

#include "klee/ASContext.h"

#include <list>

namespace klee {

class MemoryObject;

class AllocationRecord {
public:

    AllocationRecord() {

    }

    AllocationRecord(const AllocationRecord &other);

    AllocationRecord &operator=(const AllocationRecord &other);

    ~AllocationRecord();

    void addAddr(ASContext &context, MemoryObject *mo);
    
    MemoryObject *getAddr(ASContext &context);

    bool exists(ASContext &context);

    void dump();

private:
    /*  TODO: change ref<ASContext> to ASContext? */
    typedef std::list<MemoryObject *> MemoryObjectList;
    typedef std::pair<ref<ASContext>, MemoryObjectList> Entry;
    typedef std::vector<Entry> Record;

    void incRefCount();

    void decRefCount();

    Entry *find(ASContext &context);

    Record record;
};

}

#endif
