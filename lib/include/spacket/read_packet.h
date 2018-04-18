#pragma once

#include <spacket/result.h>
#include <spacket/buffer.h>
#include <spacket/time_utils.h>

// source() must return fail<Buffer>(Error::Timeout) on timeout
template <typename Buffer>
using Source = std::function<Result<Buffer>(Timeout t, size_t maxRead)>;

template <typename Buffer>
struct ReadResult {
    Buffer packet;
    Buffer suffix;
};

// this function skips until synced
template<typename Buffer>
Result<ReadResult<Buffer>> readPacket(
    Source<Buffer> s,
    size_t maxRead,
    Timeout t);

// this function assumes that stream is synced (as presense of "prefix" suggests)
template<typename Buffer>
Result<ReadResult<Buffer>> readPacket(
    Source<Buffer> s,
    Buffer prefix,
    size_t maxRead,
    Timeout t);

#include <spacket/impl/read_packet-impl.h>
