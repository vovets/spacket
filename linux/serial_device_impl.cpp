#include <spacket/serial_device.h>
#include "logging.h"
#include "traced_call.h"
#include "namespaces.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

const ::speed_t DEFAULT_SPEED = B115200;
// const ::speed_t DEFAULT_SPEED = B9600;
// const ::speed_t DEFAULT_SPEED = B230400;

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
    callv(fd, f, ::open, portConfig.device.c_str(), O_RDWR|O_NOCTTY|O_NONBLOCK);
    auto impl = std::make_unique<Impl>(fd);
    struct termios termios;
    call(f, ::tcgetattr, fd, &termios);
    ::cfmakeraw(&termios);
    call(f, ::cfsetispeed, &termios, DEFAULT_SPEED);
    call(f, ::cfsetospeed, &termios, DEFAULT_SPEED);
    termios.c_cc[VMIN] = 1;
    termios.c_cc[VTIME] = 1;
    call(f, ::tcsetattr, fd, TCSANOW, &termios);
    call(f, ::tcflush, fd, TCIOFLUSH);
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
    auto f = [] { return fail<Buffer>(Error::DevReadFailed); };
    auto now = Clock::now();
    auto deadline = now + t;
    auto b = Buffer(maxSize);
    uint8_t* cur = b.begin();
    while (now < deadline && cur < b.end()) {
        struct timeval timeout;
        timeout.tv_sec = 0;
        // byte interval * 1.5
        timeout.tv_usec = (1.5 * 1000000.) / (115200. / 10.);
        // timeout.tv_usec = (2. * 1000000.) / (9600. / 10.);
        // timeout.tv_usec = 400;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        callv(result, f, ::select, fd + 1, &fds, nullptr, nullptr, &timeout);
        if (!result) {
            if (cur == b.begin()) {
                now = Clock::now();
                continue;
            }
            return ok(b.prefix(cur - b.begin()));
        }
        size_t requested = b.end() - cur;
        callv(bytesRead, f, ::read, fd, cur, requested);
        TRACE("requested " << requested << ", read " << bytesRead);
        cur += bytesRead;
    }
    return ok(b.prefix(cur - b.begin()));
}

Result<b::blank> SerialDevice::Impl::write(const Buffer& b) {
    auto f = []{ return fail<b::blank>(Error::DevWriteFailed); };
    uint8_t* cur = b.begin();
    while (cur < b.end()) {
        callv(
            bytesWritten,
            f,
            ::write,
            fd,
            cur,
            b.end() - cur);
        cur += bytesWritten;
    }
    call(f, ::tcdrain, fd);
    return ok(b::blank());
}
