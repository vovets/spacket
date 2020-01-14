#pragma once

#include <spacket/result.h>
#include <spacket/fatal_error.h>

const char* toStringFatal(Error e);

template <typename Success>
Result<Success> fatal(Error e) {
    FATAL_ERROR(toStringFatal(e));
    // will never reach here
    return fail<Success>(e);
}
