#include "Memory.h"
#include "klee/ASContext.h"
#include "klee/AllocationRecord.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "klee/Internal/Support/Debug.h"

#include "llvm/Support/ErrorHandling.h"

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
            if (!mo) {
                continue;
            }

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
            if (!mo) {
                continue;
            }

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
        ref<ASContext> c(new ASContext(context));
        std::list<MemoryObject *> q;
        q.push_back(mo);
        record.push_back(std::make_pair(c, q));
    } else {
        std::list<MemoryObject *> &q = entry->second;
        q.push_back(mo);
    }

    if (mo) {
        mo->refCount++;
    }
}

MemoryObject *AllocationRecord::getAddr(ASContext &context) {
    Entry *entry = find(context);
    if (entry == NULL) {
      llvm_unreachable("Could not find any entry for the context");
    }
    
    std::list<MemoryObject *> &q = entry->second;
    if (q.empty()) {
      llvm_unreachable("Could not find any MemoryObject for the context");
    }

    MemoryObject *mo = q.front();
    q.pop_front();

    if (mo) {
        /* TODO: check reference count... */
        mo->refCount--;
    }

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
    if (record.empty()) {
        DEBUG_WITH_TYPE(DEBUG_BASIC, klee_message("allocation record is empty"));
    } else {
        DEBUG_WITH_TYPE(DEBUG_BASIC, klee_message("allocation record:"));
        for (Record::iterator i = record.begin(); i != record.end(); i++) {
            Entry &entry = *i;

            /* dump context */
            ref<ASContext> c = entry.first;
            c->dump();

            /* dump addresses */
            MemoryObjectList &memoryObjects = entry.second;
            DEBUG_WITH_TYPE(DEBUG_BASIC, klee_message("memory objects:"));
            for (MemoryObjectList::iterator j = memoryObjects.begin(); j != memoryObjects.end(); j++) {
                MemoryObject *mo = *j;
                if (mo) {
                    DEBUG_WITH_TYPE(DEBUG_BASIC, klee_message("-- %lx", mo->address));
                } else {
                    DEBUG_WITH_TYPE(DEBUG_BASIC, klee_message("-- null"));
                }
            }
        }
    }
}

}
