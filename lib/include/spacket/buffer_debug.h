#pragma once

#include <spacket/debug_print.h>

#ifdef BUFFER_ENABLE_DEBUG_PRINT_BUFFER

void debugPrintBuffer(const char* message, const Buffer& b) {
    debugPrintStart();
    debugPrint("%s: %x %d:[", message, b.id(), b.size());
#ifndef BUFFER_ENABLE_DEBUG_PRINT_BUFFER_SIZE_ONLY
    bool first = true;
    for (auto c: b) {
        if (!first) { chprintf(&rttStream, ", "); } else { first = false; }
        debugPrint("%x", c);
    }
#endif
    debugPrint("]");
    debugPrintFinish();
}

#else

void debugPrintBuffer(const char*, const Buffer&) {}

#endif

#define DEBUG_PRINT_BUFFER(VAR_NAME) debugPrintBuffer(#VAR_NAME, VAR_NAME)
