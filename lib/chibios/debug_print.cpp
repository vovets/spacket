#include <spacket/debug_print.h>

#include "chprintf.h"

void debugPrintStart() {
    chprintf(debugPrintStream, "D:%s: ", chRegGetThreadNameX(chThdGetSelfX()));
}

void debugPrint(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    chvprintf(debugPrintStream, fmt, ap);
    va_end(ap);
}

void debugPrintFinish() {
    chprintf(debugPrintStream, "\r\n");
}

void debugPrintLine(const char *fmt, ...) {
    va_list ap;
    debugPrintStart();
    va_start(ap, fmt);
    chvprintf(debugPrintStream, fmt, ap);
    va_end(ap);
    debugPrintFinish();
}
