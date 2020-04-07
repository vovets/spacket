#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"
#include "constants.h"

#include <spacket/cph.h>
#include <spacket/result_fatal.h>
#include <spacket/util/thread_error_report.h>
#include <include/spacket/buffer_debug.h>

namespace {

#ifdef ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
#endif

} // namespace


constexpr Timeout TX_TIMEOUT = std::chrono::milliseconds(20);

template <typename Buffer, std::size_t N_CONN_UP, std::size_t N_CONN_DOWN>
class SerialServiceT: public ExecutorT<Buffer> {
    using Stream = StreamT<Buffer>;
    using Uart = UartT<Buffer>;
    using UartService = UartServiceT<Buffer>;
    using Executor = ExecutorT<Buffer>;
    using ConnArrayUp = typename Executor::template ConnArray<N_CONN_UP>;
    using ConnArrayDown = typename Executor::template ConnArray<N_CONN_DOWN>;

public:
    SerialServiceT(UARTDriver& driver)
        : uart(driver)
        , uartService(uart)
    {
        setupHandlers();
        uart.startRx(createInitialBuffer());
    }

    Result<Buffer> read(Timeout t) {
        
    }

    void loop() {
        uart.waitRx(INFINITE_TIMEOUT) >=
        [&](Buffer&& b) {
            dpl("rx");
            uartService.rxInput = std::move(b);
            return
            Executor::execute(connsUp) >
            [&]() { return Executor::execute(connsDown); } >
            [&]() { return uart.waitTx(TX_TIMEOUT); };
        } <=
        threadErrorReport <=
        [&](Error) {
            chThdSleep(toSystime(std::chrono::milliseconds(1000)));
            return ok();
        };
    }
    
private:
    void setupHandlers() {
        auto inserterUp = makeArrayInserter(connsUp);
        auto inserterDown = makeArrayInserter(connsDown);
        uartService.addHandlers(inserterUp, inserterDown);
    }
    
    Buffer createInitialBuffer() {
        return getOkUnsafe(
            Buffer::create(Buffer::maxSize()) <= fatal<Buffer>);
    }

private:
    ConnArrayUp connsUp;
    ConnArrayDown connsDown;

    Uart uart;
    UartService uartService;
};

using SerialService = SerialServiceT<Buffer, 1, 1>;

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    SerialService ss(UARTD1);

    for (;;) {
        ss.loop();
        if (!!ss.upStream()) {
            ss.downStream() = extract(ss.upStream());
        }
    }
}
