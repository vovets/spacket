#pragma once

#include <spacket/serial_device_base.h>

#include "hal.h"

#if !defined(HAL_USE_UART) || HAL_USE_UART != TRUE
#error SerialDevice needs '#define HAL_USE_UART TRUE' in halconf.h \
    and '#define XXX_UART_USE_XXXX TRUE' in mcuconf.h
#endif

#if !defined(UART_USE_WAIT) || UART_USE_WAIT != TRUE
#error SerialDevice needs '#define UART_USE_WAIT' in halconf.h
#endif

class SerialDevice: public SerialDeviceBase<SerialDevice> {
    friend class SerialDeviceBase<SerialDevice>;
public:
    static Result<SerialDevice> open(UARTDriver* driver);
    SerialDevice(SerialDevice&& src): driver(src.driver) { src.driver = nullptr; }
    ~SerialDevice();
    using SerialDeviceBase<SerialDevice>::write;
    using SerialDeviceBase<SerialDevice>::read;

private:
    SerialDevice(UARTDriver* driver);
    Result<size_t> read(Timeout t, uint8_t* buffer, size_t maxRead);
    Result<boost::blank> write(uint8_t* buffer, size_t size);

private:
    UARTDriver* driver;
};
