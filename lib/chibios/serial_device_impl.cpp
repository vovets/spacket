#include <spacket/serial_device_impl.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace {

void txend2(UARTDriver*) {}

UARTConfig config = {
    .txend1_cb = nullptr,
    // without this callback driver won't enable TC interrupt
    // so uartSendFullTimeout will stuck forever
    .txend2_cb = txend2,
    .rxend_cb = nullptr,
    .rxchar_cb = nullptr,
    .rxerr_cb = nullptr,
    .timeout_cb = nullptr,
    .speed = 921600,
    .cr1 = USART_CR1_IDLEIE,
    .cr2 = 0,
    .cr3 = 0
};

}

SerialDevice::SerialDevice(UARTDriver* driver): driver(driver) {}

Result<SerialDevice> SerialDevice::open(UARTDriver* driver) {
    if (driver->refCnt > 0) {
        return fail<SerialDevice>(Error::DevAlreadyOpened);
    }
    uartStart(driver, &config);
    return ok(SerialDevice(driver));
}

SerialDevice::SerialDevice(SerialDevice&& src) noexcept
    : driver(std::move(src.driver)) {
}
    
SerialDevice& SerialDevice::operator=(SerialDevice&& src) noexcept {
    driver = std::move(src.driver);
    return *this;
}

SerialDevice::SerialDevice(const SerialDevice& src) noexcept
    : driver(src.driver) {
}

SerialDevice& SerialDevice::operator=(const SerialDevice& src) noexcept {
    driver = src.driver;
    return *this;
}


Result<size_t> SerialDevice::read(uint8_t* buffer, size_t maxRead, Timeout t) {
    size_t received = maxRead;
    auto msg = uartReceiveTimeout(driver.get(), &received, buffer, toSystime(t));
    if (msg != MSG_OK) {
        if (received > 0) {
            return ok(received);
        }
        return fail<size_t>(Error::DevReadTimeout);
    }
    return ok(received);
}

Result<boost::blank> SerialDevice::write(const uint8_t* buffer, size_t size, Timeout t) {
    auto msg = uartSendFullTimeout(driver.get(), &size, buffer, toSystime(t));
    return msg == MSG_OK ? ok(boost::blank()) : fail<boost::blank>(Error::DevWriteTimeout);
}

#pragma GCC diagnostic pop
