#include "utils.h"

#include <spacket/cobs.h>
#include <spacket/buffer.h>

#include <catch.hpp>

using Buffer = BufferT<StdAllocator, 1024>;
auto fatal = fatalT<Buffer>;

namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBase<Buffer> {}; 
}

Buffer fill(uint8_t byte, size_t size) {
    Buffer result(size);
    std::memset(result.begin(), byte, size);
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

TEST_CASE("1.1") {
    test(Buffer{0});
}

TEST_CASE("1.2") {
    test(Buffer{0, 0});
}

TEST_CASE("1.3") {
    for (size_t size = 3; size < cobs::maxUnstuffedSizeT<Buffer>(); ++size) {
        test(fill(0, size));
    }
}
