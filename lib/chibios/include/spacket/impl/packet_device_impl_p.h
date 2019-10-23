#pragma once

#include <spacket/result.h>
#include <spacket/serial_device.h>
#include <spacket/thread.h>
#include <spacket/impl/packet_device_impl_base.h>

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

template <typename Buffer>
struct PacketDeviceImpl: PacketDeviceImplBase<Buffer>,
    boost::intrusive_ref_counter<PacketDeviceImpl<Buffer>, boost::thread_unsafe_counter>
{
    using This = PacketDeviceImpl<Buffer>;
    using Base = PacketDeviceImplBase<Buffer>;
    using ThisPtr = boost::intrusive_ptr<This>;
    using SerialDevice = SerialDeviceT<Buffer>;
    using Mailbox = MailboxT<Result<Buffer>>;
    using ThreadStorage = ThreadStorageT<512>;
    
    static Result<ThisPtr> open(SerialDevice&& serialDevice, void* storage, tprio_t threadPriority);

    static void readThreadFunction_(void* instance) {
        chRegSetThreadName("pd");
        static_cast<This*>(instance)->readThreadFunction();
    }

    PacketDeviceImpl(Buffer&& buffer, SerialDevice&& serialDevice, tprio_t threadPriority);
    virtual ~PacketDeviceImpl();

    Result<boost::blank> reportError(Error e) override;

    ThreadStorage readThreadStorage;
    Thread readThread;
};

template <typename Buffer>
Result<boost::intrusive_ptr<PacketDeviceImpl<Buffer>>> PacketDeviceImpl<Buffer>::open(SerialDevice&& serialDevice, void* storage, tprio_t threadPriority) {
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

template <typename Buffer>
PacketDeviceImpl<Buffer>::PacketDeviceImpl(
    Buffer&& buffer,
    SerialDevice&& serialDevice,
    tprio_t threadPriority)
    : Base(
        std::move(buffer),
        std::move(serialDevice))
{
    Thread::create(readThreadStorage, threadPriority, [&] { this->readThreadFunction(); });
}

template <typename Buffer>
PacketDeviceImpl<Buffer>::~PacketDeviceImpl() {
    Base::requestStop();
    readThread.wait();
}

template <typename Buffer>
Result<boost::blank> PacketDeviceImpl<Buffer>::reportError(Error e) {
    return threadErrorReport(e);
}
