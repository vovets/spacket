#include "rtt_stream.h"
#include "shell_commands.h"
#include <test_macros.h>
#include "buffer.h"

#include "shell.h"
#include "chprintf.h"

#include <spacket/serial_device.h>
#include <spacket/buffer_utils.h>
#include <spacket/serial_utils.h>
#include <spacket/util/mailbox.h>
#include <spacket/util/static_thread.h>
#include <spacket/util/time_measurement.h>

#include <chrono>

namespace {

auto uartDriver = &UARTD1;

}

template <typename Buffer>
void print(BaseSequentialStream* stream, const Buffer& b) {
    chprintf(stream, "[%d]", b.size());
    for (auto c: b) {
        streamPut(stream, c);
    }
}

static void cmd_reset(BaseSequentialStream *stream, int, char*[]) {
    chprintf(stream, "Bye!");
    NVIC_SystemReset();
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
    CHECK(getFail(result) == toError(ErrorCode::DevReadTimeout));
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
        size_t i = 0;
        for (auto& c: b) {
            c = '0' + i % 10;
            ++i;
        }
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
                chprintf(stream, "created : ");
                print(stream, reference);
                chprintf(stream, "\r\n");
                chprintf(stream, "received: ");
                print(stream, b);
                chprintf(stream, "\r\n");
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
        TimeMeasurement tm;
        for (size_t i = 0; i < repetitions; ++i) {
            tm.start();
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
            tm.stop();
            passed = passed_;
        };
        if (passed) {
            chprintf(stream, "SUCCESS %d %d\r\n", tm.tm.best, tm.tm.worst);
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

    test_loopback_loop(stream, packetSize, repetitions, timeout);
}

static void cmd_test_info(BaseSequentialStream *stream, int, char*[]) {
    for (size_t size = 1; size < 249; ++size) {
        auto pt = packetTime(921600, size).count();
        auto pt64 = packetTime64(921600, size).count();
        chprintf(stream, "%d %d %d\r\n", size, pt, pt64);
    }
    chprintf(stream, "SUCCESS\r\n");
}

static const ShellCommand commands[] = {
    {"reset", cmd_reset},
    {"test_create", cmd_test_create},
    {"test_rx_timeout", cmd_test_rx_timeout},
    {"test_loopback", cmd_test_loopback},
    {"test_info", cmd_test_info},
    {NULL, NULL}
};

const ShellConfig shellConfig = {
    &rttStream,
    commands
};
