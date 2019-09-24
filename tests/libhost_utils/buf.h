#pragma once

#include <boost/optional.hpp>

inline
Buffer buf(size_t size) { return std::move(throwOnFail(Buffer::create(size))); }

inline
Buffer buf(size_t size, uint8_t fill) {
    Buffer b = throwOnFail(Buffer::create(size));
    for (auto& c: b) {
        c = fill;
    }
    return b;
}

inline
Buffer buf(std::initializer_list<uint8_t> l) {
    return throwOnFail(Buffer::create(l));
}

inline
Buffer buf(std::string s) {
    return throwOnFail(Buffer::create(std::vector<std::uint8_t>(s.begin(), s.end())));
}

inline
Buffer cob(Buffer b) {
    return throwOnFail(cobs::stuffAndDelim(std::move(b)));
}

inline
Buffer wz(std::size_t size) { return createTestBuffer<Buffer>(size); }

inline
Buffer nz(std::size_t size) { return createTestBufferNoZero<Buffer>(size); }

inline
Buffer cat_(const Buffer& l, const Buffer& r) {
    return throwOnFail(l + r);
}

template <typename Arg1, typename Arg2, typename ...Args>
Buffer cat_(const Arg1& arg1, const Arg2& arg2, const Args& ...args) {
    return cat_(cat_(arg1, arg2), args...);
}

template <typename ...Args>
Buffer cat(const Args& ...args) {
    return cat_(args...);
}

inline
Buffer stuff(Buffer b) { return throwOnFail(cobs::stuff(std::move(b))); }

inline
Buffer stuffDelim(Buffer b) { return throwOnFail(cobs::stuffAndDelim(std::move(b))); }

inline
Buffer copy(const Buffer& b) { return throwOnFail(b.copy()); }

inline
std::vector<Buffer> vec(std::initializer_list<Buffer> l) {
    std::vector<Buffer> retval;
    retval.reserve(l.size());
    for (const auto& b: l) {
        retval.emplace_back(copy(b));
    }
    return retval;
}

inline
std::vector<Buffer> vec(Buffer b) {
    std::vector<Buffer> retval;
    retval.emplace_back(std::move(b));
    return retval;
}

inline
std::vector<Buffer> copy(const std::vector<Buffer>& v) {
    std::vector<Buffer> retval;
    retval.reserve(v.size());
    for (const auto& b: v) {
        retval.emplace_back(copy(b));
    }
    return retval;
}

inline
boost::optional<Buffer> copy(const boost::optional<Buffer>& b) {
    return b ? copy(*b) : boost::optional<Buffer>();
}

template <typename Success>
Result<Success> copy(const Result<Success>& src) {
    return isOk(src) ? ok(copy(getOkUnsafe(src))) : fail<Success>(getFailUnsafe(src));
}
