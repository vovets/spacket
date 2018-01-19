#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>

template <typename Pool, Pool& pool>
class PoolAllocatorT {
public:
    Result<uint8_t*> allocate(std::size_t size) {
        if (size > Pool::objectSize()) {
            return fail<uint8_t*>(Error::PoolAllocatorObjectTooBig);
        }
        return pool.allocate();
    }

    void deallocate(uint8_t* ptr) {
        pool.deallocate(ptr);
    }
};
