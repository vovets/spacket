#pragma once

#include "ch.h"

#include <chrono>

using Duration = std::chrono::duration<systime_t, std::ratio<1, CH_CFG_ST_FREQUENCY>>;

constexpr Duration infiniteTimeout() { return Duration::max(); }
constexpr Duration immediateTimeout() { return Duration::min(); }

constexpr systime_t toSystime(Duration d) {
    return d ==  infiniteTimeout() ? TIME_INFINITE : d.count();
}
