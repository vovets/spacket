#include <spacket/packetizer.h>

Result<Buffer> readSync(Source s, size_t maxRead, Timeout t) {
    auto f = idFail<Buffer>;
    auto now = Clock::now();
    auto deadline = now + t;
    bool seenZero = false;
    for (;;) {
        rcallv(b, f, s, deadline - now, maxRead);
        uint8_t* bc = b.begin();
        if (!seenZero) {
            // skip non-zeroes
            for (; bc < b.end() && *bc; ++bc) {}
            if (bc == b.end()) { continue; }
            seenZero = true;
        }
        // skip zeroes
        for (; bc < b.end() && !*bc; ++bc) {}
        if (bc == b.end()) { continue; }
        return ok(b.suffix(bc));
    }        
}

// Result<std::tuple<Buffer, Buffer>> readPacket(Source s, Buffer prefix, size_t maxRead, Timeout t) {
//     auto deadline = Clock::now() + t;
//     Buffer packet(Buffer::maxSize());
//     uint8_t* c = packet.begin();

//     enum AppendResult {
//         PacketFinished,
//         PacketTooBig,
//         Continue
//     };
        
//     auto append = [&](Buffer b) {
//                       if (c == packet.begin()) {
//                           // new packet: look for first zero, skip zeroes
//                           // then copy non-zero bytes
//                           uint8_t* bc = b.begin();
//                           // skip non-zeroes
//                           while (bc < b.end() && *bc) {}
//                           if (bc == b.end()) { return std::make_tuple(Continue, Buffer(0)); }
//                           // skip zeroes
//                           while (bc < b.end() && !*bc) {}
//                           if (bc == b.end()) { return std::make_tuple(
//                               if (*bc) {
//                                   *c++ = *bc++;
//                               }
//                               if (c == packet.end()) {
//                                   if (bc == b.end()) {
//                                       return std::make_tuple(Continue, Buffer(0));
//                                   }
//                                   if (*bc == 0) {
//                                       return std::make_tuple(PacketFinished, b.suffix(bc + 1));
//                                   }
//                                   return std::make_tuple(PacketTooBig, b.suffix(bc));
//                               }
//                           }
                          
//                       if (c == packet.end()) {
//                           if (!b.size() || *b.begin() != 0) {
//                               // output packet full and next byte is not zero
//                               return fail<Buffer>(Errors::PacketTooBig);
//                           }
//                           return 
//                       }
//                           uint8_t* bc = b.begin();
//                       while (bc < b.end() && *bc == 0) { ++bc; }
//                       while (bc < b.end() && c < packet.end() && *bc != 0) { *c = *bc; ++bc; ++c}
//                       if (c == packet.end()) {
//                       }
//                   };

//     uint8_t* firstZero =
//         static_cast<uint8_t*>(std::memchr(prefix.begin(), 0, prefix.size()));
//     firstZero = firstZero ? firstZero : prefix.end();
    
//     if (firstZero != prefix.end()) {
//         return ok(std::make_tuple(
//             prefix.prefix(firstZero - prefix.begin()),
//             prefix.suffix(firstZero + 1)));
//     }
    
    
// }
