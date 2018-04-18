#include "namespaces.h"
#include "utils.h"
#include "build_config.h"
#include <spacket/port_config.h>
#include <spacket/config_utils.h>
#include <spacket/errors.h>
#include <spacket/serial_device.h>
#include <spacket/buffer_utils.h>
#include <spacket/buffer_new_allocator.h>
#include <spacket/bind.h>
#include <spacket/result_utils.h>

#include <catch.hpp>

#include <sstream>
#include <cstring>

const size_t MAX_READ = 1024;
std::vector<size_t> sizes = {100, 200, 300, 400, 500, 600, 700, 800, 1000, 2000, 4096};
// std::vector<Baud> bauds = {Baud::B_115200};
// std::vector<Baud> bauds = {Baud::B_230400};
// std::vector<Baud> bauds = {Baud::B_460800};
std::vector<Baud> bauds = {Baud::B_921600};
const size_t REPETITIONS = 10;
const size_t BYTE_TIMEOUT_US = 0;

using Buffer = BufferT<NewAllocator>;

Buffer buffer(size_t size) { return std::move(throwOnFail(Buffer::create(size))); }

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
    auto test =
    [&](size_t size, Baud b, size_t rep) {
        PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
        pc.baud = b;
        pc.byteTimeout_us = BYTE_TIMEOUT_US;

        auto result =
        SerialDevice::open(pc) >=
        [&](SerialDevice&& sd) {
            auto wb = createTestBuffer<Buffer>(size);
            return
            sd.write(wb) >=
            [&](boost::blank&&) {
                return
                sd.read(buffer(MAX_READ), ch::seconds(1)) >=
                [&](Buffer&& rb) {
                    CAPTURE(rb);
                    REQUIRE(isPrefix(rb, wb));
                    return ok(boost::blank{});
                };
            };
        };
        throwOnFail(result);
    };
    runTest(test);
}

TEST_CASE("read whole", "[loopback]") {
    auto test =
    [&](size_t size, Baud b, size_t rep) {
        PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
        pc.baud = b;
        pc.byteTimeout_us = BYTE_TIMEOUT_US;

        auto result =
        SerialDevice::open(pc) >=
        [&](SerialDevice&& sd) {
            auto wb = createTestBuffer<Buffer>(size);
            return
            sd.write(wb) >=
            [&](boost::blank&&) {
                Buffer rb = buffer(0);
                while (rb.size() < wb.size()) {
                    returnOnFailT(
                        tmp,
                        boost::blank,
                        sd.read(buffer(MAX_READ), ch::seconds(1)));
                    CAPTURE(tmp);
                    if (!rb.size()) {
                        REQUIRE(isPrefix(tmp, wb));
                    }
                    returnOnFailT(sum, boost::blank, rb + tmp);
                    rb = std::move(sum);
                }
                REQUIRE_THAT(rb, isEqualTo(wb));
                return ok(boost::blank{});
            };
        };
        throwOnFail(result);
    };

    runTest(test);
}

TEST_CASE("timeout", "[loopback]") {
    auto test =
    [&](size_t size, Baud b, size_t rep) {
        PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
        pc.baud = b;
        pc.byteTimeout_us = BYTE_TIMEOUT_US;

        auto result =
        SerialDevice::open(pc) >=
        [&](SerialDevice&& sd) {
            auto wb = createTestBuffer<Buffer>(size);
            return
            sd.write(wb) >=
            [&](boost::blank&&) {
                Buffer rb = buffer(0);
                while (rb.size() < wb.size()) {
                    returnOnFailT(
                        tmp,
                        boost::blank,
                        sd.read(buffer(MAX_READ), ch::seconds(1)));
                    CAPTURE(tmp);
                    if (!rb.size()) {
                        REQUIRE(isPrefix(tmp, wb));
                    }
                    returnOnFailT(sum, boost::blank, rb + tmp);
                    rb = std::move(sum);
                }
                REQUIRE_THAT(rb, isEqualTo(wb));
                auto r = sd.read(buffer(MAX_READ), ch::milliseconds(10));
                REQUIRE(isFail(r));
                REQUIRE(getFailUnsafe(r) == Error::DevReadTimeout);
                return ok(boost::blank{});
            };
        };
        throwOnFail(result);
    };

    runTest(test);
}
