#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include <spacket/util/rtt_stream.h>
#include <spacket/util/static_thread.h>
#include <spacket/util/guarded_memory_pool.h>
#include <spacket/buffer_pool_allocator.h>

using RTTStream = RTTStreamT<0, 10>;
using Pool = GuardedMemoryPoolT<256, 8>;

RTTStream rttStream{};
StaticThread<176> blinkerThread{};
Pool pool;

static __attribute__((noreturn)) THD_FUNCTION(blinkerThreadFunction, arg) {
    (void)arg;
    chRegSetThreadName("blinker");
    while(true) {
        const systime_t time = 250;
        palClearPad(GPIOC, GPIOC_LED);
        chThdSleepMilliseconds(time);
        palSetPad(GPIOC, GPIOC_LED);
        chThdSleepMilliseconds(time);
    }
}

int __attribute__((noreturn)) main(void) {
  halInit();
  chSysInit();

  chprintf(&rttStream, "RTT ready\n\r");
  
  sdStart(&SD1, NULL);

  blinkerThread.create(NORMALPRIO, blinkerThreadFunction, 0);

  while (true) {
    port_wait_for_interrupt();
    CH_CFG_IDLE_LOOP_HOOK();
  }
}
