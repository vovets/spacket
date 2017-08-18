#include "namespaces.h"
#include "utils.h"
#include <spacket/errors.h>
#include <spacket/buffer_utils.h>
#include <spacket/packetizer.h>

#include <catch.hpp>


TEST_CASE("1", "[nodev][sync]") {
    TestSource ts({{0, 1, 2}});
    auto source = [&] (Timeout t, size_t maxSize) { return ts.read(t, maxSize); };
    rcallv(b, fatal, readSync, source, 3, ch::milliseconds(1));
    Buffer ref{1, 2};
    REQUIRE(b == ref);
}

TEST_CASE("2", "[nodev][sync]") {
    TestSource ts({{0, 0}, {0, 1, 2}});
    auto source = [&] (Timeout t, size_t maxSize) { return ts.read(t, maxSize); };
    rcallv(b, fatal, readSync, source, 3, ch::milliseconds(1));
    Buffer ref{1, 2};
    REQUIRE(b == ref);
}

TEST_CASE("3", "[nodev][sync]") {
    TestSource ts({{0, 0}, {1, 2, 3}});
    auto source = [&] (Timeout t, size_t maxSize) { return ts.read(t, maxSize); };
    rcallv(b, fatal, readSync, source, 3, ch::milliseconds(1));
    Buffer ref{1, 2, 3};
    REQUIRE(b == ref);
}

TEST_CASE("4", "[nodev][sync]") {
    TestSource ts({{0}, {0}, {0}});
    auto source = [&] (Timeout t, size_t maxSize) { return ts.read(t, maxSize); };
    auto result = readSync(source, 3, ch::milliseconds(1));
    REQUIRE(isFail(result));
    REQUIRE(getFailUnsafe(result) == Error::Timeout);
}

TEST_CASE("5", "[nodev][sync]") {
    TestSource ts({{1}, {1, 2, 3}, {1}});
    auto source = [&] (Timeout t, size_t maxSize) { return ts.read(t, maxSize); };
    auto result = readSync(source, 3, ch::milliseconds(1));
    REQUIRE(isFail(result));
    REQUIRE(getFailUnsafe(result) == Error::Timeout);
}

TEST_CASE("6", "[nodev][sync]") {
    TestSource ts({{1}, {1, 0, 3}, {1}});
    auto source = [&] (Timeout t, size_t maxSize) { return ts.read(t, maxSize); };
    rcallv(b, fatal, readSync, source, 3, ch::milliseconds(1));
    Buffer ref{3};
    REQUIRE(b == ref);
}

TEST_CASE("7", "[nodev][sync]") {
    TestSource ts({{1}, {1, 2, 0}, {2}});
    auto source = [&] (Timeout t, size_t maxSize) { return ts.read(t, maxSize); };
    rcallv(b, fatal, readSync, source, 3, ch::milliseconds(1));
    Buffer ref{2};
    REQUIRE(b == ref);
}
