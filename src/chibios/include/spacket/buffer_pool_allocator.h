#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>
#include <spacket/allocator.h>

template <typename Pool>
class PoolAllocatorT: public alloc::Allocator {
public:
    Result<void*> allocate() override {
        return Pool::instance().allocate();
    }

    void deallocate(void* ptr) override {
        Pool::instance().deallocate(ptr);
    }

    size_t maxSize() const override { return Pool::objectSize(); }
};
