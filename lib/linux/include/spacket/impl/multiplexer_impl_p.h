#pragma once

#include <spacket/impl/multiplexer_impl_base.h>

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
struct MultiplexerImplT: MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>,
    boost::intrusive_ref_counter<MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>>
{
    using This = MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>;
    using Base = MultiplexerImplBaseT<Buffer, LowerLevel, NUM_CHANNELS>;
    using ThisPtr = boost::intrusive_ptr<This>;

    using Base::start;
    using Base::wait;

    static Result<ThisPtr> open(LowerLevel&& lowerLevel);

    MultiplexerImplT(LowerLevel&& lowerLevel);
    virtual ~MultiplexerImplT();

    Result<boost::blank> reportError(Error e) override;
};

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<boost::intrusive_ptr<MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>>>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::open(LowerLevel&& lowerLevel) {
    return ok(ThisPtr(new This(std::move(lowerLevel))));
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::MultiplexerImplT(
    LowerLevel&& lowerLevel)
    : Base(
        std::move(lowerLevel),
        Thread::params())
{
    start();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::~MultiplexerImplT() {
    wait();
}

template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
Result<boost::blank> MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>::reportError(Error e) {
    return fail<boost::blank>(e);
}
