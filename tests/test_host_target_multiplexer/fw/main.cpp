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
#include <spacket/memory_utils.h>


ThreadStorageT<256> echoThreadStorage[MULTIPLEXER_NUM_CHANNELS];
const char* echoThreadNames[MULTIPLEXER_NUM_CHANNELS] = { "echo 0", "echo 1"};
Allocator allocator;

using PacketDevice = PacketDeviceT<SerialDevice>;
using Multiplexer = MultiplexerT<PacketDevice, MULTIPLEXER_NUM_CHANNELS>;

constexpr tprio_t SD_THREAD_PRIORITY = NORMALPRIO + 3;
constexpr tprio_t PD_THREAD_PRIORITY = NORMALPRIO + 2;
constexpr tprio_t MX_THREAD_PRIORITY = NORMALPRIO + 1;

namespace {

#ifdef ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
#endif

} // namespace

class Globals {
public:
    static Result<StaticPtr<Globals>> init(void* storage) {
        return
            SerialDevice::open(allocator, UARTD1, SD_THREAD_PRIORITY) >=
            [&](SerialDevice sd) {
                return
                    Buffer::create(allocator) >=
                    [&] (Buffer&& pdBuffer) {
                        return ok(makeStatic<Globals>(storage, std::move(sd), std::move(pdBuffer)));
                    };
            };
    }

    Multiplexer& mx() { return *mx_; }

    Globals(SerialDevice&&sd, Buffer&& pdBuffer)
        : sd_(std::move(sd))
        , pd_(new (&pdStorage) PacketDevice(sd_, std::move(pdBuffer), PD_THREAD_PRIORITY))
        , mx_(new (&mxStorage) Multiplexer(allocator, *pd_, MX_THREAD_PRIORITY))
    {}

private:
    SerialDevice sd_;
    Storage<PacketDevice> pdStorage;
    Storage<Multiplexer> mxStorage;
    StaticPtr<PacketDevice> pd_;
    StaticPtr<Multiplexer> mx_;
};

Storage<Globals> globalsStorage;

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
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize(allocator));

    auto globals = std::move(getOkUnsafe(Globals::init(&globalsStorage) <= fatal<StaticPtr<Globals>>));

    for (std::uint8_t c = 0; c < MULTIPLEXER_NUM_CHANNELS; ++c) {
        Thread::create(Thread::params(echoThreadStorage[c], NORMALPRIO), [&] { echoThreadFunction(*globals, c); });
    }

    for (;;) {
        port_wait_for_interrupt();
    }
}
