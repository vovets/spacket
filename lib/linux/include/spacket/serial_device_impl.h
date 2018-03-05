#pragma once

#include <spacket/result.h>
#include <spacket/port_config.h>
#include <spacket/time_utils.h>

#include <memory>

class SerialDeviceImpl {
    friend class SerialDevice;
    
public:
    template <typename SerialDevice>
    static Result<SerialDevice> open(PortConfig portConfig) {
        return
        doOpen(portConfig) >=
        [&](SerialDeviceImpl&& impl) {
            return ok(SerialDevice(std::move(impl)));
        };
    }

    static Result<SerialDeviceImpl> doOpen(PortConfig portConfig);

    SerialDeviceImpl(const SerialDeviceImpl& r) noexcept;
    SerialDeviceImpl& operator=(const SerialDeviceImpl& src) noexcept;

    SerialDeviceImpl(SerialDeviceImpl&& r) noexcept;
    SerialDeviceImpl& operator=(SerialDeviceImpl&& src) noexcept;
    
    ~SerialDeviceImpl();

    Result<size_t> read(uint8_t* buffer, size_t maxRead, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size);
    Result<boost::blank> flush();

private:
    struct Impl;
    using ImplPtr = std::shared_ptr<Impl>;

private:
    SerialDeviceImpl(ImplPtr p) noexcept;

private:
    ImplPtr impl;
};
