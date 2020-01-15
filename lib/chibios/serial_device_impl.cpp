#include <spacket/serial_device_impl.h>

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

SerialDeviceImpl::SerialDeviceImpl(UARTDriver* driver): driver(driver) {}

void SerialDeviceImpl::start(UARTDriver* driver) {
    uartStart(driver, &config);
}

SerialDeviceImpl::SerialDeviceImpl(SerialDeviceImpl&& src) noexcept
    : driver(std::move(src.driver)) {
}
    
SerialDeviceImpl& SerialDeviceImpl::operator=(SerialDeviceImpl&& src) noexcept {
    driver = std::move(src.driver);
    return *this;
}

SerialDeviceImpl::SerialDeviceImpl(const SerialDeviceImpl& src) noexcept
    : driver(src.driver) {
}

SerialDeviceImpl& SerialDeviceImpl::operator=(const SerialDeviceImpl& src) noexcept {
    driver = src.driver;
    return *this;
}


Result<size_t> SerialDeviceImpl::read(uint8_t* buffer, size_t maxRead, Timeout t) {
    size_t received = maxRead;
    auto msg = uartReceiveTimeout(driver.get(), &received, buffer, toSystime(t));
    if (msg != MSG_OK) {
        if (received > 0) {
            return ok(received);
        }
        if (msg == MSG_TIMEOUT) {
            return fail<size_t>(toError(ErrorCode::DevReadTimeout));
        }
        return fail<size_t>(toError(ErrorCode::DevReadError));
    }
    return ok(received);
}

Result<Void> SerialDeviceImpl::write(const uint8_t* buffer, size_t size, Timeout t) {
    auto msg = uartSendFullTimeout(driver.get(), &size, buffer, toSystime(t));
    return msg == MSG_OK ? ok() : fail(toError(ErrorCode::DevWriteTimeout));
}
