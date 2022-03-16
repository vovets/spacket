#pragma once

#include <spacket/debug_print.h>
#include <spacket/buffer.h>

#include <cstdarg>


namespace dbg {

inline
void write(const Buffer& b) {
    write(b.begin(), b.size());
}

template<typename Out>
void pb(Out& out, const Buffer& b) {
    oprintf(out, "%X %d:[", b.id(), b.size());

#ifdef DEBUG_PRINT_BUFFER_ASCII
#define DP(C) do { \
if (unsigned(C) < 32 || unsigned(C) > 127) { oprintf(out, "%02x", (C)); } \
else { debugPrint("%c", (C)); } \
} while(false)
#else
#define DP(C) oprintf(out, "%02x", (C))
#endif

#ifdef DEBUG_PRINT_BUFFER_FULL
#ifndef DEBUG_PRINT_BUFFER_ASCII
    bool first = true;
#endif
    for (auto c: b) {
#ifndef DEBUG_PRINT_BUFFER_ASCII
        if (!first) { oprintf(out, ", "); } else { first = false; }
#endif
DP(c);
    }
#elif defined DEBUG_PRINT_BUFFER_FIRST_ONLY
std::size_t i = 0;
    for (;
    i < std::min(b.size(), 3u); ++i) {
        if (i) { oprintf(out, ", "); }
        oprintf(out, "%02x", *(b.begin() + i));
    }
    if (b.size() > i) { oprintf(out, ", ..."); }
#endif
    oprintf(out, "]");
}

} // dbg

inline
void debugPrintBuffer_(const char* fmt, const Buffer& b, std::va_list args) {
    dbg::ArrayOut out;
    dbg::prefix(out);
    voprintf(out, fmt, args);
    dbg::pb(out, b);
    dbg::suffix(out);
    dbg::write(out);
}

#define IMPLEMENT_DPB_FUNCTION                          \
    void dpb(const char* fmt, const Buffer* b, ...) {   \
        std::va_list args;                              \
        va_start(args, b);                              \
        debugPrintBuffer_(fmt, *b, args);               \
        va_end(args);                                   \
    }


#define IMPLEMENT_DPB_FUNCTION_NOP                  \
    void dpb(const char*, const Buffer*, ...) {}

#define DPB(VAR_NAME) dpb(#VAR_NAME ": ", &VAR_NAME)
