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

const size_t MAX_READ = 1024;
std::vector<size_t> sizes = {100, 200, 300, 400, 500, 600, 700, 800, 1000, 2000, 4096};
std::vector<Baud> bauds = {Baud::B_115200, Baud::B_230400};
const size_t REPETITIONS = 20;
const size_t BYTE_TIMEOUT_US = 0;

template<typename Test>
void runTest(Test test) {
    for (auto b: bauds) {
        for (auto s: sizes) {
            for (size_t n = 0; n < REPETITIONS; ++n) {
                INFO("buffer size: " << s << ", baud: " << toInt(b) << ", rep: " << n);
                test(s, b, n);
            }
        }
    }
}

TEST_CASE("read prefix", "[loopback]") {
    auto test = [&](size_t size, Baud b, size_t rep) {
                    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
                    pc.baud = b;
                    pc.byteTimeout_us = BYTE_TIMEOUT_US;
    
                    rcallv(sd, fatal, SerialDevice::open, pc);
                    auto wb = createTestBuffer(size);
                    rcall(fatal, sd.write, wb);
                    rcallv(rb, fatal, sd.read, ch::seconds(1), MAX_READ);
                    CAPTURE(rb);
                    REQUIRE(isPrefix(rb, wb));
                };
    runTest(test);
}

TEST_CASE("read whole", "[loopback]") {
    auto test = [&](size_t size, Baud b, size_t rep) {
                    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
                    pc.baud = b;
                    pc.byteTimeout_us = BYTE_TIMEOUT_US;

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

    runTest(test);
}

TEST_CASE("timeout", "[loopback]") {
    auto test = [&](size_t size, Baud b, size_t rep) {
                    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
                    pc.baud = b;
                    pc.byteTimeout_us = BYTE_TIMEOUT_US;

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
                    auto tmp = sd.read(ch::milliseconds(10), MAX_READ);
                    REQUIRE(isFail(tmp));
                    REQUIRE(getFailUnsafe(tmp) == Error::Timeout);
                };

    runTest(test);
}
