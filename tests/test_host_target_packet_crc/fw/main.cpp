#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"
#include "constants.h"

#include <spacket/serial_device.h>
#include <spacket/mailbox.h>
#include <spacket/util/static_thread.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/result_fatal.h>
#include <spacket/cobs.h>
#include <spacket/buffer_debug.h>
#include <spacket/packetizer.h>
#include <spacket/packet_device.h>
#include <spacket/crc.h>


StaticThreadT<512> echoThread;

using SerialDevice = SerialDeviceT<Buffer>;
using PacketDevice = PacketDeviceT<Buffer>;

constexpr tprio_t SD_THREAD_PRIORITY = NORMALPRIO + 2;
constexpr tprio_t PD_THREAD_PRIORITY = NORMALPRIO + 1;

PacketDevice::Storage packetDeviceStorage;

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
                return ok(Globals(std::move(pd)));
            };
        };
    }

    Globals(Globals&&) = default;

    PacketDevice& pd() { return pd_; }

private:
    Globals(PacketDevice&& pd)
        : pd_(std::move(pd))
    {}

private:
    PacketDevice pd_;
};

Result<boost::blank> writeBuffer(PacketDevice& pd, Buffer b) {
    palClearPad(GPIOC, GPIOC_LED);
    return
    pd.write(std::move(b)) >
    [&]() {
        dpl("write");
        palSetPad(GPIOC, GPIOC_LED);
        return ok(boost::blank{});
    };
}

static __attribute__((noreturn)) THD_FUNCTION(echoThreadFunction, arg) {
    chRegSetThreadName("echo");
    Globals& g = *static_cast<Globals*>(arg);
    for (;;) {
        dpl("tick");
        g.pd().read(INFINITE_TIMEOUT) >=
        [&](Buffer read) {
            dpl("read");
            return crc::check(std::move(read));
        } >=
        [&](Buffer checked) {
            return
            crc::append(std::move(checked)) >=
            [&] (Buffer appended) { return writeBuffer(g.pd(), std::move(appended)); };
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

    echoThread.create(NORMALPRIO, echoThreadFunction, &globals);

    for (;;) {
        port_wait_for_interrupt();
    }
}
