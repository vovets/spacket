#pragma once

#include "typedefs.h"


namespace sbus {

static constexpr Error::Source errorSource = 1;

enum class ErrorCode: Error::Code {
    PacketSizeMismatch   = 0,
    PacketHeaderMismatch = 1,
    PacketFooterMismatch = 2,
    DestBufferTooSmall   = 3
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

template <std::uint8_t nBytes>
std::uint16_t decodeT(const std::uint8_t d[], std::uint8_t rshift);

template <>
std::uint16_t decodeT<2>(const std::uint8_t d[], std::uint8_t rshift) {
    return decode2(d[0], d[1], rshift);
}

template <>
std::uint16_t decodeT<3>(const std::uint8_t d[], std::uint8_t rshift) {
    return decode3(d[0], d[1], d[2], rshift);
}

void decode(const std::uint8_t* data, std::uint16_t* channels) {
    channels[0] = decodeT<2>(data + 0, 0);
    channels[1] = decodeT<2>(data + 1, 3);
    channels[2] = decodeT<3>(data + 2, 6);
    channels[3] = decodeT<2>(data + 4, 1);
    channels[4] = decodeT<2>(data + 5, 4);
    channels[5] = decodeT<3>(data + 6, 7);
    channels[6] = decodeT<2>(data + 8, 2);
    channels[7] = decodeT<2>(data + 9, 5);
}

Result<Buffer> decode(const Buffer& source, Buffer dest) {
    if (source.size() != 25) { return { toError(ErrorCode::PacketSizeMismatch) }; }
    const RawPacket* rp = reinterpret_cast<const RawPacket*>(source.begin());
    if (rp->header != 0x0f) { return { toError(ErrorCode::PacketHeaderMismatch) }; }
    if (rp->footer != 0x00) { return { toError(ErrorCode::PacketFooterMismatch) }; }
    dest.resize(sizeof(Packet));
    if (dest.size() != sizeof(Packet)) { return toError(ErrorCode::DestBufferTooSmall); }
    Packet* p = reinterpret_cast<Packet*>(dest.begin());
    const std::uint8_t* data = &rp->data[0];
    std::uint16_t* channels = p->channels;
    decode(data, channels);
    decode(data + 11, channels + 8);
    channels[16] = rp->flags & 0x01 ? Packet::channelMax : 0;
    channels[17] = rp->flags & 0x02 ? Packet::channelMax : 0;
    p->frameLost = rp->flags & 0x04;
    p->failsafe  = rp->flags & 0x08;
    return ok(std::move(dest));
}

struct Decoder: Module {
    alloc::Allocator& allocator;

    explicit Decoder(alloc::Allocator& allocator): allocator(allocator) {}
    
    Result<Void> up(Buffer&& buffer) override {
        cpm::dpb("SbusDecoder::up|", &buffer);
        return
        Buffer::create(allocator, sizeof(Packet)) >=
        [&] (Buffer&& dest) {
            return
            decode(buffer, std::move(dest)) >=
            [&] (Buffer&& decoded) {
                return deferUp(std::move(decoded));
            };
        };
    }

    Result<Void> down(Buffer&&) override {
        cpm::dpl("SbusDecoder::down|");
        return { ::toError(::ErrorCode::ModulePacketDropped) };
    }
};

} // sbus
