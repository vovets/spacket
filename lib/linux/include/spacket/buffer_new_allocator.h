#pragma once

#include <spacket/result.h>

#include <cstdint>


class NewAllocator {
public:
    static constexpr size_t maxSize() { return 1024 * 1024; }
    
    Result<void*> allocate(std::size_t count) {
        return ok(::operator new[](count));
    }

    void deallocate(void* ptr) {
        ::operator delete[](ptr);
    }
};
