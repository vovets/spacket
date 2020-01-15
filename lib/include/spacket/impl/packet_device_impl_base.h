#pragma once

#include <spacket/result.h>
#include <spacket/debug_print.h>
#include <spacket/buffer_debug.h>
#include <spacket/mailbox.h>
#include <spacket/thread.h>
#include <spacket/impl/packet_decode_fsm.h>
#include <spacket/cobs.h>

namespace packet_device_impl_base {

template <typename Buffer>
struct Debug {

#define PREFIX()
#ifdef PACKET_DEVICE_ENABLE_DEBUG_PRINT

IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPX_FUNCTIONS
IMPLEMENT_DPB_FUNCTION

#else

IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPX_FUNCTIONS_NOP
IMPLEMENT_DPB_FUNCTION_NOP

#endif
#undef PREFIX

};

} // packet_device_impl_base

template <typename Buffer, typename LowerLevel>
class PacketDeviceImplBase: public packet_device_impl_base::Debug<Buffer>
{
    using This = PacketDeviceImplBase<Buffer, LowerLevel>;
    using Dbg = packet_device_impl_base::Debug<Buffer>;
    using PacketDecodeFSM = PacketDecodeFSMT<Buffer>;
    using Mailbox = MailboxImplT<Result<Buffer>>;

    static constexpr Timeout stopCheckPeriod = std::chrono::milliseconds(10);

public:
    Result<Buffer> read(Timeout t);

    Result<Void> write(Buffer b);

protected:
    using Dbg::dpl;
    using Dbg::dpb;

    PacketDeviceImplBase(LowerLevel& lowerLevel, Buffer&& buffer, ThreadParams p);
    virtual ~PacketDeviceImplBase();

    PacketDeviceImplBase(PacketDeviceImplBase&&) = delete;
    PacketDeviceImplBase(const PacketDeviceImplBase&) = delete;

    void start();
    void wait();

    void readThreadFunction();

private:
    Result<Void> packetFinished(Buffer& buffer);
    virtual Result<Void> reportError(Error e) = 0;
    
private:
    PacketDecodeFSM decodeFSM;
    LowerLevel& lowerLevel;
    Mailbox readMailbox;
    ThreadParams readThreadParams;
    Thread readThread;
};

template <typename Buffer, typename LowerLevel>
constexpr Timeout PacketDeviceImplBase<Buffer, LowerLevel>::stopCheckPeriod;

template <typename Buffer, typename LowerLevel>
PacketDeviceImplBase<Buffer, LowerLevel>::PacketDeviceImplBase(
LowerLevel& lowerLevel,
Buffer&& buffer,
ThreadParams p)
    : decodeFSM(
        [&] (Buffer& b) { return this->packetFinished(b); },
        std::move(buffer)
        )
    , lowerLevel(lowerLevel)
    , readThreadParams(p)
{
}

template <typename Buffer, typename LowerLevel>
PacketDeviceImplBase<Buffer, LowerLevel>::~PacketDeviceImplBase() {
}

template <typename Buffer, typename LowerLevel>
void PacketDeviceImplBase<Buffer, LowerLevel>::start() {
    readThread = Thread::create(readThreadParams, [&] { this->readThreadFunction(); });
}

template <typename Buffer, typename LowerLevel>
void PacketDeviceImplBase<Buffer, LowerLevel>::wait() {
    readThread.requestStop();
    readThread.wait();
}

template <typename Buffer, typename LowerLevel>
void PacketDeviceImplBase<Buffer, LowerLevel>::readThreadFunction() {
    Thread::setName("pd");
    dpl("pdib::readThreadFunction|started");
    while (!Thread::shouldStop()) {
        lowerLevel.read(stopCheckPeriod) >=
        [&] (Buffer&& b) {
            dpb("pdib::readThreadFunction|read ", &b);
            for (std::size_t i = 0; i < b.size(); ++i) {
                auto r = decodeFSM.consume(b.begin()[i]) <=
                [&] (Error e) {
                    return reportError(e);
                };
                if (isFail(r)) { return r; }
            }
            return ok();
        } <=
        [&] (Error e) {
            if (e == toError(ErrorCode::ReadTimeout)) {
                // do nothing, just chance to stop
                return ok();
            }
            auto message = fail<Buffer>(e);
            return readMailbox.replace(message);
        };
    }
    dpl("pdib::readThreadFunction|finished");
}

template <typename Buffer, typename LowerLevel>
Result<Void> PacketDeviceImplBase<Buffer, LowerLevel>::packetFinished(Buffer& buffer) {
    dpb("pdib::packetFinished|", &buffer);
    return
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& newBuffer) {
        auto message = ok(std::move(buffer));
        return
        readMailbox.replace(message) >
        [&] {
            buffer = std::move(newBuffer);
            Dbg::dpl("pdib::packetFinished|replaced");
            return ok();
        };
    };
}

template <typename Buffer, typename LowerLevel>
Result<Buffer> PacketDeviceImplBase<Buffer, LowerLevel>::read(Timeout t) {
    dpl("pdib::read|started, timeout %d ms", toMs(t).count());
    return
    readMailbox.fetch(t) >=
    [&] (Result<Buffer> r) {
        if (isOk(r)) {
            dpb("pdib::read|fetched ", &getOkUnsafe(r));
        }
        return std::move(r);
    } <=
    [&] (Error e) {
        if (e == toError(ErrorCode::Timeout)) {
            return fail<Buffer>(toError(ErrorCode::ReadTimeout));
        }
        return fail<Buffer>(e);
    };
}

template <typename Buffer, typename LowerLevel>
Result<Void> PacketDeviceImplBase<Buffer, LowerLevel>::write(Buffer b) {
    return
    cobs::stuffAndDelim(std::move(b)) >=
    [&](Buffer&& stuffed) {
        dpb("pdib::write| ", &stuffed);
        return lowerLevel.write(stuffed);
    };
}
