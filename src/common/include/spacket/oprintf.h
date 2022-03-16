#pragma once

#include <printf.h>

template <typename Out>
int voprintf(Out& out, const char* format, va_list args) {
    return vfctprintf(Out::out, &out, format, args);
}

template <typename Out>
int oprintf(Out& out, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = voprintf(out, format, args);
    va_end(args);
    return n;
}
