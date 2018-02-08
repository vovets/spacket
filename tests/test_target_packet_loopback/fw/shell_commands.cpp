#include "rtt_stream.h"
#include "shell_commands.h"
#include <test_macros.h>
#include "buffer.h"

#include "shell.h"
#include "chprintf.h"

#include <spacket/serial_device.h>
#include <spacket/buffer_utils.h>
#include <spacket/util/mailbox.h>
#include <spacket/util/static_thread.h>

#include <chrono>

namespace {

auto uartDriver = &UARTD1;

}

static void cmd_test_create(BaseSequentialStream *stream, int, char*[]) {
    {
        auto sd = SerialDevice::open(uartDriver);
        CHECK(isOk(sd));
    }
    CHECK(uartDriver->state == UART_STOP);
}

static void cmd_test_rx_timeout(BaseSequentialStream *stream, int, char*[]) {
    auto result =
    SerialDevice::open(uartDriver) >=
    [&](SerialDevice&& sd) {
        return
        Buffer::create(10) >=
        [&](Buffer&& b) {
            chprintf(stream, "created: %d\r\n", b.size());
            return
            sd.read(std::move(b), std::chrono::milliseconds(100)) >=
            [&](Buffer&& b) {
                chprintf(stream, "read: %d\r\n", b.size());
                return ok(b.size());
            };
        };
    };
    CHECK(isFail(result));
    CHECK(getFail(result) == Error::DevReadTimeout);
}

struct RxContext {
    SerialDevice sd;
    Timeout timeout;
};

static MailboxT<RxContext, 1> rxInMailbox;
static MailboxT<Result<Buffer>, 1> rxOutMailbox;

static Result<Buffer> testBuffer(size_t size) {
    return
    Buffer::create(size) >=
    [&](Buffer&& b) {
        for (size_t i = 0; i < size - 1; ++i) {
            *(b.begin() + i) = '0' + i % 10;
        }
        *(b.end() - 1) = '\0';
        return ok(std::move(b));
    };
}

THD_FUNCTION(rxThreadFunction, arg) {
    static_cast<void>(arg);
    chRegSetThreadName("rx");
    while (true) {
        rxInMailbox.fetch(INFINITE_TIMEOUT) >=
        [&](RxContext&& c) {
            return
            Buffer::create(Buffer::maxSize()) >=
            [&](Buffer&& b) {
                auto r = c.sd.read(b, c.timeout);
                return
                rxOutMailbox.post(r, INFINITE_TIMEOUT);
            } <=
            [&](Error e) {
                auto r = fail<Buffer>(e);
                palSetPad(GPIOA, GPIOA_DBG0);
                return
                rxOutMailbox.post(r, INFINITE_TIMEOUT);
            };
        };
    }
}

static Result<bool> test_loopback(SerialDevice sd, size_t packetSize, Timeout rxTimeout, BaseSequentialStream *stream) {
    return
    testBuffer(packetSize) >=
    [&](Buffer&& reference) {
        RxContext rxContext{sd, rxTimeout};
        return
        [&]{ return rxInMailbox.post(rxContext, INFINITE_TIMEOUT); }() >
        [&]{ return sd.write(reference); } >
        []{ return rxOutMailbox.fetch(INFINITE_TIMEOUT); } >=
        [&](Result<Buffer>&& r) {
            returnOnFailT(b, bool, std::move(r));
            if (b != reference) {
                chprintf(stream, "created : [%d]%s\r\n", reference.size(), reference.begin());
                chprintf(stream, "received: [%d]%s\r\n", b.size(), b.begin());
                return ok(false);
            }
            return ok(true);
        };
    };
}

static void test_loopback_loop(BaseSequentialStream *stream, size_t packetSize, size_t repetitions, Timeout rxTimeout) {
    SerialDevice::open(uartDriver) >=
    [&](SerialDevice&& sd) {
        bool passed = false;
        for (size_t i = 0; i < repetitions; ++i) {
            auto result = test_loopback(sd, packetSize, rxTimeout, stream) >=
            [&](bool packetsEqual) {
                if (!packetsEqual) {
                    chprintf(
                        stream,
                        "FAILURE[Packets differ] s=%d r=%d\r\n",
                        packetSize,
                        i);
                    return ok(false);
                }
                return ok(true);
            };
            returnOnFail(passed_, result);
            passed = passed_;
        };
        if (passed) {
            chprintf(stream, "SUCCESS\r\n");
        }
        return ok(false);
    } <=
    [&](Error e) {
        chprintf(stream, "FAILURE[%s]\r\n", toString(e));
        return ok(false);
    };
}

static void cmd_test_loopback(BaseSequentialStream *stream, int argc, char* argv[]) {
    if (argc < 2) {
        chprintf(stream, "FAILURE[Bad args]\r\n");
        return;
    }
    size_t packetSize = std::atol(argv[0]);
    size_t repetitions = std::atol(argv[1]);
    size_t milliseconds = argc > 2 ? std::atol(argv[2]) : 0;
    Timeout timeout = milliseconds > 0
    ? std::chrono::milliseconds(milliseconds)
    : INFINITE_TIMEOUT;

    chDbgResumeTrace(CH_DBG_TRACE_MASK_SWITCH);
    test_loopback_loop(stream, packetSize, repetitions, timeout);
    chDbgSuspendTrace(CH_DBG_TRACE_MASK_SWITCH);
}

static const ShellCommand commands[] = {
    {"test_create", cmd_test_create},
    {"test_rx_timeout", cmd_test_rx_timeout},
    {"test_loopback", cmd_test_loopback},
    {NULL, NULL}
};

const ShellConfig shellConfig = {
    &rttStream,
    commands
};
