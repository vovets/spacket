#pragma once

#include <spacket/time_utils_impl.h>

using Timeout = Duration;

constexpr Timeout INFINITE_TIMEOUT = infiniteTimeout();
constexpr Timeout IMMEDIATE_TIMEOUT = immediateTimeout();

// until c++17 goes this replacement
template <class To, class Rep, class Period>
constexpr
To ceil(const std::chrono::duration<Rep, Period>& d)
{
    auto t = std::chrono::duration_cast<To>(d);
    if (t < d)
        return t + To{1};
    return t;
}

template <typename Duration>
std::chrono::milliseconds toMs(Duration duration) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}
