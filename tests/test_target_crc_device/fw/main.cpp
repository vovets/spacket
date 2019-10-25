#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "shell_commands.h"

#include "rtt_stream.h"

#include <spacket/thread.h>

ThreadStorageT<1024> shellThreadStorage;

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");

    Thread::create(Thread::params(shellThreadStorage, NORMALPRIO), [] { shellThread(const_cast<ShellConfig*>(&shellConfig)); });

    for (;;) {
        port_wait_for_interrupt();
    }
}
