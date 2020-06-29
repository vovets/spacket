#pragma once

#include <spacket/impl/multiplexer_impl_base.h>

template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
struct MultiplexerImplT: MultiplexerImplBaseT<LowerLevel, NUM_CHANNELS>
{
    using This = MultiplexerImplT<LowerLevel, NUM_CHANNELS>;
    using Base = MultiplexerImplBaseT<LowerLevel, NUM_CHANNELS>;
    using ThreadStorage = ThreadStorageT<332>;

    MultiplexerImplT(alloc::Allocator& allocator, LowerLevel& lowerLevel, tprio_t threadPriority);
    virtual ~MultiplexerImplT();

    using Base::start;
    using Base::wait;

    Result<Void> reportError(Error e) final;

    ThreadStorage threadStorage;
};

template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<LowerLevel, NUM_CHANNELS>::MultiplexerImplT(
    alloc::Allocator& allocator,
    LowerLevel& lowerLevel,
    tprio_t threadPriority)
    : Base(
        allocator,
        lowerLevel,
        Thread::params(threadStorage, threadPriority))
{
    start();
}

template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<LowerLevel, NUM_CHANNELS>::~MultiplexerImplT() {
    wait();
}

template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<Void> MultiplexerImplT<LowerLevel, NUM_CHANNELS>::reportError(Error e) {
    return threadErrorReport(e);
}
