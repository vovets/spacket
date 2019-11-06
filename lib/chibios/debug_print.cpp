#include <spacket/debug_print.h>

#include "chprintf.h"

#include <cstdarg>


void debugPrint_(const char *fmt, std::va_list args) {
    chvprintf(debugPrintStream, fmt, args);
}

void debugPrintFinish() {
    debugPrint("\r\n");
}
