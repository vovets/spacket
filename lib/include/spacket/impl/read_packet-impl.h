#pragma once

#include <spacket/packetizer.h>

template<typename Buffer>
Result<ReadResult<Buffer>> readPacket(
    Source<Buffer> s,
    Buffer next,
    size_t maxRead,
    Timeout t)
{
    return readPacket<Buffer>(
        std::move(s), std::move(next), maxRead, Buffer::maxSize(), t);
}

template<typename Buffer>
Result<ReadResult<Buffer>> readPacket(
    Source<Buffer> s,
    Buffer next,
    size_t maxRead,
    size_t maxPacketSize,
    Timeout t)
{
    using SType = ReadResult<Buffer>;
    using Packetizer = PacketizerT<Buffer>;
    auto deadline = Clock::now() + t;
    returnOnFailT(packet, SType, Buffer::create(maxPacketSize));
    auto lastResult = Packetizer::Continue;
    Packetizer pktz(packet);

    for (uint8_t* ptr = next.begin(); ptr < next.end(); ++ptr) {
        auto r = pktz.consume(*ptr);
        switch (r) {
            case Packetizer::Overflow:
                return fail<SType>(Error::PacketTooBig);
            case Packetizer::Finished:
                return
                packet.prefix(pktz.size()) >=
                [&](Buffer&& prefix) {
                    return
                    next.suffix(++ptr) >=
                    [&](Buffer&& suffix) {
                        return ok(SType{std::move(prefix), std::move(suffix)});
                    };
                };
            case Packetizer::Continue:
                ;
            }
    }
    
    for (;;) {
        auto now = Clock::now();
        if (now >= deadline) {
            return fail<SType>(Error::Timeout);
        }
        returnOnFailT(b, SType, s(deadline - now, maxRead));
        for (uint8_t* ptr = b.begin(); ptr < b.end(); ++ptr) {
            auto r = pktz.consume(*ptr);
            switch (r) {
                case Packetizer::Overflow:
                    return fail<SType>(Error::PacketTooBig);
                case Packetizer::Finished:
                    return
                    packet.prefix(pktz.size()) >=
                    [&](Buffer&& prefix) {
                        return
                        b.suffix(++ptr) >=
                        [&](Buffer&& suffix) {
                            return ok(SType{std::move(prefix), std::move(suffix)});
                        };
                    };
                case Packetizer::Continue:
                    ;
            }
        }
    }
}
