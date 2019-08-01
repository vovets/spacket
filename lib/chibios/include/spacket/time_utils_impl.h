#pragma once

#include "ch.h"

#include <chrono>

using Duration = std::chrono::duration<systime_t, std::ratio<1, CH_CFG_ST_FREQUENCY>>;

constexpr Duration infiniteTimeout() { return Duration::max(); }
constexpr Duration immediateTimeout() { return Duration::min(); }

constexpr systime_t toSystime(Duration d) {
    return d ==  infiniteTimeout() ? TIME_INFINITE : d.count();
}

constexpr systime_t minTimeout() {
    constexpr systime_t timedelta = CH_CFG_ST_TIMEDELTA;
    return timedelta > 0 ? timedelta : 1;
}

template <typename ToDuration, std::uint32_t CPU_FREQ>
std::uint32_t to(rtcnt_t counter) {
    return std::chrono::duration_cast<ToDuration>(
    std::chrono::duration<rtcnt_t, std::ratio<1, CPU_FREQ>>(counter)
    ).count();
}
