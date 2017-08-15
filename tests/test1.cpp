#include "namespaces.h"
#include "utils.h"
#include "build_config.h"
#include <spacket/config.h>
#include <spacket/config_utils.h>
#include <spacket/errors.h>
#include <spacket/serial_device.h>
#include <spacket/buffer_utils.h>

#include <catch.hpp>

#include <sstream>
#include <cstring>
#include <thread>

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

inline Equals isEqualTo(const Buffer& reference) { return Equals(reference); }

namespace Catch {
	template<> struct StringMaker<Buffer> {
    	static std::string convert( Buffer const& value ) {
            std::ostringstream ss;
            ss << hr(value);
        	return ss.str(); 
        } 
    }; 
}

Buffer createTestBuffer(size_t size) {
    Buffer buffer(size);
    auto c = buffer.begin();
    for (size_t i = 0; i < size; ++i, ++c) {
        *c = i % 256;
    }
    return std::move(buffer);
}

void fatal(Error e) {
    std::ostringstream ss;
    ss << "fatal error [" << toInt(e) << "]: " << toString(e);
    throw std::runtime_error(ss.str());
}

const size_t MAX_READ = 1024;
std::vector<size_t> sizes = {100, 200, 300, 400, 500, 600, 700, 800, 1000, 2000, 4096};
const size_t REPETITIONS = 20;

TEST_CASE("open each write prefix", "[loopback]") {
    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);

    auto test = [&](size_t size, size_t rep) {
                    INFO("buffer size: " << size << ", rep: " << rep);
                    rcallv(sd, fatal, SerialDevice::open, pc);
                    auto wb = createTestBuffer(size);
                    // rcall(fatal, sd.flush);
                    // std::this_thread::sleep_for(ch::milliseconds(100));
                    rcall(fatal, sd.write, wb);
                    rcallv(rb, fatal, sd.read, ch::seconds(1), MAX_READ);
                    CAPTURE(rb);
                    REQUIRE(isPrefix(rb, wb));
                };
    
    for (auto s: sizes) {
        for (size_t n = 0; n < REPETITIONS; ++n) {
            test(s, n);
        }
    }
}

TEST_CASE("open each write full", "[loopback]") {
    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);

    auto test = [&](size_t size, size_t rep) {
                    INFO("buffer size: " << size << ", rep: " << rep);
                    rcallv(sd, fatal, SerialDevice::open, pc);
                    auto wb = createTestBuffer(size);
                    rcall(fatal, sd.write, wb);
                    Buffer rb(0);
                    while (rb.size() < wb.size()) {
                        rcallv(tmp, fatal, sd.read, ch::seconds(1), MAX_READ);
                        CAPTURE(tmp);
                        if (!rb.size()) {
                            REQUIRE(isPrefix(tmp, wb));
                        }
                        rb = rb + tmp;
                    }
                    REQUIRE_THAT(rb, isEqualTo(wb));
                };

    for (auto s: sizes) {
        for (size_t n = 0; n < REPETITIONS; ++n) {
            test(s, n);
        }
    }
}
