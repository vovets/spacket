#include "namespaces.h"
#include "utils.h"
#include <spacket/errors.h>
#include <spacket/buffer_utils.h>
#include <spacket/buffer_new_allocator.h>
#include <spacket/read_packet.h>
#include <spacket/result_utils.h>

#include <catch.hpp>

using Buffer = BufferT<NewAllocator>;
using TestSource = TestSourceT<Buffer>;
namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBase<Buffer> {}; 
}

Buffer buffer(std::initializer_list<uint8_t> l) {
    return throwOnFail(Buffer::create(l));
}

Buffer buffer(size_t size) {
    return std::move(throwOnFail(Buffer::create(size)));
}

TEST_CASE("1", "[nodev][read]") {
    TestSource ts({{1, 2, 3, 4, 5, 6}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto result = readPacket<Buffer>(source, std::move(next), 6, ch::milliseconds(1));
    REQUIRE(isFail(result));
    REQUIRE(getFailUnsafe(result) == Error::Timeout);
}

TEST_CASE("2", "[nodev][read]") {
    TestSource ts({{0, 0, 0, 0, 0, 0}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto result = readPacket<Buffer>(source, std::move(next), 6, ch::milliseconds(1));
    REQUIRE(isFail(result));
    REQUIRE(getFailUnsafe(result) == Error::Timeout);
}

TEST_CASE("3", "[nodev][read]") {
    TestSource ts({{1, 2, 3, 4, 5, 0}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto result = throwOnFail(
        readPacket<Buffer>(source, std::move(next), 6, ch::milliseconds(1)));
    REQUIRE(result.packet == (buffer({1, 2, 3, 4, 5})));
    REQUIRE(result.suffix == buffer(0));
}

TEST_CASE("4", "[nodev][read]") {
    TestSource ts({{0, 1, 2, 0}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto b = throwOnFail(
        readPacket<Buffer>(source, std::move(next), 4, ch::milliseconds(1)));
    REQUIRE(b.packet == (buffer({1, 2})));
    REQUIRE(b.suffix == buffer(0));
}

TEST_CASE("5", "[nodev][read]") {
    TestSource ts({{0, 0}, {0, 1, 2, 0}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto b = throwOnFail(
        readPacket<Buffer>(source, std::move(next), 4, ch::milliseconds(1)));
    REQUIRE(b.packet == (buffer({1, 2})));
    REQUIRE(b.suffix == buffer(0));
}

TEST_CASE("6", "[nodev][read]") {
    TestSource ts({{0, 0}, {0, 1, 2, 0}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto b = throwOnFail(
        readPacket<Buffer>(source, std::move(next), 4, ch::milliseconds(1)));
    REQUIRE(b.packet == (buffer({1, 2})));
    REQUIRE(b.suffix == buffer(0));
}

TEST_CASE("7", "[nodev][read]") {
    TestSource ts({{0, 0}, {0, 1, 2}, {0}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto b = throwOnFail(
        readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
    REQUIRE(b.packet == (buffer({1, 2})));
    REQUIRE(b.suffix == buffer(0));
}

TEST_CASE("8", "[nodev][read]") {
    TestSource ts({{1}, {1, 2, 3}, {1}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto result = readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1));
    REQUIRE(isFail(result));
    REQUIRE(getFailUnsafe(result) == Error::Timeout);
}

TEST_CASE("9", "[nodev][read]") {
    TestSource ts({{1}, {1, 0, 3}, {1}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto b = throwOnFail(
        readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
    REQUIRE(b.packet == (buffer({1, 1})));
    REQUIRE(b.suffix == buffer({3}));
}

TEST_CASE("10", "[nodev][read]") {
    TestSource ts({{1}, {1, 2, 0}, {2}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto b = throwOnFail(
        readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
    REQUIRE(b.packet == (buffer({1, 1, 2})));
    REQUIRE(b.suffix == buffer(0));
}

TEST_CASE("20", "[nodev][read]") {
    TestSource ts({{1}, {1, 2, 0}, {2, 0}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    {
        auto b = throwOnFail(
            readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
        REQUIRE(b.packet == (buffer({1, 1, 2})));
        REQUIRE(b.suffix == buffer(0));
        next = std::move(b.suffix);
    }
    {
        auto b = throwOnFail(
            readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
        REQUIRE(b.packet == (buffer({2})));
        REQUIRE(b.suffix == buffer(0));
        next = std::move(b.suffix);
    }
}

TEST_CASE("21", "[nodev][read]") {
    TestSource ts({{1}, {1, 2, 0}, {0, 2, 0}});
    Buffer next = buffer(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    {
        auto b = throwOnFail(
            readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
        REQUIRE(b.packet == (buffer({1, 1, 2})));
        REQUIRE(b.suffix == buffer(0));
        next = std::move(b.suffix);
    }
    {
        auto b = throwOnFail(
            readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
        REQUIRE(b.packet == (buffer({2})));
        REQUIRE(b.suffix == buffer(0));
        next = std::move(b.suffix);
    }
}

TEST_CASE("30", "[nodev][read]") {
    TestSource ts({{1}, {2, 3, 4}, {5, 6, 0}});
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto r = readPacket<Buffer>(source, buffer(0), 3, 5, ch::milliseconds(1));
    REQUIRE(isFail(r));
    REQUIRE(getFailUnsafe(r) == Error::PacketTooBig);
}

TEST_CASE("31", "[nodev][read]") {
    TestSource ts({{1}, {2, 3, 4}, {5, 6, 0}});
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto r = throwOnFail(
        readPacket<Buffer>(source, buffer(0), 3, 6, ch::milliseconds(1)));
    REQUIRE(r.packet == (buffer({1, 2, 3, 4, 5, 6})));
    REQUIRE(r.suffix == buffer(0));
}
