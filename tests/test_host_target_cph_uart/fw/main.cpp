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
#include <spacket/multistack.h>


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
    using Module::ops;

    Result<Void> up(Buffer&& buffer) override {
        cpm::dpl("LoopbackT::up|");
        return
        ops->lower(*this) >=
        [&] (Module* m) {
            return ops->deferProc(makeProc(std::move(buffer), *m, &Module::down));
        };
    }

    Result<Void> down(Buffer&&) override {
        cpm::dpl("LoopbackT::down|");
        return { toError(ErrorCode::ModulePacketDropped) };
    }
};

using Driver = UartT<Buffer, DRIVER_RX_RING_CAPACITY, DRIVER_TX_RING_CAPACITY>;
using Stack = StackT<Buffer, STACK_IO_RING_CAPACITY, STACK_PROC_RING_CAPACITY>;
using Loopback = LoopbackT<Buffer>;
using Endpoint = EndpointT<Buffer>;
using Address = AddressT<Buffer>;
using Multistack = MultistackT<Buffer, Address, 2>;

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());
    chprintf(&rttStream, "sizeof(Buffer): %d\r\n", sizeof(Buffer));
    chprintf(&rttStream, "sizeof(DeferredProc): %d\r\n", sizeof(DeferredProcT<Buffer>));

    Driver driver(UARTD1);
    Stack stack(driver);
    Loopback loopback;
    Multistack multistack;
    Address addressA{'A'};
    Address addressB{'B'};
    Endpoint endpointA;
    Endpoint endpointB;
    
    
    stack.push(multistack);
    stack.push(loopback);
    multistack.push(addressA, endpointA);
    multistack.push(addressB, endpointB);

    for (;;) {
        stack.tick();
        endpointA.read() >=
        [&] (Buffer&& b) {
            return
            endpointB.write(std::move(b));
        } <=
        [] (Error e) {
            if (e != toError(ErrorCode::Timeout)) {
                return threadErrorReport(e);
            }
            return fail(e);
        };
        endpointB.read() >=
        [&] (Buffer&& b) {
            return endpointA.write(std::move(b));
        } <=
        [] (Error e) {
            if (e != toError(ErrorCode::Timeout)) {
                return threadErrorReport(e);
            }
            return fail(e);
        };
    }
}
