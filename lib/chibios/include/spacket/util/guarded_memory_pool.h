#pragma once

#include <spacket/result.h>

#include "ch.h"
#include "chmempools.h"

template <size_t OBJECT_SIZE, size_t NUM_OBJECTS>
class GuardedMemoryPoolT {
public:
    GuardedMemoryPoolT()
        : pool(_GUARDEDMEMORYPOOL_DATA(pool, OBJECT_SIZE)) {
        chGuardedPoolLoadArray(&pool, data, NUM_OBJECTS);
    }

    Result<void*> allocate() {
        return allocateCheck(chGuardedPoolAllocTimeout(&pool, TIME_INFINITE));
    }

    Result<void*> allocateS() {
        return allocateCheck(chGuardedPoolAllocTimeoutS(&pool, TIME_INFINITE));
    }

    void deallocate(void* p) {
        chGuardedPoolFree(&pool, p);
    }

    static constexpr size_t objectSize() { return OBJECT_SIZE; }
    
private:
    Result<void*> allocateCheck(void* p) {
        if (!p) { return fail<void*>(Error::GuardedPoolOutOfMem); }
        return ok(static_cast<void*>(p));
    }
    
private:
    guarded_memory_pool_t pool;
    uint8_t data[OBJECT_SIZE * NUM_OBJECTS];
};
