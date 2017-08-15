#pragma once

#include <spacket/macro_utils.h>

#include <string>

#ifdef BAUD_LIST
#error macro BAUD_LIST already defined
#else
#define BAUD_LIST(BAUD, SEP)                    \
    X(BAUD(9600), SEP())                        \
    X(BAUD(38400), SEP())                       \
    X(BAUD(115200), SEP())                      \
    X(BAUD(230400), )
#endif

enum class Baud: size_t {
    NonStandard = 0,
#define BAUD(X) B_##X = X // underscore used to not clash with BNNNN defines from termios.h
#define X(BAUD, SEP) BAUD SEP
    BAUD_LIST(BAUD, SEP_COMMA)
#undef X
#undef BAUD
};

Baud fromInt(size_t b);

inline
constexpr
size_t toInt(Baud b) {
    return static_cast<size_t>(b);
}

struct PortConfig {
    std::string device;
    Baud baud;
    size_t byteTimeout_us; // 0 means 2 * byte interval according to baud
};
