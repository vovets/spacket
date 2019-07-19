#pragma once

#include <spacket/debug_print_impl.h>

#include <utility>

void debugPrint(const char *fmt, ...);
void debugPrintStart();
void debugPrintFinish();

void debugPrintLine(const char *fmt, ...);

#define IMPLEMENT_DPX_FUNCTIONS_NOP \
                                    \
    template <typename ...Args>     \
    void dpl(Args...) {}            \
                                    \
    inline                          \
    void dps() {}                   \
                                    \
    inline                          \
    void dpf() {}

#define IMPLEMENT_DPX_FUNCTIONS                         \
                                                        \
    template <typename ...Args>                         \
    void dpl(Args... args) {                            \
        debugPrintLine(std::forward<Args>(args)...);    \
    }                                                   \
                                                        \
    inline                                              \
    void dps() {                                        \
        debugPrintStart();                              \
    }                                                   \
                                                        \
    inline                                              \
    void dpf() {                                        \
        debugPrintFinish();                             \
    }

