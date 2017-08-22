#include "namespaces.h"
#include "utils.h"
#include <spacket/errors.h>
#include <spacket/buffer_utils.h>
#include <spacket/packetizer.h>

#include <catch.hpp>

using Buffer = BufferT<StdAllocator, 1024>;
using TestSource = TestSourceT<Buffer>;
auto fatal = fatalT<Buffer>;

TEST_CASE("1", "[nodev][read]") {
    TestSource ts({{1, 2, 3, 4, 5, 6}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto result = readPacket<Buffer>(source, std::move(next), 6, ch::milliseconds(1));
    REQUIRE(isFail(result));
    REQUIRE(getFailUnsafe(result) == Error::Timeout);
}

TEST_CASE("2", "[nodev][read]") {
    TestSource ts({{0, 0, 0, 0, 0, 0}});
    Buffer next(0);
    const size_t MAX_READ = 6;
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto result = readPacket<Buffer>(source, std::move(next), 6, ch::milliseconds(1));
    REQUIRE(isFail(result));
    REQUIRE(getFailUnsafe(result) == Error::Timeout);
}

TEST_CASE("3", "[nodev][read]") {
    TestSource ts({{1, 2, 3, 4, 5, 0}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    nrcallv(result, fatal, readPacket<Buffer>(source, std::move(next), 6, ch::milliseconds(1)));
    REQUIRE(result.packet == (Buffer{1, 2, 3, 4, 5}));
    REQUIRE(result.suffix == Buffer(0));
}

TEST_CASE("4", "[nodev][read]") {
    TestSource ts({{0, 1, 2, 0}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 4, ch::milliseconds(1)));
    REQUIRE(b.packet == (Buffer{1, 2}));
    REQUIRE(b.suffix == Buffer(0));
}

TEST_CASE("5", "[nodev][read]") {
    TestSource ts({{0, 0}, {0, 1, 2, 0}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 4, ch::milliseconds(1)));
    REQUIRE(b.packet == (Buffer{1, 2}));
    REQUIRE(b.suffix == Buffer(0));
}

TEST_CASE("6", "[nodev][read]") {
    TestSource ts({{0, 0}, {0, 1, 2, 0}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 4, ch::milliseconds(1)));
    REQUIRE(b.packet == (Buffer{1, 2}));
    REQUIRE(b.suffix == Buffer(0));
}

TEST_CASE("7", "[nodev][read]") {
    TestSource ts({{0, 0}, {0, 1, 2}, {0}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
    REQUIRE(b.packet == (Buffer{1, 2}));
    REQUIRE(b.suffix == Buffer(0));
}

TEST_CASE("8", "[nodev][read]") {
    TestSource ts({{1}, {1, 2, 3}, {1}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto result = readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1));
    REQUIRE(isFail(result));
    REQUIRE(getFailUnsafe(result) == Error::Timeout);
}

TEST_CASE("9", "[nodev][read]") {
    TestSource ts({{1}, {1, 0, 3}, {1}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
    REQUIRE(b.packet == (Buffer{1, 1}));
    REQUIRE(b.suffix == Buffer{3});
}

TEST_CASE("10", "[nodev][read]") {
    TestSource ts({{1}, {1, 2, 0}, {2}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
    REQUIRE(b.packet == (Buffer{1, 1, 2}));
    REQUIRE(b.suffix == Buffer(0));
}

TEST_CASE("20", "[nodev][read]") {
    TestSource ts({{1}, {1, 2, 0}, {2, 0}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    {
        nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
        REQUIRE(b.packet == (Buffer{1, 1, 2}));
        REQUIRE(b.suffix == Buffer(0));
        next = std::move(b.suffix);
    }
    {
        nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
        REQUIRE(b.packet == (Buffer{2}));
        REQUIRE(b.suffix == Buffer(0));
        next = std::move(b.suffix);
    }
}

TEST_CASE("21", "[nodev][read]") {
    TestSource ts({{1}, {1, 2, 0}, {0, 2, 0}});
    Buffer next(0);
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    {
        nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
        REQUIRE(b.packet == (Buffer{1, 1, 2}));
        REQUIRE(b.suffix == Buffer(0));
        next = std::move(b.suffix);
    }
    {
        nrcallv(b, fatal, readPacket<Buffer>(source, std::move(next), 3, ch::milliseconds(1)));
        REQUIRE(b.packet == (Buffer{2}));
        REQUIRE(b.suffix == Buffer(0));
        next = std::move(b.suffix);
    }
}

TEST_CASE("30", "[nodev][read]") {
    TestSource ts({{1}, {2, 3, 4}, {5, 6, 0}});
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    auto r = readPacket<Buffer>(source, Buffer(0), 3, 5, ch::milliseconds(1));
    REQUIRE(isFail(r));
    REQUIRE(getFailUnsafe(r) == Error::PacketTooBig);
}

TEST_CASE("31", "[nodev][read]") {
    TestSource ts({{1}, {2, 3, 4}, {5, 6, 0}});
    auto source = [&] (Timeout t, size_t maxRead) { return ts.read(t, maxRead); };
    nrcallv(r, fatal, readPacket<Buffer>(source, Buffer(0), 3, 6, ch::milliseconds(1)));
    REQUIRE(r.packet == (Buffer{1, 2, 3, 4, 5, 6}));
    REQUIRE(r.suffix == Buffer(0));
}
