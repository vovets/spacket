#pragma once

#include <spacket/packetizer.h>

namespace impl {

inline
Result<ReadResult> readPacket(
    alloc::Allocator& allocator,
    Source source,
    Buffer prefix,
    size_t maxRead,
    PacketizerNeedSync needSync,
    Timeout t)
{
    using SType = ReadResult;

    auto deadline = Clock::now() + t;
    returnOnFailT(packet, SType, Buffer::create(allocator));
    auto lastResult = Packetizer::Continue;
    returnOnFailT(pktz, SType, Packetizer::create(allocator, needSync));

    for (uint8_t* ptr = prefix.begin(); ptr < prefix.end(); ++ptr) {
        auto r = pktz.consume(*ptr);
        switch (r) {
            case Packetizer::Overflow:
                return fail<SType>(toError(ErrorCode::PacketizerOverflow));
            case Packetizer::Finished: {
                Buffer packet = pktz.release();
                return
                prefix.suffix(++ptr) >=
                [&](Buffer&& suffix) {
                    return ok(SType{std::move(packet), std::move(suffix)});
                };
            }
            case Packetizer::Continue:
                ;
            }
    }
    
    for (;;) {
        auto now = Clock::now();
        if (now >= deadline) {
            return fail<SType>(toError(ErrorCode::Timeout));
        }
        returnOnFailT(b, SType, source(deadline - now, maxRead));
        for (uint8_t* ptr = b.begin(); ptr < b.end(); ++ptr) {
            auto r = pktz.consume(*ptr);
            switch (r) {
                case Packetizer::Overflow:
                    return fail<SType>(toError(ErrorCode::PacketizerOverflow));
                case Packetizer::Finished: {
                    Buffer packet = pktz.release();
                    return
                    b.suffix(++ptr) >=
                    [&](Buffer&& suffix) {
                        return ok(SType{std::move(packet), std::move(suffix)});
                    };
                }
                case Packetizer::Continue:
                    ;
            }
        }
    }
}

}

// this function skips until synced
inline
Result<ReadResult> readPacket(
    alloc::Allocator& allocator,
    Source s,
    size_t maxRead,
    Timeout t) {
    return
    Buffer::create(allocator, 0) >=
    [&](Buffer&& prefix) {
        return impl::readPacket(
            allocator,
            std::move(s),
            std::move(prefix),
            maxRead,
            PacketizerNeedSync::Yes,
            t);
    };
}        
        

// this function assumes that stream is synced (as presense of "prefix" suggests)
Result<ReadResult> readPacket(
    alloc::Allocator& allocator,
    Source s,
    Buffer prefix,
    size_t maxRead,
    Timeout t) {
    return
    impl::readPacket(
        allocator,
        std::move(s),
        std::move(prefix),
        maxRead,
        PacketizerNeedSync::No,
        t);
}        
