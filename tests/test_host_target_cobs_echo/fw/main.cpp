#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"
#include "constants.h"
#include "process_incoming.h"

#include <spacket/serial_device.h>
#include <spacket/util/mailbox.h>
#include <spacket/util/static_thread.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/result_fatal.h>
#include <spacket/cobs.h>
#include <spacket/buffer_debug.h>
#include <spacket/packetizer.h>

StaticThreadT<256> rxThread;
StaticThreadT<512> txThread;

using BufferMailbox = MailboxT<Buffer, MAILBOX_SIZE>;

BufferMailbox receivedMb;

class Globals {
public:
    static Result<Globals> init() {
        return
        SerialDevice::open(&UARTD1) >=
        [&](SerialDevice&& sd) {
            return ok(Globals(std::move(sd)));
        };
    }

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
    DEBUG_PRINT_BUFFER(b);
    return
    cobs::stuffAndDelim(std::move(b)) >=
    [&](Buffer&& stuffedAndDelimited) {
        DEBUG_PRINT_BUFFER(stuffedAndDelimited);
        return
        writeBuffer(sd, stuffedAndDelimited);
    };
}

static __attribute__((noreturn)) THD_FUNCTION(rxThreadFunction, arg) {
    chRegSetThreadName("rx");
    Globals& g = *static_cast<Globals*>(arg);
    for (;;) {
        debugPrintLine("tick");
        Buffer::create(Buffer::maxSize()) >=
        [&](Buffer&& b) {
            return
            g.sd().read(std::move(b), INFINITE_TIMEOUT) >=
            [&](Buffer&& read) {
                DEBUG_PRINT_BUFFER(read);
                return receivedMb.post(read, IMMEDIATE_TIMEOUT);
            };
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
                DEBUG_PRINT_BUFFER(received);
                // this loop fully processes incoming buffer reporting errors as needed
                for (;;) {
                    result = processIncoming(received, result.inputOffset, packetizer);
                    if (!result.packet) {
                        // buffer consumed without error but packet still is not complete
                        break;
                    }
                    std::move(*result.packet) >=
                    [&](Buffer&& packet) {
                        DEBUG_PRINT_BUFFER(packet);
                        return
                        cobs::unstuff(std::move(packet)) >=
                        [&](Buffer&& unstuffed) {
                            DEBUG_PRINT_BUFFER(unstuffed);
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

int __attribute__((noreturn)) main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    auto globals = getOkUnsafe(Globals::init() <= fatal<Globals>);

    txThread.create(NORMALPRIO, txThreadFunction, &globals);
    rxThread.create(NORMALPRIO + 1, rxThreadFunction, &globals);

    for (;;) {
        port_wait_for_interrupt();
    }
}
