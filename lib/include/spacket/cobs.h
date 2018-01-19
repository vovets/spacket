#pragma once

#include <spacket/result.h>
#include <spacket/cobs/cobs.h>

namespace cobs {

template<typename Buffer>
Result<Buffer> stuff(Buffer source) {
    returnOnFail(result, Buffer::create(source.size() + source.size() / 254 + 1));
    auto r = ::stuff(
        source.begin(),
        source.size(),
        result.begin());
    return result.prefix(r);
}

template<typename Buffer>
Result<Buffer> unstuff(Buffer source) {
    returnOnFail(result, Buffer::create(source.size()));
    auto r = ::unstuff(source.begin(), source.size(), result.begin());
    if (!r) { return fail<Buffer>(Error::CobsBadEncoding); }
    return result.prefix(r);
}

template<typename Buffer>
constexpr size_t maxUnstuffedSizeT() { return (Buffer::maxSize() - 1) * 254 / 255; }

} // cobs
