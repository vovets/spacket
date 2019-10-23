#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shell_commands.h"

#include "rtt_stream.h"

#include <spacket/thread.h>
#include <spacket/fatal_error.h>

ThreadStorageT<176> blinkerThreadStorage;
ThreadStorageT<256> echoThreadStorage;
ThreadStorageT<1024> shellThreadStorage;
ThreadStorageT<256> rxThreadStorage;

static __attribute__((noreturn)) void blinkerThreadFunction() {
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

static __attribute__((noreturn)) void echoThreadFunction() {
    chRegSetThreadName("echo");
    for (;;) {
        chThdSleepMilliseconds(1);
    }
}

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
  
    Thread::create(blinkerThreadStorage, NORMALPRIO - 2, blinkerThreadFunction);
    Thread::create(echoThreadStorage, NORMALPRIO - 1, echoThreadFunction);
    auto shellThread_ = Thread::create(shellThreadStorage, NORMALPRIO, [] { shellThread(const_cast<ShellConfig*>(&shellConfig)); });
    chRegSetThreadNameX(shellThread_.nativeHandle(), "shell");
    Thread::create(rxThreadStorage, NORMALPRIO + 1, rxThreadFunction);

    while (true) {
        port_wait_for_interrupt();
    }
}
