#include "build_config.h"
#include "utils.h"
#include "fw/constants.h"

#include <catch.hpp>

#include <spacket/buffer_new_allocator.h>
#include <spacket/bind.h>
#include <spacket/serial_device.h>
#include <spacket/port_config.h>
#include <spacket/config_utils.h>
#include <spacket/result_utils.h>

#include <chrono>
#include <thread>
#include <future>

using Buffer = BufferT<NewAllocator>;
using SerialDevice = SerialDeviceT<Buffer>;

namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBufferBase<Buffer> {}; 
template<> struct StringMaker<Result<Buffer>>: public StringMakerResultBase<Result<Buffer>> {}; 
}


constexpr size_t BYTE_TIMEOUT_US = 0;
constexpr Baud BAUD = Baud::B_921600;
constexpr size_t REP = 10;

#include "buf.h"

struct TestCase {
    std::vector<Buffer> send;
    Result<Buffer> expected;
};

Result<Buffer> readFull(SerialDevice& sd, Timeout timeout) {
    bool finished = false;
    Result<Buffer> result = ok(buf(0));
    while (!finished) {
        sd.read(timeout) >=
        [&](Buffer&& read) {
            return
            std::move(result) >=
            [&](Buffer&& result_) {
                result = result_ + read;
                return ok(boost::blank{});
            };
        } <=
        [&](Error e) {
            finished = true;
            if (e == toError(ErrorCode::SerialDeviceReadTimeout)) {
                return ok(boost::blank{});
            }
            result = fail<SuccessT<decltype(result)>>(e);
            return ok(boost::blank{});
        };
    }
    return result;
}

void runCaseOnce(const PortConfig& pc, TestCase c) {
    SerialDevice::open(pc) >=
    [&](SerialDevice&& sd) {
        auto future = std::async(
            std::launch::async, readFull, std::ref(sd), Timeout(std::chrono::milliseconds(100)));
        for (const auto& bufferToSend: c.send) {
            sd.write(bufferToSend) <=
            [&](Error e) {
                throw std::runtime_error(toString(e));
                return fail<boost::blank>(e);
            } >
            [&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return ok(boost::blank{});
            };
        }
        auto result = future.get();
        REQUIRE(result == c.expected);
        return ok(boost::blank{});
    };
}

void runCase(TestCase c, size_t repetitions) {
    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
    pc.baud = BAUD;
    pc.byteTimeout_us = BYTE_TIMEOUT_US;

    for (size_t r = 0; r < repetitions; ++r) {
        CAPTURE(r);
        runCaseOnce(pc, c);
    }
}

TEST_CASE("sync") {
    runCase(
    {
        {
            buf({ 1, 2, 3, 4, 5 }),
            buf({ 0, 1, 0 })
        },
        ok(buf({ 1 }))
    },
    REP);
}

TEST_CASE("overflow") {
    Buffer expected = createTestBufferNoZero<Buffer>(BUFFER_MAX_SIZE);
    runCase(
    {
        {
            buf({0}),
            expected,
            buf({0}),
        },
        ok(expected)
    },
    REP);
    runCase(
    {
        {
            buf({0}),
            createTestBufferNoZero<Buffer>(BUFFER_MAX_SIZE),
            createTestBufferNoZero<Buffer>(1),
            buf({0})
        },
        ok(buf(0))
    },
    REP);
}

TEST_CASE("misc") {
    runCase(
    {
        {
            buf({0, 1, 2, 3, 4}),
            buf({0})
        },
        ok(buf({1, 2, 3, 4}))
    },
    REP);

    runCase(
    {
        {
            buf({0, 1, 2, 3, 4}),
            buf({5, 6, 7, 8 }),
            buf({9, 0})
        },
        ok(buf({1, 2, 3, 4, 5, 6, 7, 8, 9}))
    },
    REP);
}
