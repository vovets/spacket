#pragma once

#include <spacket/result.h>
#include <spacket/cobs/cobs.h>

namespace cobs {

constexpr size_t maxStuffedSize(size_t payloadSize) {
    return payloadSize + 1 + payloadSize / 254;
}

constexpr size_t maxStuffedAndDelimitedSize(size_t payloadSize) {
    return maxStuffedSize(payloadSize) + 2;
}

constexpr size_t maxPayloadSize(size_t stuffedSize) { return (stuffedSize - 1) * 254 / 255; }

constexpr size_t maxPayloadSizeDelimited(size_t stuffedAndDelimitedSize) {
    return (stuffedAndDelimitedSize - 2 - 1) * 254 / 255;
}

inline
Result<Buffer> stuff(Buffer source_) {
    // Lifetime of arguments may extend to the end of expression containing
    // function call. Here we ensure that source buffer released at exit of this function
    Buffer source(std::move(source_));
    alloc::Allocator& allocator = source.allocator();
    returnOnFail(result, Buffer::create(allocator, maxStuffedSize(source.size())));
    auto r = ::stuff(
        source.begin(),
        source.size(),
        result.begin());
    result.resize(r);
    return ok(std::move(result));
}

inline
Result<Buffer> stuffAndDelim(Buffer source_) {
    Buffer source(std::move(source_));
    alloc::Allocator& allocator = source.allocator();
    returnOnFail(result, Buffer::create(allocator, maxStuffedAndDelimitedSize(source.size())));
    auto r = ::stuff(
        source.begin(),
        source.size(),
        result.begin() + 1);
    *result.begin() = 0;
    *(result.begin() + r + 1) = 0;
    result.resize(r + 2);
    return ok(std::move(result));
}

inline
Result<Buffer> unstuff(Buffer source_) {
    Buffer source(std::move(source_));
    alloc::Allocator& allocator = source.allocator();
    returnOnFail(result, Buffer::create(allocator, source.size()));
    auto r = ::unstuff(source.begin(), source.size(), result.begin());
    if (!r) { return fail<Buffer>(toError(ErrorCode::CobsBadEncoding)); }
    result.resize(r);
    return ok(std::move(result));
}

} // cobs
