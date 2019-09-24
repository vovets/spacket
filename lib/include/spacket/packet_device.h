#pragma once

#include <spacket/impl/packet_device_impl.h>

#include <boost/intrusive_ptr.hpp>


template <typename Buffer>
class PacketDeviceT: public packet_device_impl_base::Debug<Buffer> {
public:
    using Impl = PacketDeviceImpl<Buffer>;
    using Dbg = packet_device_impl_base::Debug<Buffer>;
    using Dbg::dpl;
    using Storage = std::aligned_storage_t<sizeof(Impl), alignof(Impl)>;
    using SerialDevice = SerialDeviceT<Buffer>;

public:
    template <typename ...U>
    static Result<PacketDeviceT<Buffer>> open(SerialDevice serialDevice, U&&... u) {
        return
        Impl::open(std::move(serialDevice), std::forward<U>(u)...) >=
        [&] (ImplPtr&& impl) {
            return ok(PacketDeviceT(std::move(impl)));
        };
    }

    PacketDeviceT(const PacketDeviceT&) = delete;
    PacketDeviceT(PacketDeviceT&&) = default;

    ~PacketDeviceT() {
    }

    Result<Buffer> read(Timeout t) {
        return impl->read(t);
    }

    Result<boost::blank> write(Buffer b) {
        return impl->write(std::move(b));
    }

private:
    using ImplPtr = boost::intrusive_ptr<Impl>;

    PacketDeviceT(ImplPtr impl)
        : impl(std::move(impl))
    {
    }

    ImplPtr impl;
};
