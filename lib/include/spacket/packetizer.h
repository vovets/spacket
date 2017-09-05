#pragma once

#include <spacket/result.h>
#include <spacket/buffer.h>
#include <spacket/time_utils.h>

template <typename Buffer>
using Source = std::function<Result<Buffer>(Timeout t, size_t maxRead)>;

template <typename Buffer>
struct ReadResult {
    Buffer packet;
    Buffer suffix;
};

template<typename Buffer>
Result<ReadResult<Buffer>> readPacket(
    Source<Buffer> s,
    Buffer next,
    size_t maxRead,
    size_t maxPacketSize,
    Timeout t);

template<typename Buffer>
Result<ReadResult<Buffer>> readPacket(Source<Buffer> s, Buffer prefix, size_t maxRead, Timeout t);

#include <spacket/impl/packetizer-impl.h>
