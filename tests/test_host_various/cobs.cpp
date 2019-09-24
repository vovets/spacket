#include "utils.h"

#include <spacket/cobs.h>
#include <spacket/buffer.h>
#include <spacket/buffer_new_allocator.h>
#include <spacket/result_utils.h>

#include <catch.hpp>

using Buffer = BufferT<NewAllocator>;
constexpr size_t maxPayload = cobs::maxPayloadSize(2048);

#include "buf.h"

namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBufferBase<Buffer> {}; 
}

Buffer fill(uint8_t byte, size_t size) {
    Buffer result = throwOnFail(Buffer::create(size));
    std::memset(result.begin(), byte, size);
    return std::move(result);
}

Buffer fill1(size_t chunkSize, size_t size) {
    Buffer result = buf(size);
    for (std::size_t n = 0; n < size; ++n) {
        std::uint8_t byte = (n + 1) % chunkSize;
        result.begin()[n] = byte;
    }
    return std::move(result);
}

void test(Buffer b) {
    Buffer stuffed = stuff(copy(b));
    CAPTURE(stuffed);
    for (auto byte: stuffed) {
        REQUIRE(byte != 0);
    }
    auto unstuffed = throwOnFail(cobs::unstuff(std::move(stuffed)));
    REQUIRE(unstuffed == b);
}

TEST_CASE("1") {
    for (size_t size = 1; size <= maxPayload; ++size) {
        test(fill(0, size));
    }
}

TEST_CASE("2") {
    for (size_t size = 2; size <= maxPayload; ++size) {
        test(fill(42, size));
    }
}

TEST_CASE("3") {
    constexpr size_t size = 513;
    for (size_t chunkSize = 1; chunkSize <= size; ++chunkSize) {
        CAPTURE(chunkSize);
        test(fill1(chunkSize, size));
    }
}

TEST_CASE("4") {
    REQUIRE(stuff(buf({ 0 })) == buf({ 1, 1 }));
}

TEST_CASE("5") {
    REQUIRE(cobs::unstuff(buf({ 1 })) == fail<Buffer>(toError(ErrorCode::CobsBadEncoding)));
}

TEST_CASE("6") {
    test(fill1(255, 254));
}
