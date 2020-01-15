#pragma once

#include <spacket/crc.h>

#include <array>


namespace multiplexer_impl_base {

template <typename Buffer>
struct Debug {

#define PREFIX()
#ifdef MULTIPLEXER_ENABLE_DEBUG_PRINT

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

} // multiplexer_impl_base

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
class MultiplexerImplBaseT: public multiplexer_impl_base::Debug<Buffer> {
    using Mailbox = MailboxImplT<Result<Buffer>>;
    using Mailboxes = std::array<Mailbox, NUM_CHANNELS>;
    using Dbg = multiplexer_impl_base::Debug<Buffer>;

    static constexpr Timeout stopCheckPeriod = std::chrono::milliseconds(10);

protected:
    using Dbg::dpl;
    using Dbg::dpb;

    MultiplexerImplBaseT(LowerLevel& lowerLevel, ThreadParams p);
    virtual ~MultiplexerImplBaseT();
    
    MultiplexerImplBaseT(MultiplexerImplBaseT&&) = delete;
    MultiplexerImplBaseT(const MultiplexerImplBaseT&) = delete;

    void start();
    void wait();

public:
    Result<Buffer> read(std::uint8_t channel, Timeout t);
    Result<Void> write(std::uint8_t channel, Buffer b);

private:
    void readThreadFunction();
    virtual Result<Void> reportError(Error e) = 0;

private:
    LowerLevel& lowerLevel;
    Mailboxes readMailboxes;
    ThreadParams readThreadParams;
    Thread readThread;
};

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
constexpr Timeout MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>::stopCheckPeriod;

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>::MultiplexerImplBaseT(
    LowerLevel& lowerLevel,
    ThreadParams p)
    : lowerLevel(lowerLevel)
    , readThreadParams(p)
{
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
void MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>::start() {
    readThread = Thread::create(readThreadParams, [&] { this->readThreadFunction(); });
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
void MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>::wait() {
    readThread.requestStop();
    readThread.wait();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>::~MultiplexerImplBaseT() {
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<Buffer> MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>::read(std::uint8_t c, Timeout t) {
    dpl("mib::read|started|channel %d|timeout %d ms", c, toMs(t).count());
    if (c >= NUM_CHANNELS) {
        return fail<Buffer>(toError(ErrorCode::MultiplexerBadChannel));
    }
    return
    readMailboxes[c].fetch(t) >=
    [&] (Result<Buffer>&& r) {
        if (isOk(r)) {
            dpb("mib::read|channel %d|fetched ", &getOkUnsafe(r), c);
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

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<Void> MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>::write(std::uint8_t c, Buffer b) {
    if (c >= NUM_CHANNELS) {
        return fail(toError(ErrorCode::MultiplexerBadChannel));
    }
    if (b.size() > Buffer::maxSize() - 1) {
        return fail(toError(ErrorCode::MultiplexerBufferTooBig));
    }
    return
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& newBuffer) {
        dpb("mib::write|channel %d|", &b, c);
        newBuffer.begin()[0] = c;
        std::memcpy(newBuffer.begin() + 1, b.begin(), b.size());
        newBuffer.resize(b.size() + 1);
        return
        crc::append(std::move(newBuffer)) >=
        [&] (Buffer&& b) { return lowerLevel.write(std::move(b)); };
    };
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
void MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>::readThreadFunction() {
    Thread::setName("mx");
    dpl("mib::readThreadFunction|started");
    while (!Thread::shouldStop()) {
        lowerLevel.read(stopCheckPeriod) >=
        [&] (Buffer&& b) {
            return crc::check(std::move(b));
        } >=
        [&] (Buffer&& b) {
            if (b.size() == 0) {
                return fail(toError(ErrorCode::MultiplexerBufferTooSmall));
            }
            std::uint8_t channel = b.begin()[0];
            if (channel >= NUM_CHANNELS) {
                return fail(toError(ErrorCode::MultiplexerBadChannel));
            }
            return
            Buffer::create(Buffer::maxSize()) >=
            [&] (Buffer&& newBuffer) {
                std::memcpy(newBuffer.begin(), b.begin() + 1, b.size() - 1);
                newBuffer.resize(b.size() -1);
                dpb("mib::readThreadFunction|channel %d|", &newBuffer, channel);
                auto message = ok(std::move(newBuffer));
                return
                readMailboxes[channel].replace(message) >
                [&] {
                    dpl("mib::readThreadFunction|channel %d|replaced", channel);
                    return ok();
                };
            };
        } <=
        [&] (Error e) {
            if (e == toError(ErrorCode::ReadTimeout)) {
                return ok();
            }
            return reportError(e);
        };
    }
    dpl("mib::readThreadFunction|finished");
}
