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

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "ch_test.h"
#include "shell.h"
#include "chprintf.h"
#include "hal_serial.h"
#include "SEGGER_RTT.h"

#define _rtt_stream_data                                                   \
  _base_sequential_stream_data

struct RTTStreamVMT {
  _base_sequential_stream_methods
};

typedef struct {
    const struct RTTStreamVMT *vmt;
    _rtt_stream_data
} RTTStream;

static size_t rtt_write(void *ip, const uint8_t *bp, size_t n) {
    (void)ip;
    return SEGGER_RTT_Write(0, bp, n);
}

static size_t rtt_read(void *ip, uint8_t *bp, size_t n) {
    (void)ip;
    return SEGGER_RTT_Read(0, bp, n);
}

static msg_t rtt_put(void *ip, uint8_t b) {
    if (rtt_write(ip, &b, 1) != 1) {
        return MSG_RESET;
    }
    return MSG_OK;
}

static msg_t rtt_get(void *ip) {
    uint8_t b;
    if (rtt_read(ip, &b, 1) != 1) {
        return MSG_RESET;
    }
    return b;
}

static const struct RTTStreamVMT rtt_vmt = {rtt_write, rtt_read, rtt_put, rtt_get};

static RTTStream rttStream_;

static BaseSequentialStream* const rttStream = (BaseSequentialStream*)&rttStream_;

static void RTTStreamObjectInit(RTTStream *s) {
    s->vmt = &rtt_vmt;
}


/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)

static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[]) {
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

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: write\r\n");
    return;
  }

  while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
    streamWrite(&SD1, buf, sizeof buf - 1);
  }
  chprintf(chp, "\r\n\nstopped\r\n");
}

static const ShellCommand commands[] = {
  {"write", cmd_write},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD1,
  commands
};

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

/*
 * Blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 160);
static __attribute__((noreturn)) THD_FUNCTION(Thread1, arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    const systime_t time = 250;
    palClearPad(GPIOC, GPIOC_LED);
    chprintf(rttStream, "flip O\n\r");
    chThdSleepMilliseconds(time);
    palSetPad(GPIOC, GPIOC_LED);
    chprintf(rttStream, "flop *\n\r");
    chThdSleepMilliseconds(time);
  }
}

static THD_WORKING_AREA(waShellThread, 2048);

/*
 * Application entry point.
 */
int __attribute__((noreturn)) main(void) {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  RTTStreamObjectInit(&rttStream_);

  chprintf(rttStream, "Hello from RTT!\n\r");
  
  /*
   * Initializes a serial driver.
   */
  sdStart(&SD1, NULL);

  /*
   * Shell manager initialization.
   */
  shellInit();

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  chThdCreateStatic(
      waShellThread, sizeof(waShellThread), NORMALPRIO, shellThread, (void*)&shell_cfg1);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
