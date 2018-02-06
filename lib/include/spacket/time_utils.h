#pragma once

#include <spacket/time_utils_impl.h>

using Timeout = Duration;

constexpr Timeout INFINITE_TIMEOUT = infiniteTimeout();
constexpr Timeout IMMEDIATE_TIMEOUT = immediateTimeout();
