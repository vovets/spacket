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


Buffer buf(size_t size) { return std::move(throwOnFail(Buffer::create(size))); }
Buffer buf(size_t size, uint8_t fill) {
    Buffer b = throwOnFail(Buffer::create(size));
    for (auto& c: b) {
        c = fill;
    }
    return b;
}

Buffer buf(std::initializer_list<uint8_t> l) {
    return throwOnFail(Buffer::create(l));
}

Buffer cob(Buffer b) {
    return throwOnFail(cobs::stuffAndDelim(std::move(b)));
}

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
        sd.flush();
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

Result<std::vector<Buffer>> receive(SerialDevice& sd) {
    Source<Buffer> source =
    [&](Timeout t, size_t maxRead) {
        return
        sd.read(buf(maxRead), t) >=
        [&](Buffer&& received) {
            WARN("received: " << hr(received));
            return ok(std::move(received));
        } <=
        [&](Error e) {
            if (e == Error::DevReadTimeout) {
                return fail<Buffer>(Error::Timeout);
            }
            return fail<Buffer>(e);
        };
    };
    
    Result<Buffers> result = ok(Buffers{});
    bool finished = false;
    Buffer prefix = buf(0);
    while (!finished) {
        readPacket(source, prefix, BUFFER_MAX_SIZE, std::chrono::milliseconds(100)) >=
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
            if (e == Error::Timeout) {
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
        auto future = std::async(std::launch::async, receive, std::ref(sd));
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
    std::vector<Buffer> bufs, stuffed;
    for (uint8_t fill = 0; fill < 5; ++fill) {
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
