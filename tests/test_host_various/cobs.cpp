#include "utils.h"

#include <spacket/cobs.h>
#include <spacket/buffer.h>

#include <catch.hpp>

using Buffer = BufferT<StdAllocator, 2048>;
auto fatal = fatalT<Buffer>;
constexpr size_t maxUnstuffed = cobs::maxUnstuffedSizeT<Buffer>();

namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBase<Buffer> {}; 
}

Buffer fill(uint8_t byte, size_t size) {
    Buffer result(size);
    std::memset(result.begin(), byte, size);
    return std::move(result);
}

Buffer fill1(uint8_t byte, size_t chunkSize, size_t size) {
    Buffer result(size);
    std::memset(result.begin(), byte, size);
    for (size_t n = chunkSize; n <= size; n += chunkSize) {
        result.begin()[n - 1] = 0;
    }
    return std::move(result);
}

void test(Buffer b) {
    CAPTURE(b);
    Buffer stuffed = cobs::stuff(b.copy());
    CAPTURE(stuffed);
    for (auto byte: stuffed) {
        REQUIRE(byte != 0);
    }
    nrcallv(unstuffed, fatal, cobs::unstuff(std::move(stuffed)));
    REQUIRE(unstuffed == b);
}

TEST_CASE("1") {
    for (size_t size = 1; size <= maxUnstuffed; ++size) {
        test(fill(0, size));
    }
}

TEST_CASE("2") {
    for (size_t size = 2; size <= maxUnstuffed; ++size) {
        test(fill(42, size));
    }
}

TEST_CASE("3") {
    constexpr size_t size = 4;
    for (size_t chunkSize = 2; chunkSize <= size; ++chunkSize) {
        CAPTURE(chunkSize);
        test(fill1(42, chunkSize, size));
    }
}