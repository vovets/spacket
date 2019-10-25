#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"
#include "constants.h"
#include "process_incoming.h"

#include <spacket/serial_device.h>
#include <spacket/mailbox.h>
#include <spacket/thread.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/result_fatal.h>
#include <spacket/cobs.h>
#include <spacket/buffer_debug.h>
#include <spacket/packetizer.h>


ThreadStorageT<256> rxThreadStorage;
ThreadStorageT<512> txThreadStorage;

using BufferMailbox = MailboxT<Buffer>;
using SerialDevice = SerialDeviceT<Buffer>;

constexpr tprio_t SD_THREAD_PRIORITY = NORMALPRIO + 2;

BufferMailbox receivedMb;

#ifdef ENABLE_DEBUG_PRINT

IMPLEMENT_DPX_FUNCTIONS
IMPLEMENT_DPB_FUNCTION

#else

IMPLEMENT_DPX_FUNCTIONS_NOP
IMPLEMENT_DPB_FUNCTION_NOP

#endif

class Globals {
public:
    static Result<Globals> init() {
        return
        SerialDevice::open(&UARTD1, SD_THREAD_PRIORITY) >=
        [&](SerialDevice&& sd) {
            return ok(Globals(std::move(sd)));
        };
    }

    Globals(Globals&&) = default;

    SerialDevice& sd() { return sd_; }

private:
    Globals(SerialDevice&& sd)
        : sd_(std::move(sd))
    {}

private:
    SerialDevice sd_;
};

Result<boost::blank> writeBuffer(SerialDevice& sd, const Buffer& b) {
    palClearPad(GPIOC, GPIOC_LED);
    return
    sd.write(b) >
    [&]() {
        palSetPad(GPIOC, GPIOC_LED);
        return ok(boost::blank{});
    };
}

Result<boost::blank> encodeAndSend(SerialDevice& sd, Buffer&& b) {
    DPB(b);
    return
    cobs::stuffAndDelim(std::move(b)) >=
    [&](Buffer&& stuffedAndDelimited) {
        DPB(stuffedAndDelimited);
        return
        writeBuffer(sd, stuffedAndDelimited);
    };
}

static __attribute__((noreturn)) THD_FUNCTION(rxThreadFunction, arg) {
    chRegSetThreadName("rx");
    Globals& g = *static_cast<Globals*>(arg);
    for (;;) {
        debugPrintLine("tick");
        g.sd().read(INFINITE_TIMEOUT) >=
        [&](Buffer&& read) {
            DPB(read);
            return receivedMb.post(read, IMMEDIATE_TIMEOUT);
        } <=
        threadErrorReport;
    }
}

static __attribute__((noreturn)) THD_FUNCTION(txThreadFunction, arg) {
    using Packetizer = PacketizerT<Buffer>;
    chRegSetThreadName("tx");
    Globals& g = *static_cast<Globals*>(arg);
    Packetizer::create(PacketizerNeedSync::Yes) <=
    fatal<Packetizer> >=
    [&](Packetizer&& packetizer) {
        ProcessIncomingResult result{ 0, boost::none };
        for (;;) {
            debugPrintLine("tick");
            receivedMb.fetch(INFINITE_TIMEOUT) >=
            [&](Buffer&& received) {
                DPB(received);
                // this loop fully processes incoming buffer reporting errors as needed
                for (;;) {
                    result = processIncoming(received, result.inputOffset, packetizer);
                    if (!result.packet) {
                        // buffer consumed without error but packet still is not complete
                        break;
                    }
                    std::move(*result.packet) >=
                    [&](Buffer&& packet) {
                        DPB(packet);
                        return
                        cobs::unstuff(std::move(packet)) >=
                        [&](Buffer&& unstuffed) {
                            DPB(unstuffed);
                            return encodeAndSend(g.sd(), std::move(unstuffed));
                        };
                    } <=
                    threadErrorReport >
                    [&]() {
                        return
                        Packetizer::create(PacketizerNeedSync::Yes) <=
                        fatal<Packetizer> >=
                        [&](Packetizer&& p) {
                            packetizer = std::move(p);
                            return ok(boost::blank{});
                        };
                    };
                }
                return ok(boost::blank{});
            } <=
            threadErrorReport;
        }
        return ok(boost::blank{});
    };
    for (;;) {}
}    

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    auto globals = std::move(getOkUnsafe(Globals::init() <= fatal<Globals>));

    Thread::create(Thread::params(txThreadStorage, NORMALPRIO), [&] { txThreadFunction(&globals); });
    Thread::create(Thread::params(rxThreadStorage, NORMALPRIO + 1), [&] { rxThreadFunction(&globals); });

    for (;;) {
        port_wait_for_interrupt();
    }
}
