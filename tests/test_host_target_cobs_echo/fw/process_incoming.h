#pragma once

#include <boost/optional.hpp>

struct ProcessIncomingResult {
    // this is to keep track of position in incoming buffer between invokations
    size_t inputOffset;
    // there may be 3 outcomes of invokation:
    // none - packetizer consumed incoming buffer but packet is not complete yet,
    // Error - packetizer overflowed and should be replaced,
    // Buffer - packetizer contains complete packet
    boost::optional<Result<Buffer>> packet;
};

template <typename Buffer, typename Packetizer>
ProcessIncomingResult processIncoming(const Buffer& input, size_t inputOffset, Packetizer& p) {
    size_t offset = inputOffset;
    for (; offset < input.size(); ++offset) {
        auto r = p.consume(*(input.begin() + offset));
        switch (r) {
            case Packetizer::Overflow:
                return { offset, fail<Buffer>(Error::PacketizerOverflow) };
            case Packetizer::Finished: {
                return { offset + 1, ok(p.release()) };
            }
            case Packetizer::Continue:
                ;
        }
    }
    return { 0, boost::none };
}
