#pragma once

#include <spacket/result.h>

#include <cstdint>


class NewAllocator {
public:
    Result<uint8_t*> allocate(std::size_t count) {
        return ok(static_cast<uint8_t*>(::operator new[](count)));
    }

    void deallocate(uint8_t* ptr) {
        ::operator delete[](ptr);
    }
};
