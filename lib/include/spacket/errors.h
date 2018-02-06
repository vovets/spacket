#pragma once

#include <spacket/macro_utils.h>

#include <type_traits>

#ifdef ERROR_LIST
#error "ERROR_LIST macro should not be defined here"
#else
#define ERROR_LIST(ID, CODE, SEP)                       \
    X(ID(Timeout),                   CODE(100),   SEP())  \
    X(ID(GuardedPoolOutOfMem),       CODE(200),   SEP())  \
    X(ID(ConfigNoDevSpecified),      CODE(300),   SEP())  \
    X(ID(ConfigBadBaud),             CODE(301),   SEP())  \
    X(ID(DevInitFailed),             CODE(400),   SEP())  \
    X(ID(DevReadFailed),             CODE(401),   SEP())  \
    X(ID(DevWriteFailed),            CODE(402),   SEP())  \
    X(ID(DevWriteTimeout),           CODE(403),   SEP())  \
    X(ID(DevReadTimeout),            CODE(404),   SEP())  \
    X(ID(DevAlreadyOpened),          CODE(405),   SEP())  \
    X(ID(PacketTooBig),              CODE(500),   SEP())  \
    X(ID(CobsBadEncoding),           CODE(600),   SEP())  \
    X(ID(PoolAllocatorObjectTooBig), CODE(700),   SEP())  \
    X(ID(ChMsgTimeout),              CODE(800),   SEP())  \
    X(ID(ChMsgReset),                CODE(801),   SEP())  \
    X(ID(MiserableFailure0),         CODE(10000), SEP())  \
    X(ID(MiserableFailure1),         CODE(10001), SEP())  \
    X(ID(MiserableFailure9),         CODE(10010), )
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
