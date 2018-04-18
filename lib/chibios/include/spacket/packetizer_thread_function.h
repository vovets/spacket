#pragma once

#include <spacket/result.h>
#include <spacket/result_fatal.h>
#include <spacket/packetizer.h>
#include <spacket/util/mailbox.h>
#include <spacket/buffer_debug.h>

template <typename Buffer, typename Mailbox, typename ErrorReportF>
void packetizerThreadFunctionT(Mailbox& in, Mailbox& out, ErrorReportF reportError) {
    using Packetizer = PacketizerT<Buffer>;
    Packetizer::create(PacketizerNeedSync::Yes) >=
    [&](Packetizer pktz) {
        for (;;) {
            debugPrintLine("tick");
            in.fetch(INFINITE_TIMEOUT) >=
            [&](Buffer&& fetched) {
                DEBUG_PRINT_BUFFER(fetched);
                for (uint8_t c: fetched) {
                    auto r = pktz.consume(c);
                    switch (r) {
                        case Packetizer::Overflow:
                            Packetizer::create(PacketizerNeedSync::Yes) <=
                            fatal<Packetizer> >=
                            [&](Packetizer&& p) {
                                pktz = std::move(p);
                                return fail<boost::blank>(Error::PacketizerOverflow);
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
