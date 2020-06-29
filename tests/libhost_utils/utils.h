#pragma once

#include <spacket/result.h>
#include <spacket/buffer.h>
#include <spacket/buffer_utils.h>
#include <spacket/time_utils.h>
#include <spacket/port_config.h>
#include <spacket/cobs.h>
#include <spacket/error_to_string.h>
#include <spacket/allocator.h>
#include <spacket/buffer_new_allocator.h>

#include <catch.hpp>

PortConfig fromJson(const std::string& path);

inline
alloc::Allocator& defaultAllocator() {
    static NewAllocator allocator;
    return allocator;
};

class Equals: public Catch::MatcherBase<Buffer> {
public:
    Equals(const Buffer& reference): reference(reference) {}

    bool match(const Buffer& b) const override {
        return b == reference;
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "is equal to " << hr(reference);
        return ss.str();
    }

private:
    const Buffer& reference;
};

inline
Equals isEqualTo(const Buffer& reference) { return Equals(reference); }

struct StringMakerBufferBase {
    static std::string convert( Buffer const& value ) {
        std::ostringstream ss;
        ss << hr(value);
        return ss.str();
    }
};

template<typename Result>
struct StringMakerResultBase {
    static std::string convert( Result const& value ) {
        std::ostringstream ss;
        if (isOk(value)) {
            ss << hr(getOkUnsafe(value));
        } else {
            ss << toString(getFailUnsafe(value));
        }
        return ss.str();
    }
};

namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBufferBase {};

// make this specialization(s) for concrete result type available to compiler for Catch to be
// able to print buffers contents from asserions

template<> struct StringMaker<Result<Buffer>>: public StringMakerResultBase<Result<Buffer>> {};
} // Catch

inline
Buffer createTestBuffer(size_t size) {
    Buffer buffer = throwOnFail(Buffer::create(defaultAllocator(), size));
    auto c = buffer.begin();
    for (size_t i = 0; i < size; ++i, ++c) {
        *c = i % 256;
    }
    return std::move(buffer);
}

inline
Buffer createTestBufferNoZero(size_t size) {
    Buffer buffer = throwOnFail(Buffer::create(defaultAllocator(), size));
    auto c = buffer.begin();
    for (size_t i = 0; i < size; ++i, ++c) {
        *c = i % 255 + 1;
    }
    return std::move(buffer);
}

template<typename Success>
Result<Success> fatalT(Error e) {
    std::ostringstream ss;
    ss << "fatal:" << e.source << "." << e.code << " " << toString(ErrorCode(e.code));
    throw std::runtime_error(ss.str());
}

class TestSource {
public:
    TestSource(std::vector<std::vector<uint8_t>> bs)
        : buffers(init(std::move(bs)))
        , index(0) {
    }

    static std::vector<Buffer> init(std::vector<std::vector<uint8_t>> bs) {
        std::vector<Buffer> result;
        for (auto& v: bs) {
            result.push_back(throwOnFail(Buffer::create(defaultAllocator(), std::move(v))));
        }
        return std::move(result);
    }

    Result<Buffer> read(Timeout t, size_t maxRead) {
        if (index >= buffers.size()) {
            return fail<Buffer>(toError(ErrorCode::Timeout));
        }
        Buffer b = std::move(buffers[index]);
        if (b.size() > maxRead) {
            throw std::logic_error("TestSource: maxRead exceeded in read");
        }
        ++index;
        return ok(std::move(b));
    }

private:
    std::vector<Buffer> buffers;
    size_t index;
};
