#pragma once

#include <spacket/errors.h>
#include <spacket/debug_print.h>

#include "ch.h"

#include <array>


namespace impl {

void logError(Error e) {
    dbg::ArrayOut<128> out;
    std::array<char, 12> buffer;
    oprintf(out, "E|%s|%s\r\n", chRegGetThreadNameX(chThdGetSelfX()), toString(e, buffer.data(), buffer.size()));
    dbg::write(out);
}

} // impl
