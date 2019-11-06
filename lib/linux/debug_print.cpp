#include <spacket/debug_print.h>
#include <spacket/thread.h>

#include <thread>

#include <cstdio>
#include <cstdarg>
#include <sstream>

namespace {

const auto debugPrintStream = stderr;

} // namespace

void debugPrint_(const char *fmt, std::va_list args) {
    vfprintf(debugPrintStream, fmt, args);
}

void debugPrintFinish() {
    debugPrint("\n");
}
