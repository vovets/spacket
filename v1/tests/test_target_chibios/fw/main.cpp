/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include "ch_test.h"
#include "shell.h"
#include "chprintf.h"

#include <spacket/util/rtt_stream.h>
#include <spacket/thread.h>

using RTTStream = RTTStreamT<0, 10>;

RTTStream rttStream{};
ThreadStorageT<176> blinkerThreadStorage;
ThreadStorageT<128> writerThreadStorage;
ThreadStorageT<2048> shellThreadStorage;
Thread writerThread;

static void writerThreadFunction();

static void cmd_start_write(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: start_write\n");
        return;
    }
    if(writerThread.joinable()) {
        chprintf(chp, "already running\n");
        return;
    }
    Thread::create(Thread::params(writerThreadStorage, NORMALPRIO), writerThreadFunction);
    chprintf(chp, "started\r\n");
}

static void cmd_stop_write(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: stop_write\n");
        return;
    }
    if (!writerThread.joinable()) {
        chprintf(chp, "not running\n");
        return;
    }
    writerThread.requestStop();
    writerThread.wait();
    chprintf(chp, "stopped\n");
}

static const ShellCommand commands[] = {
    {"start_write", cmd_start_write},
    {"stop_write", cmd_stop_write},
    {NULL, NULL}
};

static const ShellConfig shellCfg = {
    &rttStream,
    commands
};



static __attribute__((noreturn)) void blinkerThreadFunction() {
    chRegSetThreadName("blinker");
    while(true) {
        const systime_t time = 250;
        palClearPad(GPIOC, GPIOC_LED);
        chThdSleepMilliseconds(time);
        palSetPad(GPIOC, GPIOC_LED);
        chThdSleepMilliseconds(time);
    }
}

static void writerThreadFunction() {
    static uint8_t buf[] =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    chRegSetThreadName("writer");

    while(!chThdShouldTerminateX()) {
        streamWrite(&SD1, buf, sizeof buf - 1);
        chThdYield();
    }
}

int main(void) {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  chprintf(&rttStream, "RTT ready\r\n");
  
  sdStart(&SD1, NULL);

  shellInit();

  Thread::create(Thread::params(blinkerThreadStorage, NORMALPRIO), blinkerThreadFunction);
  Thread::create(Thread::params(shellThreadStorage, NORMALPRIO), [] { shellThread(const_cast<ShellConfig *>(&shellCfg)); });

  while (true) {
    port_wait_for_interrupt();
    CH_CFG_IDLE_LOOP_HOOK();
  }
}
