#include "build_config.h"
#include "utils.h"
#include "fw/constants.h"

#include <catch.hpp>

#include <spacket/buffer_new_allocator.h>
#include <spacket/bind.h>
#include <spacket/serial_device.h>
#include <spacket/serial_utils.h>
#include <spacket/port_config.h>
#include <spacket/config_utils.h>
#include <spacket/result_utils.h>
#include <spacket/cobs.h>
#include <spacket/read_packet.h>
#include <spacket/packet_device.h>
#include <spacket/crc.h>

#include <chrono>
#include <thread>
#include <future>


using Buffer = BufferT<NewAllocator>;
using Buffers = std::vector<Buffer>;
using SerialDevice = SerialDeviceT<Buffer>;
using PacketDevice = PacketDeviceT<Buffer, SerialDevice>;
namespace c = std::chrono;

namespace Catch {
template<> struct StringMaker<Buffer>: public StringMakerBufferBase<Buffer> {}; 
template<> struct StringMaker<Result<Buffer>>: public StringMakerResultBase<Result<Buffer>> {};
template<> struct StringMaker<Result<Buffers>>: public StringMakerResultBase<Result<Buffers>> {};
}


constexpr size_t BYTE_TIMEOUT_US = 0;
constexpr Baud BAUD = Baud::B_921600;
constexpr size_t REP = 10;
constexpr auto defaultHoldOff = c::milliseconds(2);
constexpr auto readPacketTimeout = c::milliseconds(100);

#include "buf.h"

struct TestCase2 {
    std::vector<Buffer> send;
    Result<Buffer> expected;
    Timeout holdOffTime;

    TestCase2(const TestCase2& src)
        : send(copy(src.send))
        , expected(copy(src.expected))
        , holdOffTime(src.holdOffTime)
    {
    }

    TestCase2(std::vector<Buffer> send, Result<Buffer> expected, Timeout holdOffTime)
        : send(std::move(send))
        , expected(std::move(expected))
        , holdOffTime(holdOffTime)
    {
    }
};

void runCaseOnce(const PortConfig& pc, TestCase2 c) {
    SerialDevice::open(pc) >=
    [&](SerialDevice sd) {
        return
        PacketDevice::open(std::move(sd)) >=
        [&](PacketDevice pd) {
            std::promise<void> launched;
            auto receive = [&] (std::promise<void> launched) {
                launched.set_value();
                return
                pd.read(readPacketTimeout) >=
                [&](Buffer received) {
                    return
                    crc::check(std::move(received));
                };
            };
            auto launchedFuture = launched.get_future();
            auto future = std::async(std::launch::async, receive, std::move(launched));
            launchedFuture.wait();
            if (!c.send.empty()) {
                throwOnFail(
                    crc::append(copy(c.send[0])) >=
                    [&] (Buffer appended) {
                        return pd.write(std::move(appended));
                    });
                std::this_thread::sleep_for(c.holdOffTime);
            }
            auto r = future.get();
            REQUIRE(r == c.expected);
            return ok();
        };
    } <=
    [&] (Error e) {
        FAIL("" << toString(e));
        return fail(e);
    };
}

template <typename TestCase>
void runCaseT(TestCase c, size_t repetitions) {
    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
    pc.baud = BAUD;
    pc.byteTimeout_us = BYTE_TIMEOUT_US;

    for (size_t r = 0; r < repetitions; ++r) {
        CAPTURE(r);
        runCaseOnce(pc, c);
    }
}

void runCase2(TestCase2 c, size_t repetitions) {
    runCaseT(c, repetitions);
}

TEST_CASE("20") {
    runCase2(
        {
            {},
            fail<Buffer>(toError(ErrorCode::ReadTimeout)),
            defaultHoldOff
        },
        REP
    );
}

TEST_CASE("22.0") {
    std::size_t s = 21;
    auto b = wz(s);
    CAPTURE(s);
    runCase2(
        {
            vec(copy(b)),
            ok(copy(b)),
            defaultHoldOff
        },
        1
    );
}

TEST_CASE("22.1") {
    for (std::size_t s = 1; s < 200; ++s) {
        auto b = wz(s);
        CAPTURE(s);
        runCase2(
            {
                vec(copy(b)),
                ok(copy(b)),
                c::milliseconds(3)
            },
            1
        );
    }
}

TEST_CASE("22.2") {
    for (std::size_t s = 200; s < 256; s += 2) {
        auto b = wz(s);
        CAPTURE(s);
        runCase2(
            {
                vec(copy(b)),
                ok(copy(b)),
                c::milliseconds(4)
            },
            REP
        );
    }
}

TEST_CASE("22.3") {
    for (std::size_t s = 256; s < 515; s += 3) {
        auto b = wz(s);
        CAPTURE(s);
        runCase2(
            {
                vec(copy(b)),
                ok(copy(b)),
                c::milliseconds(9)
            },
            REP
        );
    }
}
