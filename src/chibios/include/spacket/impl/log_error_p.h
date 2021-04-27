#pragma once

#include <spacket/errors.h>

#include "ch.h"
#include "hal_streams.h"
#include "chprintf.h"

#include <array>


// this should be defined by the app
extern BaseSequentialStream* errorStream;

namespace impl {

void logError(Error e) {
    std::array<char, 12> buffer;
    chprintf(errorStream, "E|%s|%s\r\n", Thread::getName(), toString(e, buffer.data(), buffer.size()));
}

} // impl
