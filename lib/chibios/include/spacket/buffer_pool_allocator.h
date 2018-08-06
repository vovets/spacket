#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>

template <typename Pool, Pool& pool>
class PoolAllocatorT {
public:
    Result<void*> allocate(std::size_t size) {
        if (size > Pool::objectSize()) {
            return fail<void*>(toError(ErrorCode::PoolAllocatorObjectTooBig));
        }
        return pool.allocate();
    }

    void deallocate(void* ptr) {
        pool.deallocate(ptr);
    }

    static constexpr size_t maxSize() { return Pool::objectSize(); }
};
