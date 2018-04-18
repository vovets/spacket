#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>

#include "ch.h"
#include "hal_streams.h"

// this should be defined by app
extern BaseSequentialStream* errorReportStream;

Result<boost::blank> threadErrorReport(Error e);
