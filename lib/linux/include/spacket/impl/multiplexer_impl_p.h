#pragma once

#include <spacket/impl/multiplexer_impl_base.h>

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
struct MultiplexerImplT: MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>
{
    using This = MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>;
    using Base = MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>;

    using Base::start;
    using Base::wait;

    MultiplexerImplT(LowerLevel& lowerLevel);
    virtual ~MultiplexerImplT();

    Result<Void> reportError(Error e) override;
};

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::MultiplexerImplT(
    LowerLevel& lowerLevel)
    : Base(
        lowerLevel,
        Thread::params())
{
    start();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::~MultiplexerImplT() {
    wait();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<Void> MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::reportError(Error e) {
    return fail(e);
}
