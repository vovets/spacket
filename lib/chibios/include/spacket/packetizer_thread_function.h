#pragma once

#include <spacket/result.h>
#include <spacket/result_fatal.h>
#include <spacket/packetizer.h>
#include <spacket/mailbox.h>
#include <spacket/buffer_debug.h>

namespace packetizer_thread_function {

#define PREFIX()
#ifdef PACKETIZER_THREAD_FUNCTION_ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPX_FUNCTIONS
IMPLEMENT_DPB_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPX_FUNCTIONS_NOP
IMPLEMENT_DPB_FUNCTION_NOP
#endif
#undef PREFIX

} // packetizer_thread_function

template <typename Mailbox, typename ErrorReportF>
void packetizerThreadFunctionT(alloc::Allocator& allocator, Mailbox& in, Mailbox& out, ErrorReportF reportError) {
    using namespace packetizer_thread_function;
    Packetizer::create(allocator, PacketizerNeedSync::Yes) >=
    [&](Packetizer pktz) {
        for (;;) {
            dpl("tick");
            in.fetch(INFINITE_TIMEOUT) >=
            [&](Buffer&& fetched) {
                DPB(fetched);
                for (uint8_t c: fetched) {
                    auto r = pktz.consume(c);
                    switch (r) {
                        case Packetizer::Overflow:
                            Packetizer::create(allocator, PacketizerNeedSync::Yes) <=
                            fatal<Packetizer> >=
                            [&](Packetizer&& p) {
                                pktz = std::move(p);
                                return fail(toError(ErrorCode::PacketizerOverflow));
                            } <=
                            [&](Error e) {
                                reportError(e);
                                return ok();
                            };
                            break;
                        case Packetizer::Finished: {
                            Buffer packet = pktz.release();
                            out.post(packet, IMMEDIATE_TIMEOUT) <=
                            [&](Error e) {
                                reportError(e);
                                return ok();
                            } >
                            [&]() {
                                // ATTN: in real application there may be no need to demand sync here
                                // because packets may be separated by single zero byte
                                return
                                Packetizer::create(allocator, PacketizerNeedSync::Yes) <=
								fatal<Packetizer> >=
                                [&](Packetizer&& p) {
                                    pktz = std::move(p);
                                    return ok();
                                };
                            };
                            break;
                        }
                        case Packetizer::Continue:
                            ;
                    }
                }
                fetched.release();
                return ok();
            } <=
            [&](Error e) {
                reportError(e);
                return ok();
            };
        }
        return ok();
    } <=
    fatal<Void>;
}
