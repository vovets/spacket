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
#include <spacket/util/thread_error_report.h>

#include <chrono>

using SerialDevice = SerialDeviceT<Buffer>;

namespace {

auto uartDriver = &UARTD1;
tprio_t serialDeviceThreadPriority = NORMALPRIO + 2;

#if 0
IMPLEMENT_DPL_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
#endif

inline
std::uint32_t toNs(rtcnt_t counter) {
    return to<std::chrono::nanoseconds, STM32_SYSCLK>(counter);
}

inline
std::uint32_t toMs(rtcnt_t counter) {
    return to<std::chrono::milliseconds, STM32_SYSCLK>(counter);
}

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
        auto sd = SerialDevice::open(uartDriver, serialDeviceThreadPriority);
        CHECK(isOk(sd));
    }
    CHECK(uartDriver->state == UART_STOP);
}

static void cmd_test_rx_timeout(BaseSequentialStream *stream, int, char*[]) {
    auto result =
    SerialDevice::open(uartDriver, serialDeviceThreadPriority) >=
    [&](SerialDevice&& sd) {
        return
        sd.read(std::chrono::milliseconds(100)) >=
        [&](Buffer&& b) {
            chprintf(stream, "read: %d\r\n", b.size());
            return ok(b.size());
        };
    };
    CHECK(isFail(result));
    auto e = getFail(result);
    dpl("cmd_test_rx_timeout: %s", toString(e));
    CHECK(e == toError(ErrorCode::SerialDeviceReadTimeout));
}

struct RxContext {
    SerialDevice& sd;
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
            auto r = c.sd.read(c.timeout) <=
            [&](Error e) {
                // to clear the sd read buffer when the packet finally arrives
                // so consequent read wouldn't return stale packet
                c.sd.read(std::chrono::milliseconds(50)) >
                [&] {
                    dpl("second read ok");
                    return ok(boost::blank());
                };
                return fail<Buffer>(e);
            };
            dpl("isOk(r)=%d", isOk(r));
            return
            rxOutMailbox.post(r, INFINITE_TIMEOUT);
        };
    }
}

static Result<bool> test_loopback(SerialDevice& sd, size_t packetSize, Timeout rxTimeout, BaseSequentialStream *stream) {
    return
    testBuffer(packetSize) >=
    [&](Buffer&& reference) {
        RxContext rxContext{sd, rxTimeout};
        return
        rxInMailbox.post(rxContext, INFINITE_TIMEOUT) >
        [&] { return sd.write(reference); } >
        [] { return rxOutMailbox.fetch(INFINITE_TIMEOUT); } >=
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
    SerialDevice::open(uartDriver, serialDeviceThreadPriority) >=
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
            } <=
            [&](Error e) {
                chprintf(stream, "FAILURE[%s]\r\n", toString(e));
                return ok(false);
            };
            returnOnFail(passed_, result);
            tm.stop();
            passed = passed_;
        }
        if (passed) {
            chprintf(stream, "SUCCESS %d %d ns\r\n", toNs(tm.tm.best), toNs(tm.tm.worst));
        }
        return ok(false);
    } <= threadErrorReportT<bool>;
}

static void cmd_test_loopback(BaseSequentialStream *stream, int argc, char* argv[]) {
    // TimeMeasurement tm;
    // tm.start();
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
    // tm.stop();
    // chprintf(stream, "elapsed %d ms\r\n", toMs(tm.tm.last));
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
