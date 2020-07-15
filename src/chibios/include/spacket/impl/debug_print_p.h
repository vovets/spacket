#pragma once

#include "ch.h"
#include "hal_streams.h"
#include "chprintf.h"

#include <cstdarg>

void debugPrint(const char *fmt, ...);

// this should be defined by app
extern BaseSequentialStream* debugPrintStream;

inline
void debugPrint_(const char *fmt, std::va_list args) {
    chvprintf(debugPrintStream, fmt, args);
}

inline
void debugPrintFinish() {
    debugPrint("\r\n");
}
