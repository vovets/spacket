#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shell_commands.h"

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
StaticThread<512> shellThread_{};

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
        chThdSleepMilliseconds(1);
    }
}

int __attribute__((noreturn)) main(void) {
  halInit();
  chSysInit();

  chprintf(&rttStream, "RTT ready\n");
  
  blinkerThread.create(NORMALPRIO - 2, blinkerThreadFunction, 0);
  echoThread.create(NORMALPRIO - 1, echoThreadFunction, 0);
  shellThread_.create(NORMALPRIO, shellThread, const_cast<ShellConfig*>(&shellConfig));

  while (true) {
    port_wait_for_interrupt();
    CH_CFG_IDLE_LOOP_HOOK();
  }
}