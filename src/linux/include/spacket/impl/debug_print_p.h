#include <cstdio>
#include <cstdarg>
#include <sstream>

namespace {

const auto debugPrintStream = stderr;

} // namespace

void debugPrint(const char *fmt, ...);

inline
void debugPrint_(const char *fmt, std::va_list args) {
    vfprintf(debugPrintStream, fmt, args);
}

inline
void debugPrintFinish() {
    debugPrint("\n");
}
