#pragma once

#include <spacket/result.h>
#include <spacket/debug_print.h>
#include <spacket/buffer_debug.h>
#include <spacket/mailbox.h>
#include <spacket/thread.h>
#include <spacket/impl/packet_decode_fsm.h>

namespace packet_device_impl_base {

template <typename Buffer>
struct Debug {

#ifdef PACKET_DEVICE_ENABLE_DEBUG_PRINT

IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPX_FUNCTIONS
IMPLEMENT_DPB_FUNCTION

#else

IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPX_FUNCTIONS_NOP
IMPLEMENT_DPB_FUNCTION_NOP

#endif

};

} // packet_device_impl_base

template <typename Buffer>
class PacketDeviceImplBase: public packet_device_impl_base::Debug<Buffer>
{
    using This = PacketDeviceImplBase<Buffer>;
    using Dbg = packet_device_impl_base::Debug<Buffer>;
    using SerialDevice = SerialDeviceT<Buffer>;
    using PacketDecodeFSM = PacketDecodeFSMT<Buffer>;
    using Mailbox = MailboxImplT<Result<Buffer>>;

    static constexpr Timeout stopCheckPeriod = std::chrono::milliseconds(10);

public:
    Result<Buffer> read(Timeout t);

    Result<boost::blank> write(Buffer b);

protected:
    using Dbg::dpl;
    using Dbg::dpb;

    PacketDeviceImplBase(Buffer&& buffer, SerialDevice&& serialDevice, ThreadParams p);
    virtual ~PacketDeviceImplBase();

    PacketDeviceImplBase(PacketDeviceImplBase&&) = delete;
    PacketDeviceImplBase(const PacketDeviceImplBase&) = delete;

    void readThreadFunction();

    void requestStop() { readThread.requestStop(); }

private:
    Result<boost::blank> packetFinished(Buffer& buffer);
    virtual Result<boost::blank> reportError(Error e) = 0;
    
private:
    PacketDecodeFSM decodeFSM;
    SerialDevice serialDevice;
    Mailbox readMailbox;
    Thread readThread;
};

template <typename Buffer>
constexpr Timeout PacketDeviceImplBase<Buffer>::stopCheckPeriod;

template <typename Buffer>
PacketDeviceImplBase<Buffer>::PacketDeviceImplBase(
    Buffer&& buffer,
    SerialDevice&& serialDevice,
    ThreadParams p)
    : decodeFSM(
        [&] (Buffer& b) { return this->packetFinished(b); },
        std::move(buffer)
        )
    , serialDevice(std::move(serialDevice))
    , readThread(
        Thread::create(p, [&] { this->readThreadFunction(); }))
{
}

template <typename Buffer>
PacketDeviceImplBase<Buffer>::~PacketDeviceImplBase() {
    readThread.requestStop();
    readThread.wait();
}


template <typename Buffer>
void PacketDeviceImplBase<Buffer>::readThreadFunction() {
    dpl("readThreadFunction: started");
    while (!Thread::shouldStop()) {
        serialDevice.read(stopCheckPeriod) >=
        [&] (Buffer&& b) {
            dpb("readThreadFunction: read: ", b);
            for (std::size_t i = 0; i < b.size(); ++i) {
                auto r = decodeFSM.consume(b.begin()[i]) <=
                [&] (Error e) {
                    return reportError(e);
                };
                if (isFail(r)) { return r; }
            }
            return ok(boost::blank());
        } <=
        [&] (Error e) {
            if (e == toError(ErrorCode::SerialDeviceReadTimeout)) {
                // do nothing, just chance to stop
                return ok(boost::blank());
            }
            auto message = fail<Buffer>(e);
            return readMailbox.replace(message);
        };
    }
    dpl("readThreadFunction: finished");
}

template <typename Buffer>
Result<boost::blank> PacketDeviceImplBase<Buffer>::packetFinished(Buffer& buffer) {
    dpb("packetFinished: ", buffer);
    return
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& newBuffer) {
        auto message = ok(std::move(buffer));
        return
        readMailbox.replace(message) >
        [&] {
            buffer = std::move(newBuffer);
            Dbg::dpl("packetFinished: replaced");
            return ok(boost::blank());
        };
    };
}

template <typename Buffer>
Result<Buffer> PacketDeviceImplBase<Buffer>::read(Timeout t) {
    dpl("read: started, timeout %d ms", toMs(t).count());
    return
    readMailbox.fetch(t) >=
    [&] (Result<Buffer>&& r) {
        dpl("read: fetched");
        return std::move(r);
    } <=
    [&] (Error e) {
        if (e == toError(ErrorCode::Timeout)) {
            return fail<Buffer>(toError(ErrorCode::PacketDeviceReadTimeout));
        }
        return fail<Buffer>(e);
    };
}

template <typename Buffer>
Result<boost::blank> PacketDeviceImplBase<Buffer>::write(Buffer b) {
    return
    cobs::stuffAndDelim(std::move(b)) >=
    [&](Buffer&& stuffed) {
        dpb("write: ", stuffed);
        return serialDevice.write(stuffed);
    };
}
