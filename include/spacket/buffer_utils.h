#pragma once

#include <iomanip>
#include <cstring>

template<typename Buffer>
struct Hr {
    const Buffer& b;
};

template<typename Buffer>
Hr<Buffer> hr(const Buffer& b) { return Hr<Buffer>{b}; }

template<typename Buffer>
std::ostream& operator<<(std::ostream& os, Hr<Buffer> hr) {
    os  << hr.b.size() << ":[";
    for (uint8_t* cur = hr.b.begin(); cur < hr.b.end(); ++cur) {
        if (cur != hr.b.begin()) {
            os << ", ";
        }
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(*cur);
    }
    os << "]";
    return os;
}

template<typename Buffer>
Buffer operator+(const Buffer& lhs, const Buffer& rhs) {
    Buffer result(lhs.size() + rhs.size());
    std::memcpy(result.begin(), lhs.begin(), lhs.size());
    std::memcpy(result.begin() + lhs.size(), rhs.begin(), rhs.size());
    return std::move(result);
}

template<typename Buffer>
bool isPrefix(const Buffer& prefix, const Buffer& b) {
    if (prefix.size() > b.size()) {
        return false;
    }
    return 0 == std::memcmp(prefix.begin(), b.begin(), prefix.size());
}
