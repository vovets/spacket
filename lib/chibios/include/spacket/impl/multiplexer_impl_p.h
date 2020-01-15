#pragma once

#include <spacket/impl/multiplexer_impl_base.h>

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
struct MultiplexerImplT: MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>
{
    using This = MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>;
    using Base = MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>;
    using ThreadStorage = ThreadStorageT<332>;

    MultiplexerImplT(LowerLevel& lowerLevel, tprio_t threadPriority);
    virtual ~MultiplexerImplT();

    using Base::start;
    using Base::wait;

    Result<Void> reportError(Error e) final;

    ThreadStorage threadStorage;
};

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::MultiplexerImplT(
    LowerLevel& lowerLevel,
    tprio_t threadPriority)
    : Base(
        lowerLevel,
        Thread::params(threadStorage, threadPriority))
{
    start();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::~MultiplexerImplT() {
    wait();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<Void> MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::reportError(Error e) {
    return threadErrorReport(e);
}
