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
        *c = i % 10;
    }
    return std::move(buffer);
}

TEST_CASE("loopback 1") {
    auto fatal = [](Error e) {
                     std::ostringstream ss;
                     ss << "fatal error [" << toInt(e) << "]: " << toString(e);
                     throw std::runtime_error(ss.str());
                 };

    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);

    rcallv(sd, fatal, SerialDevice::open, std::move(pc));

    const size_t MAX_READ = 65536;

    auto test = [&](size_t size) {
                    INFO("buffer size: " << size);
                    auto wb = createTestBuffer(size);
                    rcall(fatal, sd.write, wb);
                    Buffer rb(0);
                    while (rb.size() < wb.size()) {
                        rcallv(tmp, fatal, sd.read, ch::milliseconds(100), MAX_READ);
                        rb = rb + tmp;
                    }
                    CHECK_THAT(rb, isEqualTo(wb));
                };

    std::vector<size_t> sizes = {1, 2, 3, 4, 5, 6, 16, 17, 32, 33, 256, 257, 4096};
    const size_t REPETITIONS = 10;
    for (auto s: sizes) {
        for (size_t n = 0; n < REPETITIONS; ++n) {
            test(s);
        }
    }
}
