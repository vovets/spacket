#pragma once

#include <spacket/result.h>
#include <spacket/result_utils.h>
#include <spacket/port_config.h>
#include <spacket/time_utils.h>
#include <spacket/allocator.h>
#include <spacket/buffer.h>

#include <memory>

class NativeDevice {
public:
    static Result<NativeDevice> doOpen(PortConfig portConfig);

    NativeDevice(NativeDevice&&) = default;
    NativeDevice& operator=(NativeDevice&&) = default;

    NativeDevice(const NativeDevice&) = delete;
    NativeDevice& operator=(const NativeDevice&) = delete;

    Result<size_t> read(uint8_t* buffer, size_t maxRead, Timeout t);
    Result<Void> write(const uint8_t* buffer, size_t size);

private:
    struct Impl;
    using ImplPtr = std::shared_ptr<Impl>;

private:
    NativeDevice(ImplPtr p) noexcept;

private:
    ImplPtr impl;
};

class SerialDeviceImpl {
public:
    template <typename SerialDevice>
    static Result<SerialDevice> open(alloc::Allocator& allocator, PortConfig portConfig) {
        return
        NativeDevice::doOpen(portConfig) >=
        [&](NativeDevice nativeDevice) {
            return ok(SerialDevice(SerialDeviceImpl(allocator, std::move(nativeDevice))));
        };
    }

    SerialDeviceImpl(SerialDeviceImpl&& r) noexcept
        : allocator(r.allocator), nativeDevice(std::move(r.nativeDevice)) {
    }

    SerialDeviceImpl& operator=(SerialDeviceImpl&& src) noexcept {
        nativeDevice = std::move(src.nativeDevice);
    }
    
    Result<Buffer> read(Timeout t) {
        return
        Buffer::create(allocator) >=
        [&] (Buffer buffer) {
            return
            nativeDevice.read(buffer.begin(), buffer.maxSize(), t) >=
            [&] (std::size_t bytesRead) {
                buffer.resize(bytesRead);
                return ok(std::move(buffer));
            };
        };
    }

    Result<Void> write(const uint8_t* buffer, size_t size) {
        return nativeDevice.write(buffer, size);
    }

private:
    SerialDeviceImpl(alloc::Allocator& allocator, NativeDevice&& nativeDevice): allocator(allocator), nativeDevice(std::move(nativeDevice)) {}

private:
    NativeDevice nativeDevice;
    alloc::Allocator& allocator;
};
