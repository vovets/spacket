#pragma once

#include <spacket/impl/serial_device_impl_p.h>

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/buffer.h>
#include <spacket/bind.h>

#include <memory>
#include <utility>

class SerialDevice: private SerialDeviceImpl {
    friend class SerialDeviceImpl;

public:
    using Impl = SerialDeviceImpl;

//    static constexpr std::size_t maxSize() { return Buffer::maxSize(); }

public:
    template <typename... U>
    static Result<SerialDevice> open(U&&... u) {
        return Impl::template open<SerialDevice>(std::forward<U>(u)...);
    }
    
    SerialDevice(SerialDevice&& src) noexcept : Impl(std::move(src)) {}
        
    SerialDevice& operator=(SerialDevice&& src) noexcept {
        static_cast<Impl&>(*this) = std::move(src);
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
//    Result<Void> write(const Buffer& b, Timeout t) {
//        return Impl::write(b.begin(), b.size(), t);
//    }

    ~SerialDevice() {}

private:
    SerialDevice(Impl&& impl)
        : Impl(std::move(impl))
    {
    }
};
