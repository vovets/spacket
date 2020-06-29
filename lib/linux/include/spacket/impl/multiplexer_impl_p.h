#pragma once

#include <spacket/impl/multiplexer_impl_base.h>

template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
struct MultiplexerImplT: MultiplexerImplBaseT<LowerLevel, NUM_CHANNELS>
{
    using This = MultiplexerImplT<LowerLevel, NUM_CHANNELS>;
    using Base = MultiplexerImplBaseT<LowerLevel, NUM_CHANNELS>;

    using Base::start;
    using Base::wait;

    MultiplexerImplT(alloc::Allocator& allocator, LowerLevel& lowerLevel);
    virtual ~MultiplexerImplT();

    Result<Void> reportError(Error e) override;
};

template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<LowerLevel, NUM_CHANNELS>::MultiplexerImplT(
    alloc::Allocator& allocator,
    LowerLevel& lowerLevel)
    : Base(
        allocator,
        lowerLevel,
        Thread::params())
{
    start();
}

template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<LowerLevel, NUM_CHANNELS>::~MultiplexerImplT() {
    wait();
}

template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<Void> MultiplexerImplT<LowerLevel, NUM_CHANNELS>::reportError(Error e) {
    return fail(e);
}
