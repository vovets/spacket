#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shell_commands.h"

#include "rtt_stream.h"

#include <spacket/util/static_thread.h>
#include <spacket/fatal_error.h>

StaticThreadT<176> blinkerThread;
StaticThreadT<256> echoThread;
StaticThreadT<1024> shellThread_;
StaticThreadT<176> rxThread;

static __attribute__((noreturn)) THD_FUNCTION(blinkerThreadFunction, arg) {
    (void)arg;
    chRegSetThreadName("blinker");
    while(true) {
        const systime_t time = 250;
        #define GPIO_PORT GPIOC
        #define LED GPIOC_LED
        palClearPad(GPIO_PORT, LED);
        chThdSleepMilliseconds(time);
        palSetPad(GPIO_PORT, LED);
        chThdSleepMilliseconds(time);
        #undef LED
    }
}

static __attribute__((noreturn)) THD_FUNCTION(echoThreadFunction, arg) {
    (void)arg;
    chRegSetThreadName("echo");
    for (;;) {
        chThdSleepMilliseconds(1);
    }
}

int __attribute__((noreturn)) main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
  
    blinkerThread.create(NORMALPRIO - 2, blinkerThreadFunction, 0);
    echoThread.create(NORMALPRIO - 1, echoThreadFunction, 0);
    shellThread_.create(NORMALPRIO, shellThread, const_cast<ShellConfig*>(&shellConfig));
    rxThread.create(NORMALPRIO + 1, rxThreadFunction, 0);

    while (true) {
        port_wait_for_interrupt();
    }
}
