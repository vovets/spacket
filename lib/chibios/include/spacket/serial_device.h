#pragma once

#include <spacket/serial_device_base.h>

class SerialDevice: public SerialDeviceBase<SerialDevice> {
    friend class SerialDeviceBase<SerialDevice>;
public:
    static Result<SerialDevice> open();

private:
    Result<size_t> read(Timeout t, uint8_t* buffer, size_t maxRead);
    Result<boost::blank> write(uint8_t* buffer, size_t size);
};
