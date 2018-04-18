#pragma once

#include <spacket/serial_device_impl.h>

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/buffer.h>
#include <spacket/bind.h>

#include <memory>
#include <utility>

class SerialDevice: private SerialDeviceImpl {
    friend class SerialDeviceImpl;
public:
    template <typename... U>
    static Result<SerialDevice> open(U&&... u) {
        return SerialDeviceImpl::open<SerialDevice>(std::forward<U>(u)...);
    }
    
    SerialDevice(SerialDevice&& src) noexcept : SerialDeviceImpl(std::move(src)) {}
        
    SerialDevice& operator=(SerialDevice&& src) noexcept {
        static_cast<SerialDeviceImpl>(*this) = src;
        return *this;
    }

    SerialDevice(const SerialDevice& src) noexcept : SerialDeviceImpl(src) {}
    
    SerialDevice& operator=(const SerialDevice& src) noexcept {
        static_cast<SerialDeviceImpl>(*this) = src;
        return *this;
    }

    template <typename Buffer>
    Result<Buffer> read(Buffer b, Timeout t) {
        return
        SerialDeviceImpl::read(b.begin(), b.size(), t) >=
        [&](size_t size) {
            b.resize(size);
            return ok(std::move(b));
        };
    }

    template <typename Buffer>
    Result<boost::blank> write(const Buffer& b) {
        return SerialDeviceImpl::write(b.begin(), b.size());
    }

    // not every platform supports write with timeout
    // so there may be static assert error here
    template <typename Buffer>
    Result<boost::blank> write(const Buffer& b, Timeout t) {
        return SerialDeviceImpl::write(b.begin(), b.size(), t);
    }

    Result<boost::blank> flush() {
        return SerialDeviceImpl::flush();
    }
    
    ~SerialDevice() {}

private:
    SerialDevice(SerialDeviceImpl&& impl)
        : SerialDeviceImpl(std::move(impl))
    {
    }
};
