#include "system_halt_hook.h"
#include "rtt_stream.h"

#include "ch.h"
#include "hal.h"
#include "chprintf.h"


void systemHaltHook(const char* reason) {
    palClearPad(GPIOC, GPIOC_LED);
    chprintf(&rttStream, "%s\n", reason);
}
