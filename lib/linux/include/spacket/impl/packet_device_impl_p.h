#pragma once

#include <spacket/result.h>
#include <spacket/packet_device.h>
#include <spacket/impl/packet_device_impl_base.h>

#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/optional.hpp>

#include <thread>


template <typename Buffer, typename LowerLevel>
struct PacketDeviceImpl: PacketDeviceImplBase<Buffer, LowerLevel>,
    boost::intrusive_ref_counter<PacketDeviceImpl<Buffer, LowerLevel>>
{
    using This = PacketDeviceImpl<Buffer, LowerLevel>;
    using Base = PacketDeviceImplBase<Buffer, LowerLevel>;
    using ThisPtr = boost::intrusive_ptr<This>;

    static Result<ThisPtr> open(LowerLevel&& serialDevice);

    PacketDeviceImpl(Buffer&& buffer, LowerLevel&& serialDevice);

    Result<boost::blank> reportError(Error e) override;
};

template <typename Buffer, typename LowerLevel>
Result<boost::intrusive_ptr<PacketDeviceImpl<Buffer, LowerLevel>>> PacketDeviceImpl<Buffer, LowerLevel>::open(LowerLevel&& serialDevice) {
    return
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& buffer) {
        return ok(
            ThisPtr(
                new This(
                    std::move(buffer),
                    std::move(serialDevice))));
    };
}

template <typename Buffer, typename LowerLevel>
PacketDeviceImpl<Buffer, LowerLevel>::PacketDeviceImpl(
Buffer&& buffer,
LowerLevel&& serialDevice)
    : Base(
        std::move(buffer),
        std::move(serialDevice),
        Thread::params())
{
}

template <typename Buffer, typename LowerLevel>
 Result<boost::blank> PacketDeviceImpl<Buffer, LowerLevel>::reportError(Error e) {
    return fail<boost::blank>(e);
}
