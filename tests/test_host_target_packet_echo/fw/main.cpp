#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"

#include <spacket/serial_device.h>
#include <spacket/thread.h>
#include <spacket/fatal_error.h>


constexpr tprio_t SD_THREAD_PRIORITY = NORMALPRIO + 1;

ThreadStorageT<256> echoThreadStorage;
Allocator allocator;

static __attribute__((noreturn)) void echoThreadFunction() {
    chRegSetThreadName("echo");
    SerialDevice::open(allocator, UARTD1, SD_THREAD_PRIORITY) >=
    [&](SerialDevice&& sd) {
        auto writeBuffer =
        [&](Buffer&& b) {
            palClearPad(GPIOC, GPIOC_LED);
            return
            sd.write(b) >
            [&]() {
                palSetPad(GPIOC, GPIOC_LED);
                return ok();
            };
        };
        
        for (;;) {
            sd.read(INFINITE_TIMEOUT) >=
            writeBuffer <=
            [&](Error e) {
                DefaultToStringBuffer buf;
                chprintf(&rttStream, "E: %s", toString(e, buf));
                return ok();
            };
        }
        // will never get here
        return ok();
    } <=
    [&](Error e) {
        DefaultToStringBuffer buf;
        FATAL_ERROR(toString(e, buf));
        // will never get here
        return ok();
    };
    for (;;) {}
}

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
  
    Thread::create(Thread::params(echoThreadStorage, NORMALPRIO), echoThreadFunction);

    while (true) {
        port_wait_for_interrupt();
    }
}
