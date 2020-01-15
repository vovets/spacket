#pragma once

#include <spacket/impl/serial_device_impl_p.h>

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/buffer.h>
#include <spacket/bind.h>

#include <memory>
#include <utility>

template <typename Buffer>
class SerialDeviceT: private SerialDeviceImpl<Buffer> {
    friend class SerialDeviceImpl<Buffer>;

public:
    using Impl = SerialDeviceImpl<Buffer>;

    static constexpr std::size_t maxSize() { return Buffer::maxSize(); }

public:
    template <typename... U>
    static Result<SerialDeviceT> open(U&&... u) {
        return Impl::template open<SerialDeviceT<Buffer>>(std::forward<U>(u)...);
    }
    
    SerialDeviceT(SerialDeviceT&& src) noexcept : Impl(std::move(src)) {}
        
    SerialDeviceT& operator=(SerialDeviceT&& src) noexcept {
        static_cast<Impl>(*this) = src;
        return *this;
    }

    Result<Buffer> read(Timeout t) {
        return Impl::read(t);
    }

    Result<Void> write(const Buffer& b) {
        return Impl::write(b.begin(), b.size());
    }

    // not every platform supports write with timeout
    // so there may be static assert error here
    Result<Void> write(const Buffer& b, Timeout t) {
        return Impl::write(b.begin(), b.size(), t);
    }

    ~SerialDeviceT() {}

private:
    SerialDeviceT(Impl&& impl)
        : Impl(std::move(impl))
    {
    }
};
