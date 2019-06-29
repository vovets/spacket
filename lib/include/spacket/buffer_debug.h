#pragma once

#include <spacket/debug_print.h>

#ifdef BUFFER_ENABLE_DEBUG_PRINT_BUFFER

void debugPrintBuffer(const char* message, const Buffer& b) {
    debugPrintStart();
    debugPrint("%s: %x %d:[", message, b.id(), b.size());
#ifdef BUFFER_ENABLE_DEBUG_PRINT_BUFFER_FULL
    bool first = true;
    for (auto c: b) {
        if (!first) { debugPrint(", "); } else { first = false; }
        debugPrint("%x", c);
    }
#elif defined BUFFER_ENABLE_DEBUG_PRINT_BUFFER_FIRST_ONLY
    std::size_t i = 0;
    for (;
         i < std::min(b.size(), 3u); ++i) {
        if (i) { debugPrint(", "); }
        debugPrint("%x", *(b.begin() + i));
    }
    if (b.size() > i) { debugPrint(", ..."); }
#endif
    debugPrint("]");
    debugPrintFinish();
}

#else

void debugPrintBuffer(const char*, const Buffer&) {}

#endif

#define DEBUG_PRINT_BUFFER(VAR_NAME) debugPrintBuffer(#VAR_NAME, VAR_NAME)
