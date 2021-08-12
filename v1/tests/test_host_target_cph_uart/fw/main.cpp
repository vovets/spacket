#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "typedefs.h"
#include "constants.h"

#include <spacket/module.h>
#include <spacket/stack.h>
#include <spacket/buffer_debug.h>
#include <spacket/endpoint.h>
#include <spacket/multistack.h>
#include <spacket/bottom_module.h>
#include <spacket/packetizer_module.h>
#include <spacket/cobs_module.h>
#include <spacket/crc_module.h>


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
    Result<Void> up(Buffer&& buffer) override {
        cpm::dpl("Loopback::up|");
        return deferDown(std::move(buffer));
    }

    Result<Void> down(Buffer&&) override {
        cpm::dpl("Loopback::down|");
        return { toError(ErrorCode::ModulePacketDropped) };
    }
};

using Driver_ = Uart2;
using Multistack = MultistackT<Address, 2>;
using Executor = ExecutorT<1>;

Allocator allocator;
ProcAllocator procAllocator;
Executor executor;
Driver_ driver(allocator, UARTD1);
Stack stack(executor, procAllocator);
BottomModule bottom(driver, procAllocator);
Loopback loopback;
Multistack multistack(procAllocator);
PacketizerModule packetizer(allocator);
CobsModule cobsModule;
CrcModule crcModule;
Address address0{0};
Address address1{1};
Endpoint endpoint0;
Endpoint endpoint1;

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "test_host_target_cph_uart_fw\r\n");
    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Allocator::maxSize(): %d\r\n", allocator.maxSize());
    chprintf(&rttStream, "sizeof(Buffer): %d\r\n", sizeof(Buffer));
    chprintf(&rttStream, "sizeof(DeferredProc): %d\r\n", sizeof(DeferredProc));

    stack.push(bottom);
    stack.push(packetizer);
    stack.push(cobsModule);
    stack.push(crcModule);
    stack.push(multistack);
    stack.push(loopback);
    multistack.push(address0, endpoint0);
    multistack.push(address1, endpoint1);

    driver.setBaud(921600);
    driver.start();

    for (;;) {
        bottom.service(executor) >
        [&] { return endpoint0.read(); } >=
            [&] (Buffer&& b) {
            return
            endpoint1.write(std::move(b));
        } <=
            [] (Error e) {
            if (e == toError(ErrorCode::Timeout)) {
                return ok();
            }
            return fail(e);
        } >
            [&] { return endpoint1.read(); } >=
            [&] (Buffer&& b) {
            return endpoint0.write(std::move(b));
        } <=
        [] (Error e) {
            if (e == toError(ErrorCode::Timeout)) {
                return ok();
            }
            return fail(e);
        } <=
        logError;
    }
}

extern "C" void __cxa_pure_virtual() {
    FATAL_ERROR("__cxa_pure_virtual");
}
