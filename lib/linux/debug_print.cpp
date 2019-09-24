#include <spacket/debug_print.h>

#include <thread>

#include <cstdio>
#include <cstdarg>
#include <sstream>

namespace {

const auto debugPrintStream = stderr;

} // namespace

void debugPrintStart() {
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    fprintf(debugPrintStream, "D: %s:", ss.str().c_str());
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
