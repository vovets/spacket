#include "build_config.h"
#include "utils.h"
#include "fw/constants.h"
#include "buf.h"

#include <catch.hpp>

#include <spacket/buffer_new_allocator.h>
#include <spacket/bind.h>
#include <spacket/serial_device.h>
#include <spacket/port_config.h>
#include <spacket/config_utils.h>
#include <spacket/result_utils.h>

#include <chrono>


constexpr size_t BYTE_TIMEOUT_US = 0;
constexpr Baud BAUD = Baud::B_921600;

Buffer buffer(size_t size) { return std::move(throwOnFail(Buffer::create(defaultAllocator(), size))); }

TEST_CASE("packet loopback") {
    PortConfig pc = fromJson(DEVICE_CONFIG_PATH);
    pc.baud = BAUD;
    pc.byteTimeout_us = BYTE_TIMEOUT_US;

    auto increment =
    [](size_t size) { return size / 1000 + 1; };

    SerialDevice::open(defaultAllocator(), pc) >=
    [&](SerialDevice&& sd) {
        for (size_t size = 1, counter = 1;
             size < BUFFER_MAX_SIZE;
             size += increment(size), ++counter) {
            auto wb = createTestBuffer(size);
            auto rb = buffer(0);

            auto readAppend =
            [&]() {
                return
                sd.read(std::chrono::seconds(1)) >=
                [&](Buffer&& b) {
                    returnOnFailT(newBuf, bool, cat__(rb, b));
                    rb = std::move(newBuf);
                    if (rb.size() >= wb.size()) {
                        return ok(true);
                    }
                    return ok(false);
                };
            };
            
            sd.write(wb) >
            [&]() {
                bool finished = false;
                // this loop is needed because host serial port driver sometimes
                // inserts blank periods on the line inside transmitted packet in which case fw echoes
                // only prefix of the packet written so we read until total read bytes equal written
                // bytes
                do {
                    returnOnFailT(finished_, Void, readAppend());
                    finished = finished_;
                } while (!finished);
                return ok();
            } >
            [&]() {
                if (counter % 100 == 0) {
                    WARN("size: " << size << ", counter: " << counter);
                }
                REQUIRE(rb == wb);
                return ok();
            } <=
            [&](Error e) {
                throw std::runtime_error(toString(e));
                return ok();
            };
        }
        return ok();
    } <=
    [&](Error e) {
        throw std::runtime_error(toString(e));
        return ok();
    };
}
