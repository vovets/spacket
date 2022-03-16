#pragma once

#include <spacket/impl/debug_print_p.h>

#include <utility>
#include <cstdarg>


inline
void debugPrintLine_(const char *fmt, ...) {
    dbg::ArrayOut out;
    std::va_list args;
    dbg::prefix(out);
    va_start(args, fmt);
    voprintf(out, fmt, args);
    va_end(args);
    dbg::suffix(out);
    dbg::write(out);
}

#define IMPLEMENT_DPL_FUNCTION                          \
                                                        \
    template <typename ...Args>                         \
    void dpl(Args&&... args) {                          \
        debugPrintLine_(std::forward<Args>(args)...);    \
    }

#define IMPLEMENT_DPL_FUNCTION_NOP  \
                                    \
    template <typename ...Args>     \
    void dpl(Args&&...) {}
