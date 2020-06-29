#pragma once

#include <spacket/result.h>
#include <spacket/allocator.h>

#include <cstdint>


class NewAllocator: public alloc::Allocator {
public:
    std::size_t maxSize() const override { return 65536; }
    
    Result<void*> allocate() override {
        return ok(::operator new[](maxSize()));
    }

    void deallocate(void* ptr) override {
        ::operator delete[](ptr);
    }
};
