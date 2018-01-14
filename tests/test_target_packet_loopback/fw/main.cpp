#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include <spacket/util/rtt_stream.h>

template <size_t STACK_SIZE>
class StaticThread {
public:
    thread_t* create(tprio_t prio, tfunc_t f, void* arg) {
        return chThdCreateStatic(
            workingArea,
            sizeof(workingArea),
            prio,
            f,
            arg);
    }

private:
    THD_WORKING_AREA(workingArea, STACK_SIZE);
};

using RTTStream = RTTStreamT<0, 10>;

RTTStream rttStream{};
StaticThread<176> blinkerThread{};
StaticThread<128> writerThread{};
thread_t* writerThreadPtr = nullptr;
StaticThread<2048> shellThread_{};

static void writerThreadFunction(void*);

static void cmd_start_write(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: start_write\r\n");
        return;
    }
    if(writerThreadPtr) {
        chprintf(chp, "already running\r\n");
        return;
    }
    writerThreadPtr = writerThread.create(NORMALPRIO, writerThreadFunction, 0);
    chprintf(chp, "started\r\n");
}

static void cmd_stop_write(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: stop_write\r\n");
        return;
    }
    if (!writerThreadPtr) {
        chprintf(chp, "not running\r\n");
        return;
    }
    chThdTerminate(writerThreadPtr);
    chThdWait(writerThreadPtr);
    writerThreadPtr = nullptr;
    chprintf(chp, "stopped\r\n");
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

static THD_FUNCTION(writerThreadFunction, arg) {
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

    (void)arg;

    chRegSetThreadName("writer");

    while(!chThdShouldTerminateX()) {
        streamWrite(&SD1, buf, sizeof buf - 1);
        chThdYield();
    }
}

int __attribute__((noreturn)) main(void) {
  halInit();
  chSysInit();

  chprintf(&rttStream, "RTT ready\n\r");
  
  sdStart(&SD1, NULL);

  shellInit();

  blinkerThread.create(NORMALPRIO, blinkerThreadFunction, 0);
  shellThread_.create(NORMALPRIO, shellThread, const_cast<ShellConfig*>(&shellCfg));

  while (true) {
    port_wait_for_interrupt();
    CH_CFG_IDLE_LOOP_HOOK();
  }
}
