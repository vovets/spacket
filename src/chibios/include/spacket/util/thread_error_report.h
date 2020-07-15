#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>

#include "ch.h"
#include "hal_streams.h"
#include "chprintf.h"

#include <array>


// this should be defined by the app
extern BaseSequentialStream* errorReportStream;

template <typename Success = Void>
Result<Success> threadErrorReportT(Error e) {
    std::array<char, 12> buffer;
    chprintf(errorReportStream, "E|%s|%s\r\n", Thread::getName(), toString(e, buffer.data(), buffer.size()));
    return fail<Success>(e);
}

inline
Result<Void> threadErrorReport(Error e) {
    return threadErrorReportT<Void>(e);
}
