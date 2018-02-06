#pragma once

#include <spacket/serial_device_base.h>

#include "hal.h"

#include <boost/intrusive_ptr.hpp>

#if !defined(HAL_USE_UART) || HAL_USE_UART != TRUE
#error SerialDevice needs '#define HAL_USE_UART TRUE' in halconf.h \
    and '#define XXX_UART_USE_XXXX TRUE' in mcuconf.h
#endif

#if !defined(UART_USE_WAIT) || UART_USE_WAIT != TRUE
#error SerialDevice needs '#define UART_USE_WAIT' in halconf.h
#endif

inline
void intrusive_ptr_add_ref(UARTDriver* d) {
    if (d->refCnt < std::numeric_limits<decltype(d->refCnt)>::max()) {
        ++d->refCnt;
    }
}

inline
void intrusive_ptr_release(UARTDriver* d) {
    --d->refCnt;
    if (d->refCnt == 0) {
        uartStop(d);
    }
}
    
class SerialDevice: public SerialDeviceBase<SerialDevice> {
    friend class SerialDeviceBase<SerialDevice>;

public:
    static Result<SerialDevice> open(UARTDriver* driver);

    SerialDevice(SerialDevice&& src) noexcept;
    SerialDevice& operator=(SerialDevice&& src) noexcept;

    SerialDevice(const SerialDevice& src) noexcept;
    SerialDevice& operator=(const SerialDevice& src) noexcept;

    using SerialDeviceBase<SerialDevice>::write;
    using SerialDeviceBase<SerialDevice>::read;

private:
    SerialDevice(UARTDriver* driver);

    Result<size_t> read(uint8_t* buffer, size_t maxRead, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size) {
        return write(buffer, size, INFINITE_TIMEOUT);
    }

private:
    using Driver = boost::intrusive_ptr<UARTDriver>;
    Driver driver;
};
