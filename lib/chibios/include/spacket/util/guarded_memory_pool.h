#pragma once

#include <spacket/result.h>

#include "chmempools.h"

template <size_t OBJECT_SIZE, size_t NUM_OBJECTS>
class GuardedMemoryPoolT {
public:
    GuardedMemoryPoolT()
        : pool(_GUARDEDMEMORYPOOL_DATA(pool, OBJECT_SIZE)) {
        chGuardedPoolLoadArray(&pool, data, NUM_OBJECTS);
    }

    Result<uint8_t*> allocate() {
        return allocateCheck(chGuardedPoolAllocTimeout(&pool, TIME_INFINITE));
    }

    Result<uint8_t*> allocateS() {
        return allocateCheck(chGuardedPoolAllocTimeoutS(&pool, TIME_INFINITE));
    }

    void deallocate(uint8_t* p) {
        chGuardedPoolFree(&pool, p);
    }
    
private:
    Result<uint8_t*> allocateCheck(uint8_t* p) {
        if (!p) { return fail<uint8_t*>(Error::GuardedPoolOutOfMem); }
        return ok(p);
    }
    
private:
    guarded_memory_pool_t pool;
    uint8_t data[OBJECT_SIZE * NUM_OBJECTS];
};
