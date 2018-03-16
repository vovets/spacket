#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"

#include <spacket/serial_device.h>
#include <spacket/util/mailbox.h>
#include <spacket/util/static_thread.h>
#include <spacket/fatal_error.h>
#include <spacket/packetizer.h>

StaticThreadT<256> rxThread;
StaticThreadT<256> txThread;
StaticThreadT<256> packetizerThread;

using BufferMailbox = MailboxT<Buffer, 1>;

BufferMailbox packetizerIn;
BufferMailbox packetizerOut;

class Globals {
public:
    static Result<Globals> init() {
        return
        SerialDevice::open(&UARTD1) >=
        [&](SerialDevice&& sd) {
            return
            Buffer::create(Buffer::maxSize()) >=
            [&](Buffer&& b) {
                return ok(Globals(std::move(sd), std::move(b)));
            };
        };
    }

    SerialDevice& sd() { return sd_; }
    Buffer& scratchBuffer() { return scratchBuffer_; }

private:
    Globals(SerialDevice&& sd, Buffer&& scratchBuffer)
        : sd_(std::move(sd))
        , scratchBuffer_(std::move(scratchBuffer))
    {}

private:
    SerialDevice sd_;
    Buffer scratchBuffer_;
};

Result<boost::blank> writeBuffer(SerialDevice& sd, Buffer&& b) {
    palClearPad(GPIOC, GPIOC_LED);
    return
    sd.write(b) >
    [&]() {
        palSetPad(GPIOC, GPIOC_LED);
        return ok(boost::blank{});
    };
}

static __attribute__((noreturn)) THD_FUNCTION(rxThreadFunction, arg) {
    chRegSetThreadName("rx");
    Globals& g = *static_cast<Globals*>(arg);
    for (;;) {
        Buffer::create(Buffer::maxSize()) >=
        [&](Buffer&& b) {
            return
            g.sd().read(std::move(b), INFINITE_TIMEOUT) >=
            [&](Buffer&& b) {
                return packetizerIn.post(b, INFINITE_TIMEOUT);
            };
        } <=
        [&](Error e) {
            chprintf(&rttStream, "E: %s", toString(e));
            return ok(boost::blank{});
        };
    }
}

static __attribute__((noreturn)) THD_FUNCTION(txThreadFunction, arg) {
    chRegSetThreadName("tx");
    Globals& g = *static_cast<Globals*>(arg);
    for (;;) {
        packetizerOut.fetch(INFINITE_TIMEOUT) >=
        [&](Buffer&& b) {
            return g.sd().write(b, INFINITE_TIMEOUT);
        } <=
        [&](Error e) {
            chprintf(&rttStream, "E: %s: %s\r\n", chRegGetThreadNameX(chThdGetSelfX()), toString(e));
            return ok(boost::blank{});
        };
    }
}    

static __attribute__((noreturn)) THD_FUNCTION(packetizerThreadFunction, arg) {
    chRegSetThreadName("packetizer");
    Globals& g = *static_cast<Globals*>(arg);
    using Packetizer = PacketizerT<Buffer>;
    auto pktz = Packetizer(g.scratchBuffer(), PacketizerNeedSync::Yes);
    for (;;) {
        packetizerIn.fetch(INFINITE_TIMEOUT) >=
        [&](Buffer&& b) {
            for (uint8_t c: b) {
                auto r = pktz.consume(c);
                switch (r) {
                    case Packetizer::Overflow:
                        pktz = Packetizer(g.scratchBuffer(), PacketizerNeedSync::Yes);
                        return fail<boost::blank>(Error::PacketizerOverflow);
                    case Packetizer::Finished:
                        return
                        g.scratchBuffer().prefix(pktz.size()) >=
                        [&](Buffer&& prefix) {
                            return packetizerOut.post(prefix, INFINITE_TIMEOUT);
                        } >
                        [&]() {
                            // ATTN: in real application there may be no need to demand sync here
                            // ira packets may be separated by single zero byte
                            pktz = Packetizer(g.scratchBuffer(), PacketizerNeedSync::Yes);
                            return ok(boost::blank{});
                        };
                    case Packetizer::Continue:
                        ;
                }
            }
            return ok(boost::blank{});
        } <=
        [&](Error e) {
            chprintf(&rttStream, "E: %s: %s\r\n", chRegGetThreadNameX(chThdGetSelfX()), toString(e));
            return ok(boost::blank{});
        };
    }
}

int __attribute__((noreturn)) main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    auto globals = getOkUnsafe(
        Globals::init() <=
        [&](Error e) {
            FATAL_ERROR(toString(e));
            // will never get here
            return fail<Globals>(e);
        });

    txThread.create(NORMALPRIO + 2, txThreadFunction, &globals);
    packetizerThread.create(NORMALPRIO + 1, packetizerThreadFunction, &globals);
    rxThread.create(NORMALPRIO, rxThreadFunction, &globals);

    while (true) {
        port_wait_for_interrupt();
    }
}
