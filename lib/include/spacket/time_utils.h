#pragma once

#include <spacket/time_utils_impl.h>

using Timeout = Duration;

constexpr Timeout INFINITE_TIMEOUT = infiniteTimeout();
constexpr Timeout IMMEDIATE_TIMEOUT = immediateTimeout();

template <typename Duration>
std::chrono::milliseconds toMs(Duration duration) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}
