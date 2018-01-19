#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"

#include <spacket/util/static_thread.h>
#include <spacket/util/guarded_memory_pool.h>
#include <spacket/buffer_pool_allocator.h>
#include <spacket/fatal_error.h>
#include <spacket/buffer.h>

const size_t MAX_BUFFER_SIZE = 256;

using Pool = GuardedMemoryPoolT<MAX_BUFFER_SIZE, 8>;
Pool pool;
using BufferAllocator = PoolAllocatorT<Pool, pool>;
using Buffer = BufferT<BufferAllocator, MAX_BUFFER_SIZE>;

StaticThread<176> blinkerThread{};
StaticThread<256> echoThread{};

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

static __attribute__((noreturn)) THD_FUNCTION(echoThreadFunction, arg) {
    (void)arg;
    chRegSetThreadName("echo");
    for (;;) {
        FATAL_ERROR("hello from echo thread!");
    }
}

int __attribute__((noreturn)) main(void) {
  halInit();
  chSysInit();

  chprintf(&rttStream, "RTT ready\n\r");
  
  sdStart(&SD1, NULL);

  blinkerThread.create(NORMALPRIO, blinkerThreadFunction, 0);
  echoThread.create(NORMALPRIO, echoThreadFunction, 0);

  while (true) {
    port_wait_for_interrupt();
    CH_CFG_IDLE_LOOP_HOOK();
  }
}
