#pragma once

#include <spacket/result.h>
#include <spacket/debug_print.h>

#include "ch.h"
#include "chmempools.h"

namespace guarded_memory_pool_impl {

#ifdef GUARDED_MEMORY_POOL_ENABLE_DEBUG_TRACE
template <typename... U>
void debugPrintLine(const char* fmt, U&&... u) {
    ::debugPrintLine(fmt, std::forward<U>(u)...);
}
#else
template <typename... U>
void debugPrintLine(const char*, U&&...) {}
#endif

}

template <size_t OBJECT_SIZE, size_t NUM_OBJECTS>
class GuardedMemoryPoolT {
public:
    GuardedMemoryPoolT()
        : pool(_GUARDEDMEMORYPOOL_DATA(pool, OBJECT_SIZE)) {
        chGuardedPoolLoadArray(&pool, data, NUM_OBJECTS);
    }

    Result<void*> allocate() {
        return allocateCheck(chGuardedPoolAllocTimeout(&pool, TIME_IMMEDIATE));
    }

    Result<void*> allocateS() {
        return allocateCheck(chGuardedPoolAllocTimeoutS(&pool, TIME_IMMEDIATE));
    }

    void deallocate(void* p) {
        chGuardedPoolFree(&pool, p);
        guarded_memory_pool_impl::debugPrintLine("pool: deallocated %x", p);
    }

    static constexpr size_t objectSize() { return OBJECT_SIZE; }
    
private:
    Result<void*> allocateCheck(void* p) {
        if (!p) {
            return fail<void*>(toError(ErrorCode::GuardedPoolOutOfMem));
        }
        guarded_memory_pool_impl::debugPrintLine("pool: allocated %x", p);
        return ok(static_cast<void*>(p));
    }
    
private:
    guarded_memory_pool_t pool;
    uint8_t data[OBJECT_SIZE * NUM_OBJECTS];
};
