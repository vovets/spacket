#include "namespaces.h"
#include "serial_device.h"
#include "logging.h"
#include "traced_call.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

const ::speed_t DEFAULT_SPEED = B115200;

struct SerialDevice::Impl {
    Impl(int fd);
    ~Impl();
    Result<Buffer> read(Timeout t, size_t maxSize);
    Result<b::blank> write(const Buffer& b);

    int fd;
};

SerialDevice::SerialDevice(ImplPtr p): impl(std::move(p)) {}
SerialDevice::SerialDevice(SerialDevice&& other): impl(std::move(other.impl)) {}
SerialDevice::~SerialDevice() {}

Result<SerialDevice> SerialDevice::open(PortConfig portConfig) {
    auto f = [] { return fail<SerialDevice>(Error::DevInitFailed); };
    callv(fd, ::open, f, portConfig.device.c_str(), O_RDWR|O_NOCTTY);
    auto impl = std::make_unique<Impl>(fd);
    struct termios termios;
    call(::tcgetattr, f, fd, &termios);
    ::cfmakeraw(&termios);
    call(::cfsetispeed, f, &termios, DEFAULT_SPEED);
    call(::cfsetospeed, f, &termios, DEFAULT_SPEED);
    return ok(SerialDevice(std::move(impl)));
}

Result<Buffer> SerialDevice::read(Timeout t, size_t maxSize) {
    return impl->read(t, maxSize);
}

Result<b::blank> SerialDevice::write(const Buffer& b) {
    return impl->write(b);
}

SerialDevice::Impl::Impl(int fd): fd(fd) {}

SerialDevice::Impl::~Impl() {
    ::close(fd);
}

Result<Buffer> SerialDevice::Impl::read(Timeout t, size_t maxSize) {
    return ok(Buffer(0));
}

Result<b::blank> SerialDevice::Impl::write(const Buffer& b) {
    return ok(b::blank());
}
