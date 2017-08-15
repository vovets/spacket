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


size_t timeoutFromBaud(Baud b) {
    return (2. * 1000000.) / (toInt(b) / 10.);
}

::speed_t fromBaud(Baud b) {
    switch (b) {
#define BAUD(X) case Baud::B_##X: return B##X
#define X(BAUD, SEP) BAUD SEP
        BAUD_LIST(BAUD, SEP_SEMICOLON);
#undef X
#undef BAUD
        default:
            return B0;
    }
}

struct SerialDevice::Impl {
    Impl(int fd, struct timeval byteTimeout);
    ~Impl();
    Result<Buffer> read(Timeout t, size_t maxSize);
    Result<b::blank> write(const Buffer& b);
    Result<boost::blank> flush();

    int fd;
    struct timeval byteTimeout;
};

SerialDevice::SerialDevice(ImplPtr p): impl(std::move(p)) {}
SerialDevice::SerialDevice(SerialDevice&& other): impl(std::move(other.impl)) {}
SerialDevice::~SerialDevice() {}

Result<SerialDevice> SerialDevice::open(PortConfig portConfig) {
    using TvMsec = decltype(timeval().tv_usec);
    
    auto f = [] { return fail<SerialDevice>(Error::DevInitFailed); };
    
    if (portConfig.baud == Baud::NonStandard) {
        return fail<SerialDevice>(Error::ConfigBadBaud);
    }
    
    struct timeval byteTimeout{
        0,
        portConfig.byteTimeout_us
        ? static_cast<TvMsec>(portConfig.byteTimeout_us)
        : static_cast<TvMsec>(timeoutFromBaud(portConfig.baud))};
    
    callv(fd, f, ::open, portConfig.device.c_str(), O_RDWR|O_NOCTTY|O_NONBLOCK);
    auto impl = std::make_unique<Impl>(fd, byteTimeout);
    struct termios termios;
    call(f, ::tcgetattr, fd, &termios);
    ::cfmakeraw(&termios);
    ::speed_t speed = fromBaud(portConfig.baud);
    call(f, ::cfsetispeed, &termios, speed);
    call(f, ::cfsetospeed, &termios, speed);
    termios.c_cc[VMIN] = 0;
    termios.c_cc[VTIME] = 0;
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

Result<boost::blank> SerialDevice::flush() {
    return impl->flush();
}

SerialDevice::Impl::Impl(int fd, struct timeval byteTimeout)
    : fd(fd)
    , byteTimeout(byteTimeout) {
}

SerialDevice::Impl::~Impl() {
    ::close(fd);
}

Result<Buffer> SerialDevice::Impl::read(Timeout t, size_t maxSize) {
    TRACE("===>");
    auto f = [] { return fail<Buffer>(Error::DevReadFailed); };
    auto now = Clock::now();
    auto deadline = now + t;
    auto b = Buffer(maxSize);
    uint8_t* cur = b.begin();
    while (now < deadline && cur < b.end()) {
        struct timeval timeout = byteTimeout;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        callv(result, f, ::select, fd + 1, &fds, nullptr, nullptr, &timeout);
        if (!result) {
            TRACE("select timeout");
            if (cur == b.begin()) {
                now = Clock::now();
                TRACE("cont");
                continue;
            }
            TRACE("<1== " << cur - b.begin());
            return ok(b.prefix(cur - b.begin()));
        }
        if (!FD_ISSET(fd, &fds)) {
            TRACE("fd not ready");
            continue;
        }
        size_t requested = b.end() - cur;
        callv(bytesRead, f, ::read, fd, cur, requested);
        TRACE("requested " << requested << ", read " << bytesRead);
        cur += bytesRead;
    }
    TRACE("<2== " << cur - b.begin());
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
    return ok(b::blank());
}

Result<boost::blank> SerialDevice::Impl::flush() {
    auto f = []{ return fail<b::blank>(Error::DevWriteFailed); };
    call(f, ::tcflush, fd, TCIOFLUSH);
    return ok(boost::blank{});
}
