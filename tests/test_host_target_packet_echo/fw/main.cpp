#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"

#include <spacket/serial_device.h>
#include <spacket/util/static_thread.h>
#include <spacket/fatal_error.h>


constexpr tprio_t SD_THREAD_PRIORITY = NORMALPRIO + 1;

using SerialDevice = SerialDeviceT<Buffer>;

StaticThreadT<256> echoThread;

static __attribute__((noreturn)) THD_FUNCTION(echoThreadFunction, arg) {
    (void)arg;
    chRegSetThreadName("echo");
    SerialDevice::open(&UARTD1, SD_THREAD_PRIORITY) >=
    [&](SerialDevice&& sd) {
        auto writeBuffer =
        [&](Buffer&& b) {
            palClearPad(GPIOC, GPIOC_LED);
            return
            sd.write(b) >
            [&]() {
                palSetPad(GPIOC, GPIOC_LED);
                return ok(boost::blank{});
            };
        };
        
        for (;;) {
            sd.read(INFINITE_TIMEOUT) >=
            writeBuffer <=
            [&](Error e) {
                chprintf(&rttStream, "E: %s", toString(e));
                return ok(boost::blank{});
            };
        }
        // will never get here
        return ok(boost::blank{});
    } <=
    [&](Error e) {
        FATAL_ERROR(toString(e));
        // will never get here
        return ok(boost::blank{});
    };
    for (;;) {}
}

int __attribute__((noreturn)) main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
  
    echoThread.create(NORMALPRIO, echoThreadFunction, 0);

    while (true) {
        port_wait_for_interrupt();
    }
}
