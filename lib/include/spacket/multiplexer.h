#pragma once

#include <spacket/impl/multiplexer_impl.h>

#include <boost/intrusive_ptr.hpp>


template <typename Buffer, typename LowerLevel, std::uint8_t NUM_CHANNELS>
class MultiplexerT {
    using Impl = MultiplexerImplT<Buffer, LowerLevel, NUM_CHANNELS>;

public:
    using Storage = std::aligned_storage_t<sizeof(Impl), alignof(Impl)>;

public:
    template <typename ...U>
    static Result<MultiplexerT<Buffer, LowerLevel, NUM_CHANNELS>> open(LowerLevel lowerLevel, U&&... u) {
        return
        Impl::open(std::move(lowerLevel), std::forward<U>(u)...) >=
        [&] (ImplPtr&& impl) {
            return ok(MultiplexerT(std::move(impl)));
        };
    }

    MultiplexerT(const MultiplexerT&) = delete;

    MultiplexerT(MultiplexerT&&) = default;

    Result<Buffer> read(std::uint8_t channel, Timeout t) {
        return impl->read(channel, t);
    }

    Result<boost::blank> write(std::uint8_t channel, Buffer b) {
        return impl->write(channel, std::move(b));
    }

private:
    using ImplPtr = boost::intrusive_ptr<Impl>;

    MultiplexerT(ImplPtr impl)
        : impl(std::move(impl))
    {
    }

    ImplPtr impl;
};
    
