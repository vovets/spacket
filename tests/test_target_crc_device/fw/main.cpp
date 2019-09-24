#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "shell_commands.h"

#include "rtt_stream.h"

#include <spacket/util/static_thread.h>

StaticThreadT<1024> shellThread_;

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");

    shellThread_.create(NORMALPRIO, shellThread, const_cast<ShellConfig*>(&shellConfig));

    for (;;) {
        port_wait_for_interrupt();
    }
}
