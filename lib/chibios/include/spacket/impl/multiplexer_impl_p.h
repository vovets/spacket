#pragma once

#include <spacket/impl/multiplexer_impl_base.h>

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
struct MultiplexerImplT: MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>,
    boost::intrusive_ref_counter<MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>, boost::thread_unsafe_counter>
{
    using This = MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>;
    using Base = MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>;
    using ThisPtr = boost::intrusive_ptr<This>;
    using ThreadStorage = ThreadStorageT<332>;

    static Result<ThisPtr> open(LowerLevel&& lowerLevel, void* storage, tprio_t threadPriority);

    MultiplexerImplT(LowerLevel&& lowerLevel, tprio_t threadPriority);
    virtual ~MultiplexerImplT();

    using Base::start;
    using Base::wait;

    Result<boost::blank> reportError(Error e) override;

    ThreadStorage threadStorage;
};

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<boost::intrusive_ptr<MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>>>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::open(LowerLevel&& lowerLevel, void* storage, tprio_t threadPriority) {
    return ok(
        ThisPtr(
            new (storage) This(
                std::move(lowerLevel),
                threadPriority)));
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::MultiplexerImplT(
    LowerLevel&& lowerLevel,
    tprio_t threadPriority)
    : Base(
        std::move(lowerLevel),
        Thread::params(threadStorage, threadPriority))
{
    start();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::~MultiplexerImplT() {
    wait();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<boost::blank> MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::reportError(Error e) {
    return threadErrorReport(e);
}
