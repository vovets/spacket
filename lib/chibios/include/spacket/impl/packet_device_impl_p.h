#pragma once

#include <spacket/result.h>
#include <spacket/thread.h>
#include <spacket/impl/packet_device_impl_base.h>


template <typename Buffer, typename LowerLevel>
struct PacketDeviceImpl: PacketDeviceImplBase<Buffer, LowerLevel>
{
    using This = PacketDeviceImpl<Buffer, LowerLevel>;
    using Base = PacketDeviceImplBase<Buffer, LowerLevel>;
    using Mailbox = MailboxT<Result<Buffer>>;
    using ThreadStorage = ThreadStorageT<400>;

    using Base::start;
    using Base::wait;

    PacketDeviceImpl(LowerLevel& serialDevice, Buffer&& buffer, tprio_t threadPriority);
    ~PacketDeviceImpl();

    Result<boost::blank> reportError(Error e) override;

    ThreadStorage threadStorage;
};

template <typename Buffer, typename LowerLevel>
PacketDeviceImpl<Buffer, LowerLevel>::PacketDeviceImpl(
LowerLevel& serialDevice,
Buffer&& buffer,
tprio_t threadPriority)
    : Base(
        serialDevice,
        std::move(buffer),
        Thread::params(threadStorage, threadPriority))
{
    start();
}

template <typename Buffer, typename LowerLevel>
PacketDeviceImpl<Buffer, LowerLevel>::~PacketDeviceImpl() {
    wait();
}

template <typename Buffer, typename LowerLevel>
Result<boost::blank> PacketDeviceImpl<Buffer, LowerLevel>::reportError(Error e) {
    return threadErrorReport(e);
}
