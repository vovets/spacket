#pragma once

#include <spacket/result.h>
#include <spacket/allocator.h>

#include <cstdint>


class NewAllocator: public alloc::Allocator {
public:
    std::size_t maxSize() const override { return 65536; }
    
    Result<void*> allocate(std::size_t size, std::size_t /*align*/) override {
        if (size > maxSize()) {
            return fail<void*>(toError(ErrorCode::NewAllocatorTooBig));
        }
        return ok(::operator new[](maxSize()));
    }

    void deallocate(void* ptr) override {
        ::operator delete[](ptr);
    }
};
