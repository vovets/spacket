#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>

#include "ch.h"
#include "hal_streams.h"
#include "chprintf.h"


// this should be defined by the app
extern BaseSequentialStream* errorReportStream;

template <typename Success = boost::blank>
Result<Success> threadErrorReportT(Error e) {
    chprintf(errorReportStream, "E:%s: %s\r\n", chRegGetThreadNameX(chThdGetSelfX()), toString(e));
    return fail<Success>(e);
}

inline
Result<boost::blank> threadErrorReport(Error e) {
    return threadErrorReportT<boost::blank>(e);
}
