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
#include "hal_serial.h"
#include "hal_channels.h"
#include "SEGGER_RTT.h"

#include <stdio.h>
#include <string.h>

#include <limits>

static inline systime_t systimeDiff(systime_t currentTime, systime_t prevTime) {
    return currentTime >= prevTime
        ? currentTime - prevTime
        : std::numeric_limits<systime_t>::max() - (prevTime - currentTime);
}

template <unsigned BUFFER_INDEX, uint32_t READ_POLL_PERIOD_MS>
class RTTStreamT: public BaseSequentialStream, public BaseChannel {
public:
    RTTStreamT()
        : BaseSequentialStream{(BaseSequentialStreamVMT*)&vmt_}
        , BaseChannel{&vmt_}
    {
    }

private:
    static RTTStreamT* cast(void* p) { return static_cast<RTTStreamT*>(p); }
    
private:
    static size_t write(void *ip, const uint8_t *bp, size_t n) { return cast(ip)->write(bp, n); }
    static size_t read(void *ip, uint8_t *bp, size_t n) { return cast(ip)->read(bp, n); }
    static msg_t put(void *ip, uint8_t b) { return cast(ip)->put(b); }
    static msg_t get(void *ip) { return cast(ip)->get(); }
    static size_t writet(
        void *ip,
        const uint8_t *bp,
        size_t n,
        systime_t time) {
        return cast(ip)->writet(bp, n, time);
    }
    static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t time) {
        return cast(ip)->readt(bp, n, time);
    }
    static msg_t putt(void *ip, uint8_t b, systime_t time) { return cast(ip)->putt(b, time); }
    static msg_t gett(void *ip, systime_t time) { return cast(ip)->gett(time); }

private:
    size_t writet(const uint8_t *bp, size_t n, systime_t) {
        return SEGGER_RTT_Write(BUFFER_INDEX, bp, n);
    }

    size_t readt(uint8_t *bp, size_t n, systime_t timeout) {
        systime_t prevTime = osalOsGetSystemTimeX();
        size_t read = 0;
        while(true) {
            read = SEGGER_RTT_Read(BUFFER_INDEX, bp, n);
            if (read) {
                return read;
            }
            chThdSleepMilliseconds(READ_POLL_PERIOD_MS);
            if (timeout != TIME_INFINITE) {
                systime_t currentTime = osalOsGetSystemTimeX();
                systime_t diff = systimeDiff(currentTime, prevTime);
                if (diff > timeout) {
                    return read;
                }
                timeout -= diff;
                prevTime = currentTime;
            }
        }
    }

    msg_t putt(uint8_t b, systime_t time) {
        switch (writet(&b, 1, time)) {
            case 1: return MSG_OK;
            default: return MSG_RESET;
        }
    }
    
    msg_t gett(systime_t time) {
        uint8_t b;
        switch (readt(&b, 1, time)) {
            case 1: return b;
            default: return MSG_RESET;
        }
    }

    msg_t put(uint8_t b) {
        return putt(b, TIME_INFINITE);
    }

    msg_t get() {
        return gett(TIME_INFINITE);
    }
    
    size_t write(const uint8_t *bp, size_t n) {
        return writet(bp, n, TIME_INFINITE);
    }

    size_t read(uint8_t *bp, size_t n) {
        return readt(bp, n, TIME_INFINITE);
    }
    
private:
    static const BaseChannelVMT vmt_;
};

template <unsigned BUFFER_INDEX, uint32_t READ_POLL_PERIOD_MS>
const BaseChannelVMT RTTStreamT<BUFFER_INDEX, READ_POLL_PERIOD_MS>::vmt_ = {
    RTTStreamT::write,
    RTTStreamT::read,
    RTTStreamT::put,
    RTTStreamT::get,
    RTTStreamT::putt,
    RTTStreamT::gett,
    RTTStreamT::writet,
    RTTStreamT::readt
};

using RTTStream = RTTStreamT<0, 10>;

static RTTStream rttStream{};

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

  streamWrite(&SD1, buf, sizeof buf - 1);
}

static const ShellCommand commands[] = {
  {"write", cmd_write},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&rttStream,
  commands
};


/*
 * Blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 176);
static __attribute__((noreturn)) THD_FUNCTION(Thread1, arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    const systime_t time = 250;
    palClearPad(GPIOC, GPIOC_LED);
    chThdSleepMilliseconds(time);
    palSetPad(GPIOC, GPIOC_LED);
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

  chprintf(&rttStream, "RTT ready\n\r");
  
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

  while (true) {
    port_wait_for_interrupt();
    CH_CFG_IDLE_LOOP_HOOK();
  }
}
