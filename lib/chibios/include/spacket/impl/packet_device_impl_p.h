#pragma once

#include <spacket/result.h>
#include <spacket/serial_device.h>
#include <spacket/impl/packet_decode_fsm.h>

#include <boost/smart_ptr/intrusive_ref_counter.hpp>

namespace packet_device_impl {

#ifdef PACKET_DEVICE_ENABLE_DEBUG_PRINT

IMPLEMENT_DPX_FUNCTIONS
IMPLEMENT_DPB_FUNCTION

#else

IMPLEMENT_DPX_FUNCTIONS_NOP
IMPLEMENT_DPB_FUNCTION_NOP

#endif

} // packet_device_impl

template <typename Buffer>
struct PacketDeviceImpl:
    boost::intrusive_ref_counter<PacketDeviceImpl<Buffer>, boost::thread_unsafe_counter>
{
    using This = PacketDeviceImpl<Buffer>;
    using ThisPtr = boost::intrusive_ptr<This>;
    using SerialDevice = SerialDeviceT<Buffer>;
    using PacketDecodeFSM = PacketDecodeFSMT<Buffer>;
    using Mailbox = MailboxT<Result<Buffer>, 1>;
    using Thread = StaticThreadT<512>;
    
    static constexpr Timeout stopCheckPeriod = std::chrono::seconds(1);

    static Result<ThisPtr> open(SerialDevice&& serialDevice, void* storage, tprio_t threadPriority);

    static void readThreadFunction_(void* instance) {
        static_cast<This*>(instance)->readThreadFunction();
    }

    PacketDeviceImpl(Buffer&& buffer, SerialDevice&& serialDevice, tprio_t threadPriority);
    ~PacketDeviceImpl();

    void readThreadFunction();
    
    Result<boost::blank> packetFinished(Buffer& buffer);

    Result<Buffer> read(Timeout t);

    Result<boost::blank> write(Buffer b);

    PacketDecodeFSM decodeFSM;
    SerialDevice serialDevice;
    Mailbox readMailbox;
    Thread readThread;
    bool stopRequested;
};

template <typename Buffer>
constexpr Timeout PacketDeviceImpl<Buffer>::stopCheckPeriod;

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
    : decodeFSM(
        [&] (Buffer& b) { return this->packetFinished(b); },
        std::move(buffer)
        )
    , serialDevice(std::move(serialDevice))
    , stopRequested(false)
{
    readThread.create(threadPriority, readThreadFunction_, this);
}

template <typename Buffer>
PacketDeviceImpl<Buffer>::~PacketDeviceImpl() {
    stopRequested = true;
    readThread.wait();
}

template <typename Buffer>
void PacketDeviceImpl<Buffer>::readThreadFunction() {
    using namespace packet_device_impl;
    chRegSetThreadName("pd");
    dpl("readThreadFunction: started");
    for (bool stop = false; !stop;) {
        serialDevice.read(stopCheckPeriod) >=
        [&] (Buffer&& b) {
            dpb("read: ", b);
            for (std::size_t i = 0; i < b.size(); ++i) {
                auto r = decodeFSM.consume(b.begin()[i]) <=
                [&] (Error e) {
                    return threadErrorReport(e);
                };
                if (isFail(r)) { return r; }
            }
            return ok(boost::blank());
        } <=
        [&] (Error e) {
            if (e == toError(ErrorCode::SerialDeviceReadTimeout)) {
                stop = stopRequested;
                return ok(boost::blank());
            }
            auto message = fail<Buffer>(e);
            return replace(readMailbox, message);
        };
    }
}

template <typename Buffer>
Result<boost::blank> PacketDeviceImpl<Buffer>::packetFinished(Buffer& buffer) {
    using namespace packet_device_impl;
    dpb("packetFinished: ", buffer);
    return
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& newBuffer) {
        auto message = ok(buffer);
        return
        replace(readMailbox, message) >
        [&] {
            buffer = std::move(newBuffer);
            dpl("packetFinished: replaced");
            return ok(boost::blank());
        };
    };
}

template <typename Buffer>
Result<Buffer> PacketDeviceImpl<Buffer>::read(Timeout t) {
    using namespace packet_device_impl;
    dpl("read");
    return
    readMailbox.fetch(t) >=
    [&] (Result<Buffer>&& r) {
        dpl("read: fetched");
        return std::move(r);
    } <=
    [&] (Error e) {
        if (e == toError(ErrorCode::ChMsgTimeout)) {
            return fail<Buffer>(toError(ErrorCode::PacketDeviceReadTimeout));
        }
        return fail<Buffer>(e);
    };
}

template <typename Buffer>
Result<boost::blank> PacketDeviceImpl<Buffer>::write(Buffer b) {
    return
    cobs::stuffAndDelim(b) >=
    [&](Buffer&& stuffed) {
        return serialDevice.write(stuffed);
    };
}
