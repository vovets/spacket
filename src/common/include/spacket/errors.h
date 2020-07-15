#pragma once

#include <spacket/macro_utils.h>

#include <type_traits>
#include <cstdint>
#include <array>

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
    X(ID(UartNothingToWait),         CODE(1400),  SEP())  \
    X(ID(UartRxTimeout),             CODE(1401),  SEP())  \
    X(ID(UartTxTimeout),             CODE(1402),  SEP())  \
    X(ID(ModulePacketDropped),       CODE(1501),  SEP())  \
    X(ID(ModuleNoLower),             CODE(1502),  SEP())  \
    X(ID(ModuleNoUpper),             CODE(1503),  SEP())  \
    X(ID(ModuleRxEmpty),             CODE(1504),  SEP())  \
    X(ID(ModuleNoModules),           CODE(1505),  SEP())  \
    X(ID(ModuleTxQueueFull),         CODE(1506),  SEP())  \
    X(ID(StackRxRequestFull),        CODE(1600),  SEP())  \
    X(ID(StackIORingFull),           CODE(1601),  SEP())  \
    X(ID(StackProcRingFull),         CODE(1602),  SEP())  \
    X(ID(AddressNoRoom),             CODE(1700),  SEP())  \
    X(ID(LastCode),                  CODE(10000),)
#endif

struct Error {
    using Source = std::uint16_t;
    using Code = std::uint16_t;

    Source source;
    Code code;

    bool operator==(const Error& rhs) const {
        return source == rhs.source && code == rhs.code;
    }

    bool operator!=(const Error& rhs) const {
        return !operator==(rhs);
    }
};

constexpr Error::Source SPACKET_ERROR_SOURCE = 0;

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
    return Error{SPACKET_ERROR_SOURCE, toInt(e)};
}

constexpr Error toError(Error::Source s, Error::Code e) noexcept {
    return Error{s, e};
}

const char* toString(ErrorCode e);

const char* toString(Error e, char* buffer, std::size_t size);

using DefaultToStringBuffer = std::array<char, 5+1+5+1>;

template <typename Array>
const char* toString(Error e, Array& a) {
    return toString(e, a.data(), a.size());
}

#include <spacket/impl/errors_p.h>
