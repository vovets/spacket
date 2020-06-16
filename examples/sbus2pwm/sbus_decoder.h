#pragma once

#include "typedefs.h"


namespace sbus {

static constexpr Error::Source errorSource = 1;

enum class ErrorCode: Error::Code {
    PacketSizeMismatch   = 0,
    PacketHeaderMismatch = 1,
    PacketFooterMismatch = 2
};

Error toError(ErrorCode e) {
    return ::toError(errorSource, toInt(e));
}
    
struct RawPacket {
    std::uint8_t header;
    std::uint8_t data[22];
    std::uint8_t flags;
    std::uint8_t footer;
};

struct Packet {
    using ChannelValue = std::uint16_t;
    static constexpr std::uint8_t channelBits = 11;
    static constexpr ChannelValue channelMax = (1 << channelBits) - 1;
    static constexpr std::uint8_t numChannels = 18;
    ChannelValue channels[18];
    bool frameLost;
    bool failsafe;
};

std::uint16_t decode2(std::uint16_t a, std::uint16_t b, std::uint8_t rshift) {
    return ((a >> rshift) | (b << (8 - rshift))) & Packet::channelMax;
}

std::uint16_t decode3(std::uint16_t a, std::uint16_t b, std::uint16_t c, std::uint8_t rshift) {
    return ((a >> rshift) | (b << (8 - rshift)) | (c << (8 + 8 - rshift))) & Packet::channelMax;
}

std::uint16_t decodeA(const std::uint8_t d[], std::uint8_t rshift) {
    if (8 - rshift < (Packet::channelBits - 8)) {
        return decode2(d[0], d[1], rshift);
    }
    return decode3(d[0], d[1], d[2], rshift);
}

void decode(const std::uint8_t* data, std::uint16_t* channels) {
    channels[0] = decodeA(data, 0);
    channels[1] = decodeA(data + 1, 3);
    channels[2] = decodeA(data + 2, 6);
    channels[3] = decodeA(data + 4, 1);
    channels[4] = decodeA(data + 5, 4);
    channels[5] = decodeA(data + 6, 7);
    channels[6] = decodeA(data + 8, 2);
    channels[7] = decodeA(data + 9, 5);
}

Result<Buffer> decode(const Buffer& buffer) {
    if (buffer.size() != 25) { return { toError(ErrorCode::PacketSizeMismatch) }; }
    const RawPacket* rp = reinterpret_cast<const RawPacket*>(buffer.begin());
    if (rp->header != 0x0f) { return { toError(ErrorCode::PacketHeaderMismatch) }; }
    if (rp->footer != 0x00) { return { toError(ErrorCode::PacketFooterMismatch) }; }
    return
    Buffer::create(sizeof(Packet)) >=
    [&] (Buffer&& b) {
        Packet* p = reinterpret_cast<Packet*>(b.begin());
        const std::uint8_t* data = &rp->data[0];
        std::uint16_t* channels = p->channels;
        decode(data, channels);
        decode(data + 11, channels + 8);
        channels[16] = rp->flags & 0x01 ? Packet::channelMax : 0;
        channels[17] = rp->flags & 0x02 ? Packet::channelMax : 0;
        p->frameLost = rp->flags & 0x04;
        p->failsafe  = rp->flags & 0x08;
        return ok(std::move(b));
    };
}

struct Decoder: Module {
    Result<Void> up(Buffer&& buffer) override {
        cpm::dpl("SbusDecoder::up|");
        return
        ops->upper(*this) >=
        [&] (Module* m) {
            return
            decode(buffer) >=
            [&] (Buffer&& b) {
                return
                ops->deferProc(makeProc(std::move(b), *m, &Module::up));
            };
        };
    }

    Result<Void> down(Buffer&&) override {
        cpm::dpl("SbusDecoder::down|");
        return { ::toError(::ErrorCode::ModulePacketDropped) };
    }
};

} // sbus