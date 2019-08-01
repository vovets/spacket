#pragma once

#include <spacket/debug_print_impl.h>

#include <utility>

void debugPrint(const char *fmt, ...);
void debugPrintStart();
void debugPrintFinish();

void debugPrintLine(const char *fmt, ...);

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
    void dps() {}                   \
                                    \
    void dpf() {}

#define IMPLEMENT_DPX_FUNCTIONS                         \
                                                        \
    void dps() {                                        \
        debugPrintStart();                              \
    }                                                   \
                                                        \
    void dpf() {                                        \
        debugPrintFinish();                             \
    }

