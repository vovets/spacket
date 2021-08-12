#pragma once

#include <spacket/allocator.h>
#include <spacket/result.h>

template <std::size_t MAX_SIZE>
class MallocAllocatorT: public alloc::Allocator {
    Result<void*> allocate(std::size_t size, std::size_t /*align*/) override {
        if (size > MAX_SIZE) {
            return fail<void*>(toError(ErrorCode::MallocAllocatorTooBig));
        }
        void* retval = std::malloc(MAX_SIZE);
        if (retval != nullptr) return ok(std::move(retval));
        return fail<void*>(toError(ErrorCode::MallocAllocatorOutOfMem));
    }

    void deallocate(void* ptr) override {
        std::free(ptr);
    }

    size_t maxSize() const override { return MAX_SIZE; }
};
