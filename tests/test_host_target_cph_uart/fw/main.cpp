#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"
#include "constants.h"

#include <spacket/cpm.h>
#include <spacket/uart.h>
#include <spacket/result_fatal.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/buffer_debug.h>

namespace {

#ifdef ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
#endif

} // namespace

constexpr Timeout TX_TIMEOUT = std::chrono::milliseconds(20);


template <typename Buffer>
struct UartServiceT: ModuleT<Buffer> {
    using Module = ModuleT<Buffer>;
    using Uart = UartT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Module::upper;
    using Module::lower;

    Uart& uart;

    UartServiceT(Uart& uart): uart(uart) {}

    Result<DeferredProc> up(Buffer&& buffer) override {
        cpm::dpl("UartServiceT::up|enter");
        return
        Buffer::create(Buffer::maxSize()) >=
        [&](Buffer&& newBuffer) {
            uart.startRx(std::move(newBuffer));
            cpm::dpl("UartServiceT::up|startRx");
            return
            upper(*this) >=
            [&] (Module* m) {
                return ok(DeferredProc(std::move(buffer), *m, &Module::up));
            };
        };
    }

    Result<DeferredProc> down(Buffer&& buffer) override {
        cpm::dpb("UartServiceT::down|", &buffer);
        uart.startTx(std::move(buffer));
        cpm::dpl("UartServiceT::down|startTx");
        return fail<DeferredProc>(toError(ErrorCode::ModulePacketConsumed));
    }
};

template <typename Buffer>
class SerialServiceT {
    using Uart = UartT<Buffer>;
    using UartService = UartServiceT<Buffer>;
    using ModuleList = ModuleListT<Buffer>;
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;

public:
    SerialServiceT(UARTDriver& driver)
        : uart(driver)
        , uartService(uart)
    {
        push(uartService);
        uart.startRx(createInitialBuffer());
    }

    void push(Module& m) {
        m.moduleList = &moduleList;
        moduleList.push_back(m);
    }

    void loop() {
        uart.waitRx(INFINITE_TIMEOUT) >=
        [&](Buffer&& b) {
            dpl("SerialServiceT::loop|rx");
            serve(std::move(b));
            return ok();
        } <=
        threadErrorReport <=
        [&](Error) {
            chThdSleep(toSystime(std::chrono::milliseconds(1000)));
            return ok();
        };
    }

    void serve(Buffer&& b) {
        if (moduleList.empty()) { return; }
        Result<DeferredProc> r = ok(
            DeferredProc(
                std::move(b),
                *moduleList.begin(),
                &Module::up
            ));
        while (isOk(r)) {
            r = getOkUnsafe(std::move(r))();
        }
        std::move(r) <=
        [&] (Error e) {
            if (e != toError(ErrorCode::ModulePacketConsumed)) {
                handleError(e);
            }
            return fail<DeferredProc>(e);
        };
    }

private:
    Buffer createInitialBuffer() {
        return getOkUnsafe(
            Buffer::create(Buffer::maxSize()) <= fatal<Buffer>);
    }

    void handleError(Error e) {
        threadErrorReport(e);
    }

private:
    ModuleList moduleList;

    Uart uart;
    UartService uartService;
};

template <typename Buffer>
struct LoopbackT: ModuleT<Buffer> {
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Module::upper;
    using Module::lower;

    Result<DeferredProc> up(Buffer&& buffer) override {
        cpm::dpl("LoopbackT::up|enter");
        return
        lower(*this) >=
        [&] (Module* m) {
            return ok(DeferredProc(std::move(buffer), *m, &Module::down));
        };
    }

    Result<DeferredProc> down(Buffer&&) override {
        cpm::dpl("LoopbackT::down|enter");
        return fail<DeferredProc>(toError(ErrorCode::ModulePacketDropped));
    }
};

using SerialService = SerialServiceT<Buffer>;
using Loopback = LoopbackT<Buffer>;

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    SerialService ss(UARTD1);
    Loopback loopback;
    ss.push(loopback);

    for (;;) {
        ss.loop();
    }
}
