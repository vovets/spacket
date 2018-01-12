#pragma once

#include <spacket/macro_utils.h>

#include <cstdint>

#ifdef BAUD_LIST
#error macro BAUD_LIST already defined
#else
#define BAUD_LIST(BAUD, SEP)                    \
    X(BAUD(9600), SEP())                        \
    X(BAUD(38400), SEP())                       \
    X(BAUD(115200), SEP())                      \
    X(BAUD(230400), SEP())                      \
    X(BAUD(460800), SEP())                      \
    X(BAUD(921600), )
#endif

enum class Baud: std::size_t {
    NonStandard = 0,
#define BAUD(X) B_##X = X // underscore used to not clash with BNNNN defines from termios.h
#define X(BAUD, SEP) BAUD SEP
    BAUD_LIST(BAUD, SEP_COMMA)
#undef X
#undef BAUD
};

Baud fromInt(std::size_t b);

inline
constexpr
std::size_t toInt(Baud b) {
    return static_cast<std::size_t>(b);
}
