#pragma once

#include <spacket/result.h>
#include <spacket/thread.h>
#include <spacket/impl/packet_device_impl_base.h>


template <typename LowerLevel>
struct PacketDeviceImpl: PacketDeviceImplBase<LowerLevel>
{
    using This = PacketDeviceImpl<LowerLevel>;
    using Base = PacketDeviceImplBase<LowerLevel>;
    using Mailbox = MailboxT<Result<Buffer>>;
    using ThreadStorage = ThreadStorageT<400>;

    using Base::start;
    using Base::wait;

    PacketDeviceImpl(LowerLevel& serialDevice, Buffer&& buffer, tprio_t threadPriority);
    ~PacketDeviceImpl();

    Result<Void> reportError(Error e) override;

    ThreadStorage threadStorage;
};

template <typename LowerLevel>
PacketDeviceImpl<LowerLevel>::PacketDeviceImpl(
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

template <typename LowerLevel>
PacketDeviceImpl<LowerLevel>::~PacketDeviceImpl() {
    wait();
}

template <typename LowerLevel>
Result<Void> PacketDeviceImpl<LowerLevel>::reportError(Error e) {
    return threadErrorReport(e);
}
