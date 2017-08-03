#pragma once

#include <iomanip>

template<typename Buffer>
struct Hr {
    const Buffer& b;
};

template<typename Buffer>
Hr<Buffer> hr(const Buffer& b) { return Hr<Buffer>{b}; }

template<typename Buffer>
std::ostream& operator<<(std::ostream& os, Hr<Buffer> hr) {
    os  << "[";
    for (uint8_t* cur = hr.b.begin(); cur < hr.b.end(); ++cur) {
        if (cur != hr.b.begin()) {
            os << ", ";
        }
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(*cur);
    }
    os << "]";
    return os;
}
        
