#include <spacket/debug_print.h>

#include <cstdio>
#include <cstdarg>

namespace {

const auto debugPrintStream = stderr;

} // namespace

void debugPrintStart() {
    fprintf(debugPrintStream, "D: ");
}

void debugPrint(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(debugPrintStream, fmt, ap);
    va_end(ap);
}

void debugPrintFinish() {
    fprintf(debugPrintStream, "\n");
}

void debugPrintLine(const char *fmt, ...) {
    va_list ap;
    debugPrintStart();
    va_start(ap, fmt);
    vfprintf(debugPrintStream, fmt, ap);
    va_end(ap);
    debugPrintFinish();
}
