#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"
#include "constants.h"

#include <spacket/serial_device.h>
#include <spacket/mailbox.h>
#include <spacket/thread.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/result_fatal.h>
#include <spacket/cobs.h>
#include <spacket/buffer_debug.h>
#include <spacket/packetizer.h>
#include <spacket/packet_device.h>
#include <spacket/multiplexer.h>
#include <spacket/crc.h>


ThreadStorageT<256> echoThreadStorage[MULTIPLEXER_NUM_CHANNELS];
const char* echoThreadNames[MULTIPLEXER_NUM_CHANNELS] = { "echo 0", "echo 1"};

using SerialDevice = SerialDeviceT<Buffer>;
using PacketDevice = PacketDeviceT<Buffer, SerialDevice>;
using Multiplexer = MultiplexerT<Buffer, PacketDevice, MULTIPLEXER_NUM_CHANNELS>;

constexpr tprio_t SD_THREAD_PRIORITY = NORMALPRIO + 3;
constexpr tprio_t PD_THREAD_PRIORITY = NORMALPRIO + 2;
constexpr tprio_t MX_THREAD_PRIORITY = NORMALPRIO + 1;

PacketDevice::Storage packetDeviceStorage;
Multiplexer::Storage multiplexerStorage;

namespace {

#ifdef ENABLE_DEBUG_PRINT

IMPLEMENT_DPL_FUNCTION

#else

IMPLEMENT_DPL_FUNCTION_NOP

#endif

} // namespace

class Globals {
public:
    static Result<Globals> init() {
        return
        SerialDevice::open(&UARTD1, SD_THREAD_PRIORITY) >=
        [&](SerialDevice sd) {
            return
            PacketDevice::open(std::move(sd), &packetDeviceStorage, PD_THREAD_PRIORITY) >=
            [&] (PacketDevice pd) {
                return
                Multiplexer::open(std::move(pd), &multiplexerStorage, MX_THREAD_PRIORITY) >=
                [&] (Multiplexer mx) {
                    return ok(Globals(std::move(mx)));
                };
            };
        };
    }

    Globals(Globals&&) = default;

Multiplexer& mx() { return mx_; }

private:
    Globals(Multiplexer&& mx)
        : mx_(std::move(mx))
    {}

private:
    Multiplexer mx_;
};

Result<Void> writeBuffer(std::uint8_t channel, Multiplexer& mx, Buffer b) {
    palClearPad(GPIOC, GPIOC_LED);
    return
    mx.write(channel, std::move(b)) >
    [&]() {
        dpl("write");
        palSetPad(GPIOC, GPIOC_LED);
        return ok();
    };
}

static __attribute__((noreturn)) void echoThreadFunction(Globals& g, std::uint8_t channel) {
    chRegSetThreadName(echoThreadNames[channel]);
    for (;;) {
        dpl("tick");
        g.mx().read(channel, INFINITE_TIMEOUT) >=
        [&](Buffer read) {
            dpl("read");
            return writeBuffer(channel, g.mx(), std::move(read));
        } <=
        threadErrorReport;
    }
}

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    auto globals = std::move(getOkUnsafe(Globals::init() <= fatal<Globals>));

    for (std::uint8_t c = 0; c < MULTIPLEXER_NUM_CHANNELS; ++c) {
        Thread::create(Thread::params(echoThreadStorage[c], NORMALPRIO), [&] { echoThreadFunction(globals, c); });
    }

    for (;;) {
        port_wait_for_interrupt();
    }
}
