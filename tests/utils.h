#pragma once

#include <spacket/result.h>
#include <spacket/buffer.h>
#include <spacket/buffer_utils.h>
#include <spacket/time_utils.h>
#include <spacket/config.h>

#include <catch.hpp>

PortConfig fromJson(const std::string& path);

class Equals: public Catch::MatcherBase<Buffer> {
public:
    Equals(const Buffer& reference): reference(reference) {}

    bool match(const Buffer& b) const override {
        if (b.size() != reference.size()) { return false; }
        return std::memcmp(b.begin(), reference.begin(), b.size()) == 0;
    }

    std::string describe() const {
        std::ostringstream ss;
        ss << "is equal to " << hr(reference);
        return ss.str();
    }

private:
    const Buffer& reference;
};

inline
Equals isEqualTo(const Buffer& reference) { return Equals(reference); }

namespace Catch {
	template<> struct StringMaker<Buffer> {
    	static std::string convert( Buffer const& value ) {
            std::ostringstream ss;
            ss << hr(value);
        	return ss.str(); 
        } 
    }; 
}

inline
Buffer createTestBuffer(size_t size) {
    Buffer buffer(size);
    auto c = buffer.begin();
    for (size_t i = 0; i < size; ++i, ++c) {
        *c = i % 256;
    }
    return std::move(buffer);
}

inline
void fatal(Error e) {
    std::ostringstream ss;
    ss << "fatal error [" << toInt(e) << "]: " << toString(e);
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
            result.push_back(Buffer(std::move(v)));
        }
        return std::move(result);
    }

    Result<Buffer> read(Timeout t, size_t maxSize) {
        if (index >= buffers.size()) {
            return fail<Buffer>(Error::Timeout);
        }
        Buffer b = std::move(buffers[index]);
        if (b.size() > maxSize) {
            throw std::logic_error("test buffer too big");
        }
        ++index;
        return std::move(b);
    }

private:
    std::vector<Buffer> buffers;
    size_t index;
};
