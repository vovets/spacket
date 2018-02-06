#include "utils.h"

#include <spacket/cobs.h>
#include <spacket/buffer.h>
#include <spacket/buffer_new_allocator.h>
#include <spacket/result_utils.h>

#include <catch.hpp>

using Buffer = BufferT<NewAllocator>;
constexpr size_t maxUnstuffed = cobs::maxUnstuffedSize(2048);

namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBase<Buffer> {}; 
}

Buffer fill(uint8_t byte, size_t size) {
    Buffer result = throwOnFail(Buffer::create(size));
    std::memset(result.begin(), byte, size);
    return std::move(result);
}

Buffer fill1(uint8_t byte, size_t chunkSize, size_t size) {
    Buffer result = throwOnFail(Buffer::create(size));
    std::memset(result.begin(), byte, size);
    for (size_t n = chunkSize; n <= size; n += chunkSize) {
        result.begin()[n - 1] = 0;
    }
    return std::move(result);
}

void test(Buffer b) {
    CAPTURE(b);
    Buffer stuffed = throwOnFail(cobs::stuff(throwOnFail(b.copy())));
    CAPTURE(stuffed);
    for (auto byte: stuffed) {
        REQUIRE(byte != 0);
    }
    auto unstuffed = throwOnFail(cobs::unstuff(std::move(stuffed)));
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
