#pragma once

#include <spacket/time_utils.h>

#include <chrono>

constexpr Timeout packetTime64(uint32_t baudRate, size_t numBytes) {
    // TODO: take into account possible stop bit variation and parity
    constexpr uint32_t bitsInByte = 10;
    constexpr uint64_t nanoInS = 1000000000;
    return
    ceil<Timeout>(
        std::chrono::nanoseconds(
            nanoInS * bitsInByte * numBytes / baudRate));
}

// this is good up to numBytes == ((uint32_t)-1 - baudRate + 1) / (1000000 * 10)
// which equals 429 for 921600 baud
// for larger sizes use packetTime64
constexpr Timeout packetTime(uint32_t baudRate, size_t numBytes) {
    // TODO: take into account possible stop bit variation and parity
    constexpr uint32_t bitsInByte = 10;
    constexpr uint32_t microInS = 1000000;
    return
    ceil<Timeout>(
        std::chrono::microseconds(
            // here we want extending toward ceiling
            (microInS * bitsInByte * numBytes + baudRate - 1) / baudRate));
}
