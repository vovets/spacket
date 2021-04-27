#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>
#include <spacket/allocator.h>

template <typename Pool>
class PoolAllocatorT: public alloc::Allocator {
private:
    Result<void*> allocate() override {
        return Pool::instance().allocate();
    }

    void deallocate(void* ptr) override {
        Pool::instance().deallocate(ptr);
    }
    
public:
    size_t maxSize() const override { return Pool::objectSize(); }
};
