#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>
#include <spacket/allocator.h>

template <typename Pool>
class PoolAllocatorT: public alloc::Allocator {
private:
    Result<void*> allocate(std::size_t size, std::size_t /*align*/) override {
        if (size > maxSize_()) {
            return fail<void*>(toError(ErrorCode::PoolAllocatorTooBig));
        }
        return Pool::instance().allocate();
    }

    void deallocate(void* ptr) override {
        Pool::instance().deallocate(ptr);
    }

    static std::size_t maxSize_() { return Pool::objectSize(); }
    
public:
    size_t maxSize() const override { return maxSize_(); }
};
