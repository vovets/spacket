#pragma once

#include <spacket/result.h>
#include <spacket/thread.h>
#include <spacket/impl/packet_device_impl_base.h>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

namespace packet_device_impl {

#ifdef PACKET_DEVICE_ENABLE_DEBUG_PRINT

IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPB_FUNCTION

#else

IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPB_FUNCTION_NOP

#endif

} // packet_device_impl

template <typename Buffer, typename LowerLevel>
struct PacketDeviceImpl: PacketDeviceImplBase<Buffer, LowerLevel>,
    boost::intrusive_ref_counter<PacketDeviceImpl<Buffer, LowerLevel>, boost::thread_unsafe_counter>
{
    using This = PacketDeviceImpl<Buffer, LowerLevel>;
    using Base = PacketDeviceImplBase<Buffer, LowerLevel>;
    using ThisPtr = boost::intrusive_ptr<This>;
    using Mailbox = MailboxT<Result<Buffer>>;
    using ThreadStorage = ThreadStorageT<400>;

    using Base::start;
    using Base::wait;

    static Result<ThisPtr> open(LowerLevel&& serialDevice, void* storage, tprio_t threadPriority);

    PacketDeviceImpl(Buffer&& buffer, LowerLevel&& serialDevice, tprio_t threadPriority);
    ~PacketDeviceImpl();

    Result<boost::blank> reportError(Error e) override;

    void operator delete(void* ) {}

    ThreadStorage threadStorage;
};

template <typename Buffer, typename LowerLevel>
Result<boost::intrusive_ptr<PacketDeviceImpl<Buffer, LowerLevel>>>
PacketDeviceImpl<Buffer, LowerLevel>::open(LowerLevel&& serialDevice, void* storage, tprio_t threadPriority) {
    return
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& buffer) {
        return ok(
            ThisPtr(
                new (storage) This(
                    std::move(buffer),
                    std::move(serialDevice),
                    threadPriority)));
    };
}

template <typename Buffer, typename LowerLevel>
PacketDeviceImpl<Buffer, LowerLevel>::PacketDeviceImpl(
Buffer&& buffer,
LowerLevel&& serialDevice,
tprio_t threadPriority)
    : Base(
        std::move(buffer),
        std::move(serialDevice),
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
