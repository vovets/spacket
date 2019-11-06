#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>

template <typename Pool>
class PoolAllocatorT {
public:
    Result<void*> allocate(std::size_t size) {
        if (size > Pool::objectSize()) {
            return fail<void*>(toError(ErrorCode::PoolAllocatorObjectTooBig));
        }
        return Pool::instance().allocate();
    }

    void deallocate(void* ptr) {
        Pool::instance().deallocate(ptr);
    }

    static constexpr size_t maxSize() { return Pool::objectSize(); }
};
