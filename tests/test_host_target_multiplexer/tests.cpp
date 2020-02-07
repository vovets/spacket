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
#include <spacket/multiplexer.h>
#include <spacket/crc.h>

#include <chrono>
#include <thread>
#include <future>


using Buffer = BufferT<NewAllocator>;
using Buffers = std::vector<Buffer>;
using SerialDevice = SerialDeviceT<Buffer>;
using PacketDevice = PacketDeviceT<Buffer, SerialDevice>;
using Multiplexer = MultiplexerT<Buffer, PacketDevice, MULTIPLEXER_NUM_CHANNELS>;
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

struct TestCase {
    Buffer send;
    Result<Buffer> expected;
    Timeout holdOffTime;

    TestCase(const TestCase& src)
        : send(copy(src.send))
        , expected(copy(src.expected))
        , holdOffTime(src.holdOffTime)
    {
    }

    TestCase(Buffer send, Result<Buffer> expected, Timeout holdOffTime)
        : send(std::move(send))
        , expected(std::move(expected))
        , holdOffTime(holdOffTime)
    {
    }
};


std::uint8_t toChannel(std::uint8_t rep) { return rep % MULTIPLEXER_NUM_CHANNELS; }

void runCase(const PortConfig& pc, TestCase c) {
    SerialDevice::open(pc) >=
    [&](SerialDevice sd) {
        return
        Buffer::create(Buffer::maxSize()) >=
        [&](Buffer&& pdBuffer) {
            PacketDevice pd(sd, std::move(pdBuffer));
            Multiplexer mx(pd);
            for (std::size_t rep = 0; rep < REP; ++rep) {
                std::promise<void> launched;
                auto receive = [&] (std::promise<void> launched) {
                    launched.set_value();
                    return
                        mx.read(toChannel(rep), readPacketTimeout);
                };
                auto launchedFuture = launched.get_future();
                auto future = std::async(std::launch::async, receive, std::move(launched));
                launchedFuture.wait();
                throwOnFail(mx.write(toChannel(rep), copy(c.send)));
                auto r = future.get();
                REQUIRE(r == c.expected);
                std::this_thread::sleep_for(c.holdOffTime);
            }
            return ok();
        };
    } <=
    [&] (Error e) {
        FAIL("" << toString(e));
        return fail(e);
    };
}

void runCaseWithPortConfig(TestCase c) {
    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
    pc.baud = BAUD;
    pc.byteTimeout_us = BYTE_TIMEOUT_US;
    runCase(pc, std::move(c));
}


TEST_CASE("22.1") {
    for (std::size_t s = 1; s < 200; ++s) {
        auto b = wz(s);
        CAPTURE(s);
        runCaseWithPortConfig(
            {
                copy(b),
                ok(copy(b)),
                c::milliseconds(1)
            }
        );
    }
}

TEST_CASE("22.2") {
    for (std::size_t s = 200; s < 256; s += 2) {
        auto b = wz(s);
        CAPTURE(s);
        runCaseWithPortConfig(
            {
                copy(b),
                ok(copy(b)),
                c::milliseconds(1)
            }
        );
    }
}

TEST_CASE("22.3") {
    for (std::size_t s = 256; s < 515; s += 3) {
        auto b = wz(s);
        CAPTURE(s);
        runCaseWithPortConfig(
            {
                copy(b),
                ok(copy(b)),
                c::milliseconds(1)
            }
        );
    }
}
