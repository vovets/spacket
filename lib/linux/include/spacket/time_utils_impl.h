#pragma once

#include <chrono>

using Clock = std::chrono::steady_clock;
using Duration = Clock::duration;

constexpr Duration infiniteTimeout() { return Duration::max(); }
