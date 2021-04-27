#pragma once

#include <spacket/impl/debug_print_p.h>
#include <spacket/thread.h>

#include <utility>
#include <cstdarg>


void debugPrint_(const char *fmt, std::va_list args);
void debugPrintFinish();

inline
void debugPrint(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    debugPrint_(fmt, args);
    va_end(args);
}

inline
void debugPrintStart() {
    debugPrint("D|%s|", Thread::getName());
}

inline
void debugPrintLine(const char *fmt, ...) {
    std::va_list args;
    debugPrintStart();
    va_start(args, fmt);
    debugPrint_(fmt, args);
    va_end(args);
    debugPrintFinish();
}

#define IMPLEMENT_DPL_FUNCTION                          \
                                                        \
    template <typename ...Args>                         \
    void dpl(Args&&... args) {                          \
        debugPrintLine(std::forward<Args>(args)...);    \
    }

#define IMPLEMENT_DPL_FUNCTION_NOP  \
                                    \
    template <typename ...Args>     \
    void dpl(Args&&...) {}

#define IMPLEMENT_DPX_FUNCTIONS_NOP \
                                    \
    PREFIX()                        \
    void dps() {}                   \
                                    \
    PREFIX()                        \
    void dpf() {}                   \
                                    \
    template <typename ...Args>     \
    void dp(Args&&...) {}

#define IMPLEMENT_DPX_FUNCTIONS                         \
                                                        \
    PREFIX()                                            \
    void dps() {                                        \
        debugPrintStart();                              \
    }                                                   \
                                                        \
    PREFIX()                                            \
    void dpf() {                                        \
        debugPrintFinish();                             \
    }                                                   \
                                                        \
    template <typename ...Args>                         \
    void dp(Args&&...args) {                            \
        debugPrint(std::forward<Args>(args)...);        \
    }