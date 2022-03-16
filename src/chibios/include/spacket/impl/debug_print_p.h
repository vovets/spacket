#pragma once

#include <spacket/fatal_error.h>
#include <spacket/oprintf.h>

#include "ch.h"

#include <SEGGER_RTT.h>

#include <cstdarg>
#include <algorithm>
#include <spacket/thread.h>

namespace dbg {

inline
void write(const void* buffer, std::size_t size) {
    syssts_t s = chSysGetStatusAndLockX();
    SEGGER_RTT_WriteSkipNoLock(0, buffer, size);
    chSysRestoreStatusX(s);
}

#ifndef SPACKET_DEBUG_PRINT_BUFFER_SIZE
#define SPACKET_DEBUG_PRINT_BUFFER_SIZE 512
#endif

constexpr std::size_t printBufferSize = SPACKET_DEBUG_PRINT_BUFFER_SIZE;

template<std::size_t BUFFER_SIZE = printBufferSize>
struct ArrayOut {
    std::array<char, BUFFER_SIZE> buffer;
    std::size_t writeIdx;

    ArrayOut(): writeIdx(0) {}

    static void out(char c, void* arg) {
        static_cast<ArrayOut*>(arg)->out_(c);
    }

    void out_(char c) {
        if (writeIdx < buffer.size()) {
            buffer[writeIdx++] = c;
        }
    }
};

template<typename Out>
void write(const Out& out) {
    write(out.buffer.begin(), out.writeIdx);
}

inline
int vp(const char* fmt, std::va_list args) {
    ArrayOut out;
    int n = voprintf(out, fmt, args);
    write(out);
    return n;
}

inline
int p(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int n = vp(format, args);
    va_end(args);
    return n;
}

template<typename Out>
void prefix(Out& out) {
    oprintf(out, "D|%s|", chRegGetThreadNameX(chThdGetSelfX()));
}

template<typename Out>
void suffix(Out& out) {
    oprintf(out, "\r\n");
}

} // dbg
