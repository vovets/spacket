#pragma once

#include <spacket/macro_utils.h>

#include <type_traits>

#ifdef ERROR_LIST
#error "ERROR_LIST macro should not be defined here"
#else
#define ERROR_LIST(ID, CODE, SEP)                \
    X(ID(Timeout),              CODE(1),  SEP()) \
    X(ID(ConfigNoDevSpecified), CODE(10), SEP()) \
    X(ID(ConfigBadBaud),        CODE(11), SEP()) \
    X(ID(DevInitFailed),        CODE(20), SEP()) \
    X(ID(DevReadFailed),        CODE(21), SEP()) \
    X(ID(DevWriteFailed),       CODE(22), SEP()) \
    X(ID(PacketTooBig),         CODE(30), SEP()) \
    X(ID(CobsBadEncoding),      CODE(40), )
#endif

enum class Error {
#define X(ID, CODE, SEP) ID = CODE SEP
    ERROR_LIST(ID, ID, SEP_COMMA)
#undef X
};

constexpr typename std::underlying_type<Error>::type toInt(Error e) noexcept {
    return static_cast<typename std::underlying_type<Error>::type>(e);
}

const char* toString(Error e);
