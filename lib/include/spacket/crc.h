#pragma once

#include <spacket/impl/crc_p.h>

namespace crc {

template <typename Buffer>
Result<Buffer> check(Buffer b) {
    if (b.size() < 5) {
        return fail<Buffer>(toError(ErrorCode::CrcCheckBufferTooSmall));
    }
    if (crc(b.begin(), b.size()) != 0) {
        return fail<Buffer>(toError(ErrorCode::CrcBad));
    }
    b.resize(b.size() - 4);
    return ok(std::move(b));
}

template <typename Buffer>
Result<Buffer> append(Buffer b) {
    if (b.size() == 0) {
        return fail<Buffer>(toError(ErrorCode::CrcAppendZeroSizeBuffer));
    }
    if (Buffer::maxSize() - b.size() < 4) {
        return fail<Buffer>(toError(ErrorCode::CrcAppendBufferTooBig));
    }
    std::uint32_t crc = crc::crc(b.begin(), b.size());
    uint8_t* crcBegin = b.end();
    b.resize(b.size() + 4);
    for (std::size_t i = 0; i < 4; ++i) {
        crcBegin[i] = (crc & 0xff000000) >> 24;
        crc <<= 8;
    }
    return ok(std::move(b));
}

} // crc