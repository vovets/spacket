#pragma once

#include <spacket/result.h>
#include <spacket/result_fatal.h>
#include <spacket/packetizer.h>
#include <spacket/mailbox.h>
#include <spacket/buffer_debug.h>

namespace packetizer_thread_function {

#ifdef PACKETIZER_THREAD_FUNCTION_ENABLE_DEBUG_PRINT

IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPX_FUNCTIONS

#else

IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPX_FUNCTIONS_NOP

#endif

} // packetizer_thread_function

template <typename Buffer, typename Mailbox, typename ErrorReportF>
void packetizerThreadFunctionT(Mailbox& in, Mailbox& out, ErrorReportF reportError) {
    using namespace packetizer_thread_function;
    using Packetizer = PacketizerT<Buffer>;
    Packetizer::create(PacketizerNeedSync::Yes) >=
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
                            Packetizer::create(PacketizerNeedSync::Yes) <=
                            fatal<Packetizer> >=
                            [&](Packetizer&& p) {
                                pktz = std::move(p);
                                return fail<boost::blank>(toError(ErrorCode::PacketizerOverflow));
                            } <=
                            [&](Error e) {
                                reportError(e);
                                return ok(boost::blank{});
                            };
                            break;
                        case Packetizer::Finished: {
                            Buffer packet = pktz.release();
                            out.post(packet, IMMEDIATE_TIMEOUT) <=
                            [&](Error e) {
                                reportError(e);
                                return ok(boost::blank{});
                            } >
                            [&]() {
                                // ATTN: in real application there may be no need to demand sync here
                                // because packets may be separated by single zero byte
                                return
                                Packetizer::create(PacketizerNeedSync::Yes) <=
								fatal<Packetizer> >=
                                [&](Packetizer&& p) {
                                    pktz = std::move(p);
                                    return ok(boost::blank{});
                                };
                            };
                            break;
                        }
                        case Packetizer::Continue:
                            ;
                    }
                }
                fetched.release();
                return ok(boost::blank{});
            } <=
            [&](Error e) {
                reportError(e);
                return ok(boost::blank{});
            };
        }
        return ok(boost::blank{});
    } <=
    fatal<boost::blank>;
}
