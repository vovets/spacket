#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "buffer.h"
#include "constants.h"

#include <spacket/module.h>
#include <spacket/uart.h>
#include <spacket/stack.h>
#include <spacket/buffer_debug.h>
#include <spacket/endpoint.h>


namespace {

#ifdef ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
//IMPLEMENT_DPB_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
//IMPLEMENT_DPB_FUNCTION_NOP
#endif

} // namespace


template <typename Buffer>
struct LoopbackT: ModuleT<Buffer> {
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Module::ops;

    Result<DeferredProc> up(Buffer&& buffer) override {
        cpm::dpl("LoopbackT::up|enter");
        return
        ops->lower(*this) >=
        [&] (Module* m) {
            return defer(std::move(buffer), *m, &Module::down);
        };
    }

    Result<DeferredProc> down(Buffer&&) override {
        cpm::dpl("LoopbackT::down|enter");
        return fail<DeferredProc>(toError(ErrorCode::ModulePacketDropped));
    }
};

using Driver = UartT<Buffer, DRIVER_RX_RING_CAPACITY, DRIVER_TX_RING_CAPACITY>;
using Stack = StackT<Buffer, STACK_RING_CAPACITY>;
using Loopback = LoopbackT<Buffer>;
// using EndpointService = EndpointServiceT<Buffer>;
// using EndpointHandle = EndpointHandleT<Buffer, EndpointService>;
using Endpoint = EndpointT<Buffer>;

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    Driver driver(UARTD1);
    Stack stack(driver);
    // Loopback loopback;
    Endpoint endpoint;
    
    
    // stack.push(loopback);
    stack.push(endpoint);

    for (;;) {
        stack.tick();
        endpoint.read() >=
        [&] (Buffer&& b) {
            return
            endpoint.write(std::move(b));
        } <=
        [] (Error e) {
            if (e != toError(ErrorCode::Timeout)) {
                return threadErrorReport(e);
            }
            return fail(e);
        };
    }
}
