#include "Memory.h"
#include "klee/ASContext.h"
#include "klee/AllocationRecord.h"

#include <map>
#include <list>

using namespace llvm;

namespace klee {

class MemoryObject;

AllocationRecord::AllocationRecord(const AllocationRecord &other) :
    record(other.record)
{
    incRefCount();
}

AllocationRecord &AllocationRecord::operator=(const AllocationRecord &other) {
    if (this != &other) {
        record = other.record;
        incRefCount();
    }
    return *this;
}

/* TODO: may have some problems with the destructor of ExecutionState (refCount) */
AllocationRecord::~AllocationRecord() {
    decRefCount();
}

void AllocationRecord::incRefCount() {
    for (Record::iterator i = record.begin(); i != record.end(); i++) {
        Entry &entry = *i;
        std::list<MemoryObject *> &memoryObjects = entry.second;
        for (std::list<MemoryObject *>::iterator j = memoryObjects.begin(); j != memoryObjects.end(); j++) {
            MemoryObject *mo = *j;
            mo->refCount++;
        }
    }
}

void AllocationRecord::decRefCount() {
    for (Record::iterator i = record.begin(); i != record.end(); i++) {
        Entry &entry = *i;
        std::list<MemoryObject *> &memoryObjects = entry.second;
        for (std::list<MemoryObject *>::iterator j = memoryObjects.begin(); j != memoryObjects.end(); j++) {
            MemoryObject *mo = *j;
            assert(mo->refCount > 0);
            mo->refCount--;
            if (mo->refCount == 0) {
                delete mo;
            }
        }
    }
}

void AllocationRecord::addAddr(ASContext &context, MemoryObject *mo) {
    Entry *entry = find(context);
    if (!entry) {
        ASContext *c = new ASContext(context);
        std::list<MemoryObject *> q;
        q.push_back(mo);
        record.push_back(std::make_pair(c, q));
    } else {
        std::list<MemoryObject *> &q = entry->second;
        q.push_back(mo);
    }

    mo->refCount++;
}

MemoryObject *AllocationRecord::getAddr(ASContext &context) {
    Entry *entry = find(context);
    if (entry == NULL) {
        assert(false);
    }
    
    std::list<MemoryObject *> &q = entry->second;
    if (q.empty()) {
        assert(false);
    }

    MemoryObject *mo = q.front();
    q.pop_front();

    /* TODO: check reference count... */
    mo->refCount--;

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

}
