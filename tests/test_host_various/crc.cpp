#include "namespaces.h"
#include "utils.h"

#include <spacket/crc.h>
#include <spacket/buffer.h>
#include <spacket/buffer_debug.h>
#include <spacket/buffer_new_allocator.h>

#include <catch.hpp>

using Buffer = BufferT<NewAllocator>;

#include "buf.h"

namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBufferBase<Buffer> {};
template<> struct StringMaker<Result<Void>>: public StringMakerBufferBase<Result<Void>> {};
}

#ifdef ENABLE_DEBUG_PRINT

IMPLEMENT_DPB_FUNCTION

#else

IMPLEMENT_DPB_FUNCTION_NOP

#endif

Buffer toBufBE(std::uint32_t c) {
    Buffer retval = throwOnFail(Buffer::create(4));
    for (std::size_t i = 0; i < 4; ++i) {
        retval.begin()[3 - i] = c & 0x000000ff;
        c >>= 8;
    }
    return retval;
}

TEST_CASE("01") {
    auto b = buf("123456789");
    REQUIRE(crc::crc(b.begin(), b.size()) == 0x0376e6e7);
}

TEST_CASE("02") {
    auto c = toBufBE(0x0376e6e7);
    DPB(c);
    auto b = cat(buf("123456789"), c);
    REQUIRE(crc::crc(b.begin(), b.size()) == 0);
}

TEST_CASE("03") {
    auto a = buf("123456789");
    auto c = toBufBE(0x0376e6e7);
    auto b = cat(a, c);
    auto r = crc::check(std::move(b));
    REQUIRE(r == ok(std::move(a)));
}

TEST_CASE("04") {
    auto a = buf("123456789");
    auto b = throwOnFail(crc::append(copy(a)));
    auto r = crc::check(std::move(b));
    REQUIRE(r == ok(std::move(a)));
}

TEST_CASE("05") {
    auto a = buf({ 0 });
    auto b = throwOnFail(crc::append(copy(a)));
    DPB(b);
    auto r = crc::check(std::move(b));
    REQUIRE(r == ok(std::move(a)));
}
