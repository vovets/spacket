#pragma once

#include <ch.h>

namespace check_stack_impl {

inline
void checkStack() {
    std::uint32_t* base = reinterpret_cast<uint32_t*>(chThdGetWorkingAreaX(chThdGetSelfX()));
    if (base[0] != 0x55555555U || base[1] != 0x55555555U) {
        chSysHalt("stack base overrun");
    }
}

} // check_stack_impl
