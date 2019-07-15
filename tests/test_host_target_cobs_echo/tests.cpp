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

#include <chrono>
#include <thread>
#include <future>

using Buffer = BufferT<NewAllocator>;
using Buffers = std::vector<Buffer>;
using SerialDevice = SerialDeviceT<Buffer>;

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
constexpr auto readPacketTimeout = c::milliseconds(50);

#include "buf.h"

struct TestCase {
    std::vector<Buffer> send;
    Result<std::vector<Buffer>> expected;
    Timeout holdOffTime;
};

Result<boost::blank> send(SerialDevice& sd, Timeout holdOffTime, Buffer b) {
    WARN("sent: " << hr(b));
    return
    sd.write(b) >
    [&]() {
        std::this_thread::sleep_for(holdOffTime);
        return ok(boost::blank{});
    } <=
    [&](Error e) {
        throw std::runtime_error(toString(e));
        return fail<boost::blank>(e);
    };
}

Result<boost::blank> send(SerialDevice& sd, Timeout holdOffTime, std::vector<Buffer> v) {
    for (auto&& b: v) {
        returnOnFailE(send(sd, holdOffTime, std::move(b)));
    }
    return ok(boost::blank{});
}

Result<std::vector<Buffer>> receive(SerialDevice& sd, std::promise<void>& launched) {
    Source<Buffer> source =
    [&](Timeout t, size_t maxRead) {
        return
        sd.read(t) >=
        [&](Buffer&& received) {
            WARN("received: " << hr(received));
            return ok(std::move(received));
        } <=
        [&](Error e) {
            if (e == toError(ErrorCode::DevReadTimeout)) {
                return fail<Buffer>(toError(ErrorCode::Timeout));
            }
            return fail<Buffer>(e);
        };
    };
    
    Result<Buffers> result = ok(Buffers{});
    bool finished = false;
    Buffer prefix = buf(0);
    launched.set_value();
    while (!finished) {
        readPacket(source, prefix, BUFFER_MAX_SIZE, readPacketTimeout) >=
        [&](ReadResult<Buffer>&& readResult) {
            prefix = std::move(readResult.suffix);
            return
            std::move(result) >=
            [&](Buffers&& r) {
                return
                cobs::unstuff(std::move(readResult.packet)) >=
                [&](Buffer&& unstuffed) {
                    r.emplace_back(std::move(unstuffed));
                    return ok(boost::blank{});
                };
            };
        } <=
        [&](Error e) {
            finished = true;
            if (e == toError(ErrorCode::Timeout)) {
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
        std::promise<void> launched;
        auto future = std::async(std::launch::async, receive, std::ref(sd), std::ref(launched));
        launched.get_future().wait();
        send(sd, c.holdOffTime, c.send);
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
        WARN("rep=" << r);
        runCaseOnce(pc, c);
    }
}

TEST_CASE("1") {
    auto b = buf({ 1, 2, 3, 4, 5 });
    runCase(
    {
        {cob(b)},
        ok(Buffers{b}),
        defaultHoldOff
    },
    REP);
}

TEST_CASE("2") {
    auto b = buf({ 1, 2, 3, 4, 0 });
    runCase(
    {
        {cob(b)},
        ok(Buffers{b}),
        defaultHoldOff
    },
    REP);
}

TEST_CASE("3") {
    auto b = buf({ 0, 2, 3, 4, 0 });
    runCase(
    {
        {cob(b)},
        ok(Buffers{b}),
        defaultHoldOff
    },
    REP);
}

TEST_CASE("4") {
    auto b = buf({ 0, 0, 0, 0, 0 });
    runCase(
    {
        {cob(b)},
        ok(Buffers{b}),
        defaultHoldOff
    },
    REP);
}

TEST_CASE("5") {
    runCase(
    {
        { buf({ 0, 5, 1, 2 }), buf({ 3, 4, 0, 0 }), buf({ 1, 1 }), buf({ 0, 0, 3, 1, 2 }), buf({ 0 }) },
        ok(Buffers{ buf({ 1, 2, 3, 4 }), buf({ 0 }), buf({ 1, 2 }) }),
        defaultHoldOff
    },
    REP);
}

TEST_CASE("10") {
    // constexpr size_t WIRE_BUFFER_SIZE = BUFFER_MAX_SIZE;
    constexpr size_t WIRE_BUFFER_SIZE = 200;
    constexpr size_t PAYLOAD_SIZE = cobs::maxPayloadSizeDelimited(WIRE_BUFFER_SIZE);
    constexpr size_t NUM_PACKETS = 5;
    std::vector<Buffer> bufs, stuffed;
    for (uint8_t fill = 0; fill < NUM_PACKETS; ++fill) {
        bufs.emplace_back(buf(PAYLOAD_SIZE, fill));
        stuffed.emplace_back(cob(bufs.back()));
        REQUIRE(stuffed.back().size() <= BUFFER_MAX_SIZE);
    }
    
    runCase(
    {
        stuffed,
        ok(bufs),
        c::milliseconds(3)
    },
    REP);
}
