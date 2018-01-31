#include <spacket/serial_device_impl.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace {

UARTConfig config = {
    .txend1_cb = nullptr,
    .txend2_cb = nullptr,
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

SerialDevice::~SerialDevice() {
    if (driver) { uartStop(driver); }
}

Result<SerialDevice> SerialDevice::open(UARTDriver* driver) {
    uartStart(driver, &config);
    return ok(SerialDevice(driver));
}

Result<size_t> SerialDevice::read(Timeout t, uint8_t* buffer, size_t maxRead) {
    size_t received = maxRead;
    auto msg = uartReceiveTimeout(driver, &received, buffer, toSystime(t));
    if (msg != MSG_OK) {
        if (received > 0) {
            return ok(received);
        }
        return fail<size_t>(Error::DevReadTimeout);
    }
    return ok(received);
}

Result<boost::blank> SerialDevice::write(uint8_t* buffer, size_t size) {
    auto msg = uartSendFullTimeout(driver, &size, buffer, toSystime(INFINITE_TIMEOUT));
    return msg == MSG_OK ? ok(boost::blank()) : fail<boost::blank>(Error::DevWriteFailed);
}

#pragma GCC diagnostic pop
