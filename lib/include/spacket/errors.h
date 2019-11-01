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
    X(ID(SerialDeviceInitFailed),    CODE(400),   SEP())  \
    X(ID(SerialDeviceReadError),     CODE(401),   SEP())  \
    X(ID(SerialDeviceWriteError),    CODE(402),   SEP())  \
    X(ID(WriteTimeout),              CODE(403),   SEP())  \
    X(ID(ReadTimeout),               CODE(404),   SEP())  \
    X(ID(SerialDeviceAlreadyOpened), CODE(405),   SEP())  \
    X(ID(BufferCreateTooBig),        CODE(500),   SEP())  \
    X(ID(CobsBadEncoding),           CODE(600),   SEP())  \
    X(ID(PoolAllocatorObjectTooBig), CODE(700),   SEP())  \
    X(ID(ChMsgTimeout),              CODE(800),   SEP())  \
    X(ID(ChMsgReset),                CODE(801),   SEP())  \
    X(ID(PacketizerOverflow),        CODE(900),   SEP())  \
    X(ID(PacketDropped),             CODE(1000),  SEP())  \
    X(ID(PacketTooBig),              CODE(1001),  SEP())  \
    X(ID(CrcBad),                    CODE(1200),  SEP())  \
    X(ID(CrcAppendBufferTooBig),     CODE(1201),  SEP())  \
    X(ID(CrcAppendZeroSizeBuffer),   CODE(1202),  SEP())  \
    X(ID(CrcCheckBufferTooSmall),    CODE(1203),  SEP())  \
    X(ID(MultiplexerBadChannel),     CODE(1300),  SEP())  \
    X(ID(MultiplexerBufferTooBig),   CODE(1301),  SEP())  \
    X(ID(MultiplexerBufferTooSmall), CODE(1302),  SEP())  \
    X(ID(MiserableFailure0),         CODE(10000), SEP())  \
    X(ID(MiserableFailure1),         CODE(10001), SEP())  \
    X(ID(MiserableFailure9),         CODE(10010), )
#endif

struct Error {
    using Code = unsigned;
    Code code;
    bool operator==(const Error& rhs) const {
        return code == rhs.code;
    }
};

enum class ErrorCode: Error::Code {
#define X(ID, CODE, SEP) ID = CODE SEP
    ERROR_LIST(ID, ID, SEP_COMMA)
#undef X
};

template <typename ErrorCode>
constexpr typename std::underlying_type_t<ErrorCode> toInt(ErrorCode e) noexcept {
    return static_cast<typename std::underlying_type_t<ErrorCode>>(e);
}

template <typename ErrorCode>
constexpr Error toError(ErrorCode e) noexcept {
    return Error{toInt(e)};
}

const char* toString(Error e);

using ErrorToStringF = const char*(*)(Error);

ErrorToStringF setErrorToStringHandler(ErrorToStringF handler);
