#include "system_halt_hook.h"

#include <spacket/debug_print.h>

#include "hal.h"


void systemHaltHook(const char* reason) {
    palClearPad(LED_PORT, LED_PIN);
    static dbg::ArrayOut<128> out;
    oprintf(out,"FATAL: %s", reason);
    dbg::suffix(out);
    dbg::write(out);
}
