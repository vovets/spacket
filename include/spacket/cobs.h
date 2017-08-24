#pragma once

#include <spacket/result.h>

#include "cobs/cobs.h"

namespace cobs {

template<typename Buffer>
Buffer stuff(Buffer source) {
    Buffer result(source.size() + source.size() / 254);
    auto r = ::stuff(
        source.begin(),
        source.size(),
        result.begin());
    return result.prefix(r);
}

template<typename Buffer>
Result<Buffer> unstuff(Buffer source) {
    Buffer result(source.size());
    auto r = cobs_decode(source.begin(), source.size(), result.begin());
    if (!r) { return fail<Buffer>(Error::CobsBadEncoding); }
    return result.prefix(r);
}

template<typename Buffer>
constexpr size_t maxUnstuffedSizeT() { return Buffer::maxSize() * 254 / 255; }

} // cobs
