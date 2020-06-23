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


struct Loopback: Module {
    using Module::ops;

    Result<Void> up(Buffer&& buffer) override {
        cpm::dpl("Loopback::up|");
        return
        ops->lower(*this) >=
        [&] (Module* m) {
            return ops->deferProc(makeProc(std::move(buffer), *m, &Module::down));
        };
    }

    Result<Void> down(Buffer&&) override {
        cpm::dpl("Loopback::down|");
        return { toError(ErrorCode::ModulePacketDropped) };
    }
};

using Driver_ = UartT<DRIVER_RX_RING_CAPACITY, DRIVER_TX_RING_CAPACITY>;
using Stack = StackT<STACK_IO_RING_CAPACITY, STACK_PROC_RING_CAPACITY>;
using Multistack = MultistackT<Address, 2>;

namespace alloc {

Allocator& defaultAllocator() {
    static DefaultAllocator allocator;
    return allocator;
}

} // alloc

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Allocator::maxSize(): %d\r\n", alloc::defaultAllocator().maxSize());
    chprintf(&rttStream, "sizeof(Buffer): %d\r\n", sizeof(Buffer));
    chprintf(&rttStream, "sizeof(DeferredProc): %d\r\n", sizeof(DeferredProc));

    Driver_ driver(UARTD1);
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

extern "C" void __cxa_pure_virtual() {
    FATAL_ERROR("__cxa_pure_virtual");
}
