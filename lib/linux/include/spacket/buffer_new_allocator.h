#pragma once

#include <cstdint>


class NewAllocator {
public:
    uint8_t* allocate(std::size_t count) {
        return static_cast<uint8_t*>(::operator new[](count));
    }

    void deallocate(uint8_t* ptr) {
        ::operator delete[](ptr);
    }
};
