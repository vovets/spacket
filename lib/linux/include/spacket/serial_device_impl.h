#pragma once

#include <spacket/serial_device_base.h>
#include <spacket/result.h>
#include <spacket/port_config.h>

class SerialDevice: public SerialDeviceBase<SerialDevice> {
    friend class SerialDeviceBase<SerialDevice>;
    
public:
    static Result<SerialDevice> open(PortConfig portConfig);

    SerialDevice(SerialDevice&& r);
    ~SerialDevice();

    using SerialDeviceBase<SerialDevice>::write;
    using SerialDeviceBase<SerialDevice>::read;

    Result<boost::blank> flush();

private:
    Result<size_t> read(uint8_t* buffer, size_t maxRead, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size);

private:
    struct Impl;
    using ImplPtr = std::unique_ptr<Impl>;

private:
    SerialDevice(ImplPtr p);

private:
    ImplPtr impl;
};
