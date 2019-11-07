#pragma once

#include <spacket/result.h>
#include <spacket/port_config.h>
#include <spacket/time_utils.h>

#include <memory>

class NativeDevice {
public:
    static Result<NativeDevice> doOpen(PortConfig portConfig);

    NativeDevice(NativeDevice&&) = default;
    NativeDevice& operator=(NativeDevice&&) = default;

    NativeDevice(const NativeDevice&) = delete;
    NativeDevice& operator=(const NativeDevice&) = delete;

    Result<size_t> read(uint8_t* buffer, size_t maxRead, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size);

private:
    struct Impl;
    using ImplPtr = std::shared_ptr<Impl>;

private:
    NativeDevice(ImplPtr p) noexcept;

private:
    ImplPtr impl;
};

template <typename Buffer>
class SerialDeviceImpl {
public:
    template <typename SerialDevice>
    static Result<SerialDevice> open(PortConfig portConfig) {
        return
        NativeDevice::doOpen(portConfig) >=
        [&](NativeDevice nativeDevice) {
            return ok(SerialDevice(SerialDeviceImpl(std::move(nativeDevice))));
        };
    }

    SerialDeviceImpl(SerialDeviceImpl&& r) noexcept
        : nativeDevice(std::move(r.nativeDevice)) {
    }

    SerialDeviceImpl& operator=(SerialDeviceImpl&& src) noexcept {
        nativeDevice = std::move(src.nativeDevice);
    }
    
    Result<Buffer> read(Timeout t) {
        return
        Buffer::create(Buffer::maxSize()) >=
        [&] (Buffer buffer) {
            return
            nativeDevice.read(buffer.begin(), Buffer::maxSize(), t) >=
            [&] (std::size_t bytesRead) {
                buffer.resize(bytesRead);
                return ok(std::move(buffer));
            };
        };
    }

    Result<boost::blank> write(const uint8_t* buffer, size_t size) {
        return nativeDevice.write(buffer, size);
    }

private:
    SerialDeviceImpl(NativeDevice&& nativeDevice): nativeDevice(std::move(nativeDevice)) {}

private:
    NativeDevice nativeDevice;
};
