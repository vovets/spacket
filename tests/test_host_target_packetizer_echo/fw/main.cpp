#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"

#include <spacket/serial_device.h>
#include <spacket/mailbox.h>
#include <spacket/thread.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/result_fatal.h>
#include <spacket/packetizer_thread_function.h>

using BufferMailbox = MailboxT<Buffer>;
using SerialDevice = SerialDeviceT<Buffer>;

constexpr tprio_t SD_THREAD_PRIORITY = NORMALPRIO + 3;

ThreadStorageT<256> rxThreadStorage;
ThreadStorageT<256> txThreadStorage;
ThreadStorageT<512> packetizerThreadStorage;

BufferMailbox packetizerIn;
BufferMailbox packetizerOut;

#define PREFIX()
#ifdef ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPX_FUNCTIONS
IMPLEMENT_DPB_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPX_FUNCTIONS_NOP
IMPLEMENT_DPB_FUNCTION_NOP
#endif
#undef PREFIX

class Globals {
public:
    static Result<Globals> init() {
        return
        SerialDevice::open(&UARTD1, SD_THREAD_PRIORITY) >=
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

Result<Void> writeBuffer(SerialDevice& sd, Buffer&& b) {
    palClearPad(GPIOC, GPIOC_LED);
    dpb("write: ", &b);
    return
    sd.write(b) >
    [&]() {
        palSetPad(GPIOC, GPIOC_LED);
        return ok();
    };
}

static __attribute__((noreturn)) THD_FUNCTION(rxThreadFunction, arg) {
    chRegSetThreadName("rx");
    Globals& g = *static_cast<Globals*>(arg);
    for (;;) {
        dpl("tick");
        g.sd().read(INFINITE_TIMEOUT) >=
        [&](Buffer&& read) {
            dpb("read: ", &read);
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
            DPB(fetched);
            return writeBuffer(g.sd(), std::move(fetched));
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

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    auto globals = std::move(getOkUnsafe(Globals::init() <= fatal<Globals>));

    Thread::create(Thread::params(txThreadStorage, NORMALPRIO + 2), [&] { txThreadFunction(&globals); });
    Thread::create(Thread::params(packetizerThreadStorage, NORMALPRIO + 1), [&] { packetizerThreadFunction(&globals); });
    Thread::create(Thread::params(rxThreadStorage, NORMALPRIO), [&] { rxThreadFunction(&globals); });

    while (true) {
        port_wait_for_interrupt();
    }
}
