#pragma once

Buffer buf(size_t size) { return std::move(throwOnFail(Buffer::create(size))); }

Buffer buf(size_t size, uint8_t fill) {
    Buffer b = throwOnFail(Buffer::create(size));
    for (auto& c: b) {
        c = fill;
    }
    return b;
}

Buffer buf(std::initializer_list<uint8_t> l) {
    return throwOnFail(Buffer::create(l));
}

Buffer cob(Buffer b) {
    return throwOnFail(cobs::stuffAndDelim(std::move(b)));
}

Buffer nz(std::size_t size) { return createTestBufferNoZero<Buffer>(size); }

Result<Buffer> cat_(const Buffer& l, const Buffer& r) {
    return l + r;
}

Result<Buffer> cat_(Result<Buffer> l, const Buffer& r) {
    return std::move(l) >= [&] (Buffer&& b) { return std::move(b) + r; };
}

template <typename Arg1, typename Arg2, typename ...Args>
Result<Buffer> cat_(const Arg1& arg1, const Arg2& arg2, Args ...args) {
    return cat_(cat_(arg1, arg2), args...);
}

template <typename ...Args>
Buffer cat(Args ...args) {
    return throwOnFail(cat_(args...));
}

Buffer stuff(Buffer b) { return throwOnFail(cobs::stuff(std::move(b))); }

Buffer stuffDelim(Buffer b) { return throwOnFail(cobs::stuffAndDelim(std::move(b))); }
