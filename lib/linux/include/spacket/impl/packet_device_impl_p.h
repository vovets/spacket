#pragma once

#include <spacket/result.h>
#include <spacket/packet_device.h>
#include <spacket/impl/packet_device_impl_base.h>

#include <boost/optional.hpp>

#include <thread>


template <typename LowerLevel>
struct PacketDeviceImpl: PacketDeviceImplBase<LowerLevel>
{
    using This = PacketDeviceImpl<LowerLevel>;
    using Base = PacketDeviceImplBase<LowerLevel>;

    using Base::start;
    using Base::wait;

    PacketDeviceImpl(LowerLevel& serialDevice, Buffer&& buffer);
    ~PacketDeviceImpl();

    Result<Void> reportError(Error e) override;
};

template <typename LowerLevel>
PacketDeviceImpl<LowerLevel>::PacketDeviceImpl(
LowerLevel& serialDevice,
Buffer&& buffer)
    : Base(
        serialDevice,
        std::move(buffer),
        Thread::params())
{
    start();
}

template <typename LowerLevel>
PacketDeviceImpl<LowerLevel>::~PacketDeviceImpl() {
    wait();
}

template <typename LowerLevel>
 Result<Void> PacketDeviceImpl<LowerLevel>::reportError(Error e) {
    return fail(e);
}
