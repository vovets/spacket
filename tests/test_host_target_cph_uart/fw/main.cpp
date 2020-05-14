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

using Driver = UartT<Buffer, POOL_NUM_BUFFERS + 1>;
using Stack = StackT<Buffer>;
using Loopback = LoopbackT<Buffer>;

int main(void) {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());

    Driver driver(UARTD1);
    Stack stack(driver);
    Loopback loopback;
    stack.push(loopback);

    for (;;) {
        stack.tick();
    }
}
