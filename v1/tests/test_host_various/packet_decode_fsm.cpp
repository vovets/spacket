#include "namespaces.h"
#include "utils.h"

#include <spacket/impl/packet_decode_fsm.h>
#include <spacket/buffer.h>
#include <spacket/buffer_new_allocator.h>

#include <catch.hpp>

#include "buf.h"

namespace Catch {
template<> struct StringMaker<Result<Void>>: public StringMakerResultBase<Result<Void>> {};
}

constexpr Error::Source ERROR_SOURCE = 10;

struct Case {
    Buffer input;
    std::vector<Buffer> expectedOutput;
    Result<Void> expectedResult;
    std::size_t expectedInputConsumed;
};

struct DecodeResult {
    std::vector<Buffer> decoded;
    Result<Void> result = fail(toError(ERROR_SOURCE, toInt(ErrorCode::Timeout)));
    std::size_t consumed = 0;
};

DecodeResult decode(Buffer b) {
    DecodeResult retval;
    auto callback =
    [&] (Buffer& b) {
        retval.decoded.emplace_back(std::move(b));
        return
        Buffer::create(defaultAllocator()) >=
        [&] (Buffer&& newBuffer) {
            b = std::move(newBuffer);
            return ok();
        };
    };
    auto decoder = PacketDecodeFSM(callback, throwOnFail(Buffer::create(defaultAllocator())));
    retval.result = ok();
    for (auto byte: b) {
        retval.result = decoder.consume(byte);
        if (isFail(retval.result)) {
            break;
        }
        ++retval.consumed;
    }
    return retval;
}

void runCase(Case c) {
    auto result = decode(std::move(c.input));
    REQUIRE(result.result == c.expectedResult);
    REQUIRE(result.consumed == c.expectedInputConsumed);
    REQUIRE(result.decoded == c.expectedOutput);
}

void runCase(Buffer b) {
    auto encoded = stuffDelim(copy(b));
    auto result = decode(std::move(encoded));
    REQUIRE(result.result == ok(Void()));
    REQUIRE(result.decoded == vec({std::move(b)}));
}

TEST_CASE("01") {
    runCase(
    {
    buf({ 0 }),
    {},
    ok(),
    1
    });
}

TEST_CASE("02") {
    runCase(
    {
    buf({ 0, 1, 1, 0 }),
    vec({ buf({0}) }),
    ok(),
    4
    });
}

TEST_CASE("03") {
    Case c = {
    buf({ 0, 1, 1, 0, 0, 2, 1, 0, 3, 1, 2, 1, 2, 1, 0 }),
    vec({ buf({ 0 }), buf({ 1 }), buf({ 1, 2, 0, 0, 1 }) }),
    ok(),
    15
    };
    CAPTURE(c.input);
    runCase(std::move(c));
}

TEST_CASE("04") {
    runCase(
    {
    buf({ 5, 4, 3, 2, 1, 0, 2, 1, 0 }),
    vec({ buf({ 1 }) }),
    ok(),
    9
    });
}

TEST_CASE("05", "[fail]") {
    runCase(
    {
    buf({ 0, 1, 0, 0 }),
    {},
    fail(toError(ErrorCode::CobsBadEncoding)),
    2
    });
}

TEST_CASE("06", "[fail]") {
    runCase(
    {
    buf({ 0, 3, 1, 0, 0 }),
    {},
    fail(toError(ErrorCode::CobsBadEncoding)),
    3
    });
}

TEST_CASE("07") {
    runCase(
    {
    cat(buf({ 0 }), buf({ 255 }), nz(254), buf({ 0 })),
    vec({ nz(254) }),
    ok(),
    257
    }
    );
}

TEST_CASE("08") {
    runCase(
    {
    cat(buf({ 0, 255 }), nz(254), buf({ 255 }), nz(254), buf({ 1, 1, 0 })),
    vec({ cat(nz(254), nz(254), buf({ 0 })) }),
    ok(),
    2 + 254 + 1 + 254 + 3
    }
    );
}

TEST_CASE("09") {
    runCase(nz(254));
}

TEST_CASE("10") {
    runCase(cat(nz(254), nz(254)));
}

TEST_CASE("11") {
    runCase(nz(253));
}

TEST_CASE("12") {
    runCase(cat(nz(253), buf({ 0 })));
}

TEST_CASE("13") {
    runCase(buf({ 0 }));
}

TEST_CASE("14") {
    runCase(buf({ 0, 0 }));
}
