#pragma once

#include <spacket/result.h>
#include <spacket/serial_device.h>
#include <spacket/impl/packet_device_impl_base.h>

#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/optional.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>

namespace packet_device_impl {

template <typename Message>
struct Mailbox {
    Result<boost::blank> post(Message& message_) {
        std::lock_guard<std::mutex> lock(mutex);
        message = std::move(message_);
        full.notify_one();
        return ok(boost::blank());
    }
    
    Result<Message> fetch(Timeout timeout) {
        std::unique_lock<std::mutex> lock(mutex);
        if (full.wait_for(lock, timeout, [&] { return !!message; })) {
            return ok(std::move(message.value()));
        }
        return fail<Message>(toError(ErrorCode::Timeout));
    }

    boost::optional<Message> message;
    std::mutex mutex;
    std::condition_variable full;
};

} // packet_device_impl

template <typename Buffer>
struct PacketDeviceImpl: PacketDeviceImplBase<Buffer>,
    boost::intrusive_ref_counter<PacketDeviceImpl<Buffer>, boost::thread_unsafe_counter>
{
    using This = PacketDeviceImpl<Buffer>;
    using Base = PacketDeviceImplBase<Buffer>;
    using ThisPtr = boost::intrusive_ptr<This>;
    using SerialDevice = SerialDeviceT<Buffer>;
    using Mailbox = packet_device_impl::Mailbox<Result<Buffer>>;
    
    static Result<ThisPtr> open(SerialDevice&& serialDevice);

    static void readThreadFunction_(void* instance) {
        static_cast<This*>(instance)->readThreadFunction();
    }

    PacketDeviceImpl(Buffer&& buffer, SerialDevice&& serialDevice);
    virtual ~PacketDeviceImpl();

    Result<boost::blank> mailboxReplace(Result<Buffer>& message) override;
    Result<Result<Buffer>> mailboxFetch(Timeout t) override;
    Result<boost::blank> reportError(Error e) override;

    Mailbox readMailbox;
    std::thread readThread;
};

template <typename Buffer>
Result<boost::intrusive_ptr<PacketDeviceImpl<Buffer>>> PacketDeviceImpl<Buffer>::open(SerialDevice&& serialDevice) {
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

template <typename Buffer>
PacketDeviceImpl<Buffer>::PacketDeviceImpl(
    Buffer&& buffer,
    SerialDevice&& serialDevice)
    : Base(
        std::move(buffer),
        std::move(serialDevice))
{
    readThread = std::thread(readThreadFunction_, this);
}

template <typename Buffer>
PacketDeviceImpl<Buffer>::~PacketDeviceImpl() {
    Base::requestStop();
    readThread.join();
}

template <typename Buffer>
Result<boost::blank> PacketDeviceImpl<Buffer>::mailboxReplace(Result<Buffer>& message) {
    return readMailbox.post(message);
}

template <typename Buffer>
Result<Result<Buffer>> PacketDeviceImpl<Buffer>::mailboxFetch(Timeout t) {
    return
    readMailbox.fetch(t);
}

template <typename Buffer>
 Result<boost::blank> PacketDeviceImpl<Buffer>::reportError(Error e) {
    return fail<boost::blank>(e);
}
