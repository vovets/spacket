#pragma once

#include <spacket/result.h>
#include <spacket/fatal_error.h>

template <typename Success>
Result<Success> fatal(Error e) {
    FATAL_ERROR(toString(e));
    // will never reach here
    return fail<Success>(e);
}
