#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"

#include <spacket/serial_device.h>
#include <spacket/util/mailbox.h>
#include <spacket/util/static_thread.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/result_fatal.h>
#include <spacket/packetizer_thread_function.h>

using BufferMailbox = MailboxT<Buffer, 1>;
using SerialDevice = SerialDeviceT<Buffer>;

StaticThreadT<256> rxThread;
StaticThreadT<256> txThread;
StaticThreadT<512> packetizerThread;

BufferMailbox packetizerIn;
BufferMailbox packetizerOut;

class Globals {
public:
    static Result<Globals> init() {
        return
        SerialDevice::open(&UARTD1) >=
        [&](SerialDevice&& sd) {
            return ok(Globals(std::move(sd)));
        };
    }

    SerialDevice& sd() { return sd_; }

private:
    Globals(SerialDevice&& sd)
        : sd_(std::move(sd))
    {}

private:
    SerialDevice sd_;
};

Result<boost::blank> writeBuffer(SerialDevice& sd, Buffer&& b) {
    palClearPad(GPIOC, GPIOC_LED);
    return
    sd.write(b) >
    [&]() {
        palSetPad(GPIOC, GPIOC_LED);
        return ok(boost::blank{});
    };
}

static __attribute__((noreturn)) THD_FUNCTION(rxThreadFunction, arg) {
    chRegSetThreadName("rx");
    Globals& g = *static_cast<Globals*>(arg);
    for (;;) {
        // debugPrintLine("tick");
        g.sd().read(INFINITE_TIMEOUT) >=
        [&](Buffer&& read) {
            DEBUG_PRINT_BUFFER(read);
            return packetizerIn.post(read, IMMEDIATE_TIMEOUT);
        } <=
        threadErrorReport;
    }
}

static __attribute__((noreturn)) THD_FUNCTION(txThreadFunction, arg) {
    chRegSetThreadName("tx");
    Globals& g = *static_cast<Globals*>(arg);
    for (;;) {
        packetizerOut.fetch(INFINITE_TIMEOUT) >=
        [&](Buffer&& fetched) {
            DEBUG_PRINT_BUFFER(fetched);
            return g.sd().write(fetched, INFINITE_TIMEOUT);
        } <=
        threadErrorReport;
    }
}    

static __attribute__((noreturn)) THD_FUNCTION(packetizerThreadFunction, arg) {
    (void)arg;
    chRegSetThreadName("packetizer");
    packetizerThreadFunctionT<Buffer>(packetizerIn, packetizerOut, threadErrorReport);
    for (;;);
}

int __attribute__((noreturn)) main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    auto globals = std::move(getOkUnsafe(Globals::init() <= fatal<Globals>));

    txThread.create(NORMALPRIO + 2, txThreadFunction, &globals);
    packetizerThread.create(NORMALPRIO + 1, packetizerThreadFunction, &globals);
    rxThread.create(NORMALPRIO, rxThreadFunction, &globals);

    while (true) {
        port_wait_for_interrupt();
    }
}
