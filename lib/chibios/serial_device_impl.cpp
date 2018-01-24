#include <spacket/serial_device.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

Result<SerialDevice> SerialDevice::open() {
    return ok(SerialDevice{});
}

Result<size_t> SerialDevice::read(Timeout t, uint8_t* buffer, size_t maxRead) {
    return fail<size_t>(Error::MiserableFailure);
}

Result<boost::blank> SerialDevice::write(uint8_t* buffer, size_t size) {
    return fail<boost::blank>(Error::MiserableFailure);
}

#pragma GCC diagnostic pop
