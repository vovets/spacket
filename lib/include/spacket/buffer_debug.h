#pragma once

#include <spacket/debug_print.h>

template <typename Buffer>
void debugPrintBuffer(const Buffer& b) {
    debugPrint("%p %d:[", b.id(), b.size());
#ifdef DEBUG_PRINT_BUFFER_FULL
    bool first = true;
    for (auto c: b) {
        if (!first) { debugPrint(", "); } else { first = false; }
        debugPrint("%02x", c);
    }
#elif defined DEBUG_PRINT_BUFFER_FIRST_ONLY
    std::size_t i = 0;
    for (;
         i < std::min(b.size(), 3u); ++i) {
        if (i) { debugPrint(", "); }
        debugPrint("%02x", *(b.begin() + i));
    }
    if (b.size() > i) { debugPrint(", ..."); }
#endif
    debugPrint("]");
}

template <typename Buffer>
void debugPrintBuffer(const char* message, const Buffer& b) {
    debugPrintStart();
    debugPrint("%s", message);
    debugPrintBuffer(b);
    debugPrintFinish();
}

#define IMPLEMENT_DPB_FUNCTION                          \
    void dpb(const char* message, const Buffer& b) {    \
        debugPrintBuffer(message, b);                   \
    }

#define IMPLEMENT_DPB_FUNCTION_NOP                      \
    void dpb(const char*, const Buffer&) {}

#define DPB(VAR_NAME) dpb(#VAR_NAME ": ", VAR_NAME)
