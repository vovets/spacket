#pragma once

#include <spacket/impl/multiplexer_impl.h>


template <typename LowerLevel, std::uint8_t NUM_CHANNELS>
class MultiplexerT: private MultiplexerImplT<LowerLevel, NUM_CHANNELS> {
    using Impl = MultiplexerImplT<LowerLevel, NUM_CHANNELS>;

public:
    static constexpr std::size_t maxSize() { return LowerLevel::maxSize() - 1; }

public:
    template <typename ...U>
    MultiplexerT(alloc::Allocator& allocator, LowerLevel& lowerLevel, U&&... u)
        : Impl(allocator, lowerLevel, std::forward<U>(u)...) {
    }

    virtual ~MultiplexerT() {}

    MultiplexerT(const MultiplexerT&) = delete;
    MultiplexerT(MultiplexerT&&) = delete;

    Result<Buffer> read(std::uint8_t channel, Timeout t) {
        return Impl::read(channel, t);
    }

    Result<Void> write(std::uint8_t channel, Buffer b) {
        return Impl::write(channel, std::move(b));
    }
};
