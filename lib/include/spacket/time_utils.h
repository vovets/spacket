#pragma once

#include <chrono>

using Clock = std::chrono::steady_clock;
using Timestamp = Clock::time_point;
using Duration = Clock::duration;
using Timeout = Duration;
