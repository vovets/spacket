#pragma once

#include <spacket/result.h>
#include <spacket/packet_device.h>
#include <spacket/impl/packet_device_impl_base.h>

#include <boost/optional.hpp>

#include <thread>


template <typename Buffer, typename LowerLevel>
struct PacketDeviceImpl: PacketDeviceImplBase<Buffer, LowerLevel>
{
    using This = PacketDeviceImpl<Buffer, LowerLevel>;
    using Base = PacketDeviceImplBase<Buffer, LowerLevel>;

    using Base::start;
    using Base::wait;

    PacketDeviceImpl(LowerLevel& serialDevice, Buffer&& buffer);
    ~PacketDeviceImpl();

    Result<boost::blank> reportError(Error e) override;
};

template <typename Buffer, typename LowerLevel>
PacketDeviceImpl<Buffer, LowerLevel>::PacketDeviceImpl(
LowerLevel& serialDevice,
Buffer&& buffer)
    : Base(
        serialDevice,
        std::move(buffer),
        Thread::params())
{
    start();
}

template <typename Buffer, typename LowerLevel>
PacketDeviceImpl<Buffer, LowerLevel>::~PacketDeviceImpl() {
    wait();
}

template <typename Buffer, typename LowerLevel>
 Result<boost::blank> PacketDeviceImpl<Buffer, LowerLevel>::reportError(Error e) {
    return fail<boost::blank>(e);
}
