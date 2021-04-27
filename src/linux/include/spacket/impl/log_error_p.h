#pragma once

#include <spacket/thread.h>
#include <spacket/errors.h>

#include <iostream>

namespace impl {

void logError(Error e) {
    std::array<char, 256> buffer;
    std::cerr
    << "E|" << Thread::getName()
    << "|" << toString(e, buffer) << std::endl;
}

} // impl
