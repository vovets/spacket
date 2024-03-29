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


constexpr size_t BYTE_TIMEOUT_US = 0;
constexpr Baud BAUD = Baud::B_921600;
constexpr size_t REP = 10;

#include "buf.h"

struct TestCase {
    std::vector<Buffer> send;
    Result<Buffer> expected;

    TestCase(const TestCase& src)
        : send(copy(src.send))
        , expected(copy(src.expected))
    {
    }

    TestCase(std::vector<Buffer> send, Result<Buffer> expected)
        : send(std::move(send))
        , expected(std::move(expected))
    {
    }
};

Result<Buffer> readFull(SerialDevice& sd, Timeout timeout, std::promise<void>& launched) {
    bool finished = false;
    Result<Buffer> result = ok(buf(0));
    launched.set_value();
    while (!finished) {
        sd.read(timeout) >=
        [&](Buffer&& read) {
            WARN("read: " << hr(read));
            return
            std::move(result) >=
            [&](Buffer&& result_) {
                result = cat__(result_, read);
                return ok();
            };
        } <=
        [&](Error e) {
            finished = true;
            if (e == toError(ErrorCode::ReadTimeout)) {
                return ok();
            }
            result = fail<SuccessT<decltype(result)>>(e);
            return ok();
        };
    }
    return result;
}

void runCaseOnce(const PortConfig& pc, TestCase c) {
    SerialDevice::open(defaultAllocator(), pc) >=
    [&](SerialDevice&& sd) {
        std::promise<void> launched;
        auto future = std::async(
            std::launch::async, readFull, std::ref(sd), Timeout(std::chrono::milliseconds(100)), std::ref(launched));
        launched.get_future().wait();
        for (const auto& bufferToSend: c.send) {
            WARN("write: " << hr(bufferToSend));
            sd.write(bufferToSend) <=
            [&](Error e) {
                throw std::runtime_error(toString(e));
                return fail(e);
            } >
            [&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return ok();
            };
        }
        auto result = future.get();
        REQUIRE(result == c.expected);
        return ok();
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
            vec({
                    buf({ 1, 2, 3, 4, 5 }),
                    buf({ 0, 1, 0 })
                })
            ,
            ok(buf({ 1 }))
        },
        REP);
}

TEST_CASE("overflow") {
    Buffer expected = createTestBufferNoZero(BUFFER_MAX_SIZE);
    runCase(
        {
            vec({
                    buf({0}),
                    copy(expected),
                    buf({0}),
                }),
            ok(std::move(expected))
        },
        REP
    );
    runCase(
        {
            vec({
                    buf({0}),
                    createTestBufferNoZero(BUFFER_MAX_SIZE),
                    createTestBufferNoZero(1),
                    buf({0}),
                    // this is to bring the target to known state
                    createTestBufferNoZero(BUFFER_MAX_SIZE),
                    createTestBufferNoZero(1)
                }),
            ok(buf(0))
        },
        REP
    );
}

TEST_CASE("misc") {
    runCase(
        {
            vec({
                    buf({0, 1, 2, 3, 4}),
                    buf({0})
                }),
            ok(buf({1, 2, 3, 4}))
        },
        REP
    );

    runCase(
        {
            vec({
                    buf({0, 1, 2, 3, 4}),
                    buf({5, 6, 7, 8 }),
                    buf({9, 0})
                }),
            ok(buf({1, 2, 3, 4, 5, 6, 7, 8, 9}))
        },
        REP
    );
}
