#pragma once

#include <spacket/format_utils.h>
#include <spacket/buffer.h>

#include <iomanip>
#include <cstring>

template<typename Buffer>
typename std::enable_if_t<
    std::is_same<typename Buffer::TypeId, buffer_impl::TypeId>::value,
    std::ostream&>
operator<<(std::ostream& os, Hr<Buffer> hr) {
    os << std::dec << hr.w.size() << ":[";
    for (const uint8_t* cur = hr.w.begin(); cur < hr.w.end(); ++cur) {
        if (cur != hr.w.begin()) {
            os << ", ";
        }
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(*cur);
    }
    os << "]";
    return os;
}

template<typename Vector>
typename std::enable_if_t<
    std::is_same<typename Vector::value_type::TypeId, buffer_impl::TypeId>::value,
    std::ostream&>
operator<<(std::ostream& os, Hr<Vector> h) {
    os << std::dec << h.w.size() << ":[";
    for (size_t i = 0; i < h.w.size(); ++i) {
        if (i != 0) {
            os << ", ";
        }
        os << hr(h.w[i]);
    }
    os << "]";
    return os;
}

template<typename Buffer>
typename std::enable_if_t<
    std::is_same<typename Buffer::TypeId, buffer_impl::TypeId>::value,
    Result<Buffer>>
operator+(const Buffer& lhs, const Buffer& rhs) {
    returnOnFail(result, Buffer::create(lhs.size() + rhs.size()));
    std::memcpy(result.begin(), lhs.begin(), lhs.size());
    std::memcpy(result.begin() + lhs.size(), rhs.begin(), rhs.size());
    return ok(std::move(result));
}

template<typename Buffer>
typename std::enable_if_t<
    std::is_same<typename Buffer::TypeId, buffer_impl::TypeId>::value,
    bool>
operator==(const Buffer& lhs, const Buffer& rhs) {
    return lhs.size() == rhs.size() && (0 == memcmp(lhs.begin(), rhs.begin(), lhs.size()));
}

template<typename Buffer>
typename std::enable_if_t<
    std::is_same<typename Buffer::TypeId, buffer_impl::TypeId>::value,
    bool>
operator!=(const Buffer& lhs, const Buffer& rhs) {
    return !operator==(lhs, rhs);
}

template<typename Buffer>
bool isPrefix(const Buffer& prefix, const Buffer& b) {
    if (prefix.size() > b.size()) {
        return false;
    }
    return 0 == std::memcmp(prefix.begin(), b.begin(), prefix.size());
}

struct BufferView {
    std::uint8_t* buffer;
    std::size_t size_;

    std::size_t size() const { return size_; }

    std::uint8_t* begin() const { return buffer; }
    std::uint8_t* end() const { return buffer + size_; }

    const void *id() const { return buffer; }
};
