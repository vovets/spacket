#pragma once

#include <spacket/macro_utils.h>

#include <type_traits>
#include <cstdint>

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
    X(ID(MultiplexerBufferTooSmall), CODE(1302),)
#endif

struct Error {
    using Code = std::uint16_t;
    using Source = std::uint16_t;

    Source source;
    Code code;

    bool operator==(const Error& rhs) const {
        return source == rhs.source && code == rhs.code;
    }
};

constexpr Error::Source ERROR_SOURCE = 0;

enum class ErrorCode: Error::Code {
#define X(ID, CODE, SEP) ID = CODE SEP
    ERROR_LIST(ID, ID, SEP_COMMA)
#undef X
};

template <typename ErrorCode>
constexpr typename std::underlying_type_t<ErrorCode> toInt(ErrorCode e) noexcept {
    return static_cast<typename std::underlying_type_t<ErrorCode>>(e);
}

inline
constexpr Error toError(ErrorCode e) noexcept {
    return Error{ERROR_SOURCE, toInt(e)};
}

constexpr Error toError(Error::Source s, Error::Code e) noexcept {
    return Error{s, e};
}

const char* toString(ErrorCode e);

const char* toString(Error e, char* buffer, std::size_t size);

using ErrorToStringF = const char*(*)(Error);

ErrorToStringF setErrorToStringHandler(ErrorToStringF handler);
