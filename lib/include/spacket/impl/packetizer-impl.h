#pragma once

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
    auto f = idFail<ReadResult<Buffer>>;
    auto deadline = Clock::now() + t;
    Buffer packet(maxPacketSize);
    uint8_t* c = packet.begin();

    enum AppendResult {
        Finished,
        TooBig,
        Continue,
    };
        
    auto skipZeroes = [&](Buffer b) {
                           uint8_t* bc = b.begin();
                           for (; bc < b.end() && *bc == 0; ++bc) {}
                           if (bc == b.end()) {
                               return std::make_pair(Continue, Buffer(0));
                           }
                           return std::make_pair(Finished, b.suffix(bc));
                       };

    auto append = [&](Buffer b) {
                           uint8_t* bc = b.begin();
                           while (bc < b.end()
                               && c < packet.end()
                               && *bc != 0) {
                               *c++ = *bc++;
                           }
                           if (c == packet.end()) {
                               if (bc < b.end()) {
                                   if (*bc == 0) {
                                       return
                                           std::make_pair(Finished, b.suffix(bc + 1));
                                   } else {
                                       return
                                           std::make_pair(TooBig, b.suffix(bc + 1));
                                   }
                               } else {
                                   return std::make_pair(Continue, Buffer(0));
                               }
                           } else {
                               if (bc < b.end()) {
                                   return std::make_pair(Finished, b.suffix(bc + 1));
                               } else {
                                   return std::make_pair(Continue, Buffer(0));
                               }
                           }
                       };
    {
        auto r = skipZeroes(std::move(next));
        for (;;) {
            if (r.first == Finished) {
                next = std::move(r.second);
                break;
            }
            rcallv(b, f, s(deadline - Clock::now(), maxRead));
            r = skipZeroes(std::move(b));
        }
    }

    {
        auto r = append(std::move(next));
        for (;;) {
            if (r.first == Finished) {
                return ok(ReadResult<Buffer>{packet.prefix(c - packet.begin()), std::move(r.second)});
            }
            if (r.first == TooBig) {
                return fail<ReadResult<Buffer>>(Error::PacketTooBig);
            }
            rcallv(b, f, s(deadline - Clock::now(), maxRead));
            r = append(std::move(b));
        }
    }
}
