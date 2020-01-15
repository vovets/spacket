#pragma once

#include <spacket/impl/packet_device_impl.h>


template <typename Buffer, typename LowerLevel>
class PacketDeviceT: private PacketDeviceImpl<Buffer, LowerLevel> {
public:
    using Impl = PacketDeviceImpl<Buffer, LowerLevel>;
    using Dbg = packet_device_impl_base::Debug<Buffer>;
    using Dbg::dpl;

    static constexpr std::size_t maxSize() { return LowerLevel::maxSize() - 4 /*crc*/ - 1 /*cobs*/ - 2 /*delim*/; }

public:
    template <typename ...U>
    PacketDeviceT(LowerLevel& lowerLevel, U&&... u)
    : Impl(lowerLevel, std::forward<U>(u)...) {
    }

    PacketDeviceT(const PacketDeviceT&) = delete;
    PacketDeviceT(PacketDeviceT&&) = delete;

    Result<Buffer> read(Timeout t) {
        return Impl::read(t);
    }

    Result<Void> write(Buffer b) {
        return Impl::write(std::move(b));
    }
};
