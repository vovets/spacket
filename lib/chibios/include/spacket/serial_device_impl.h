#pragma once

#include <spacket/result.h>
#include <spacket/time_utils.h>

#include "hal.h"

#include <boost/intrusive_ptr.hpp>

#include <limits>

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
    
class SerialDeviceImpl {
public:
    template <typename SerialDevice>
    static Result<SerialDevice> open(UARTDriver* driver) {
        if (driver->refCnt > 0) {
            return fail<SerialDevice>(Error::DevAlreadyOpened);
        }
        start(driver);
        return ok(SerialDevice(SerialDeviceImpl(driver)));
    }

    static void start(UARTDriver* driver);

    SerialDeviceImpl(SerialDeviceImpl&& src) noexcept;
    SerialDeviceImpl& operator=(SerialDeviceImpl&& src) noexcept;

    SerialDeviceImpl(const SerialDeviceImpl& src) noexcept;
    SerialDeviceImpl& operator=(const SerialDeviceImpl& src) noexcept;

    SerialDeviceImpl(UARTDriver* driver);

    Result<size_t> read(uint8_t* buffer, size_t maxRead, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size) {
        return write(buffer, size, INFINITE_TIMEOUT);
    }

private:
    using Driver = boost::intrusive_ptr<UARTDriver>;
    Driver driver;
};
