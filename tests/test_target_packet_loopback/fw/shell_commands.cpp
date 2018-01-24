#include "rtt_stream.h"
#include "shell_commands.h"
#include <test_macros.h>

#include "shell.h"
#include "chprintf.h"

#include <spacket/serial_device.h>


static void cmd_test_create(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    auto sd = SerialDevice::open();
    CHECK(isOk(sd));
}

static const ShellCommand commands[] = {
    {"test_create", cmd_test_create},
    {NULL, NULL}
};

const ShellConfig shellConfig = {
    &rttStream,
    commands
};
