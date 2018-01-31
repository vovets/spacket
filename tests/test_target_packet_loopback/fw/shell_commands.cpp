#include "rtt_stream.h"
#include "shell_commands.h"
#include <test_macros.h>
#include "buffer.h"

#include "shell.h"
#include "chprintf.h"

#include <spacket/serial_device.h>

#include <chrono>

namespace {

auto uartDriver = &UARTD1;

}

static void cmd_test_create(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    {
        auto sd = SerialDevice::open(uartDriver);
        CHECK(isOk(sd));
    }
    CHECK(uartDriver->state == UART_STOP);
}

static void cmd_test_rx_timeout(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    auto result =
    SerialDevice::open(uartDriver) >>=
    [&](SerialDevice&& sd) {
        return
        Buffer::create(10) >>=
        [&](Buffer&& b) {
            chprintf(chp, "created: %d\n", b.size());
            return
            sd.read(std::chrono::milliseconds(100), std::move(b)) >>=
            [&](Buffer&& b) {
                chprintf(chp, "read: %d\n", b.size());
                return ok(b.size());
            };
        };
    };
    CHECK(isFail(result));
    CHECK(getFail(result) == Error::DevReadTimeout);
}

static const ShellCommand commands[] = {
    {"test_create", cmd_test_create},
    {"test_rx_timeout", cmd_test_rx_timeout},
    {NULL, NULL}
};

const ShellConfig shellConfig = {
    &rttStream,
    commands
};
