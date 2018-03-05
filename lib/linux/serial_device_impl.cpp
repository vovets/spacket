#include <spacket/serial_device_impl.h>
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

struct SerialDeviceImpl::Impl {
    Impl(int fd, struct timeval byteTimeout);
    ~Impl();
    Result<size_t> read(uint8_t* buffer, size_t maxRead, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size);
    Result<boost::blank> flush();

    int fd;
    struct timeval byteTimeout;
};


SerialDeviceImpl::SerialDeviceImpl(ImplPtr p) noexcept: impl(std::move(p)) {}

SerialDeviceImpl::SerialDeviceImpl(const SerialDeviceImpl& r) noexcept: impl(r.impl) {}

SerialDeviceImpl& SerialDeviceImpl::operator=(const SerialDeviceImpl& src) noexcept {
    impl = src.impl;
}

SerialDeviceImpl::SerialDeviceImpl(SerialDeviceImpl&& r) noexcept: impl(std::move(r.impl)) {}

SerialDeviceImpl& SerialDeviceImpl::operator=(SerialDeviceImpl&& src) noexcept {
    impl = std::move(src.impl);
}

SerialDeviceImpl::~SerialDeviceImpl() {}

Result<SerialDeviceImpl> SerialDeviceImpl::doOpen(PortConfig portConfig) {
    using TvMsec = decltype(timeval().tv_usec);
    
    auto f = [] { return fail<SerialDeviceImpl>(Error::DevInitFailed); };
    
    if (portConfig.baud == Baud::NonStandard) {
        return fail<SerialDeviceImpl>(Error::ConfigBadBaud);
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
    return ok(SerialDeviceImpl(std::move(impl)));
}

Result<size_t> SerialDeviceImpl::read(uint8_t* buffer, size_t maxRead, Timeout t) {
    return impl->read(buffer, maxRead, t);
}

Result<boost::blank> SerialDeviceImpl::write(const uint8_t* buffer, size_t size) {
    return impl->write(buffer, size);
}

Result<boost::blank> SerialDeviceImpl::flush() {
    return impl->flush();
}

SerialDeviceImpl::Impl::Impl(int fd, struct timeval byteTimeout)
    : fd(fd)
    , byteTimeout(byteTimeout) {
}

SerialDeviceImpl::Impl::~Impl() {
    ::close(fd);
}

Result<size_t> SerialDeviceImpl::Impl::read(uint8_t* buffer, size_t maxRead, Timeout t) {
    TRACE("===>");
    auto f = [] { return fail<size_t>(Error::DevReadFailed); };
    auto now = Clock::now();
    auto deadline = now + t;
    uint8_t* cur = buffer;
    uint8_t* const end = buffer + maxRead;
    while (now < deadline && cur < end) {
        struct timeval timeout = byteTimeout;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        callv(result, f, ::select, fd + 1, &fds, nullptr, nullptr, &timeout);
        if (!result) {
            TRACE("select timeout");
            if (cur == buffer) {
                now = Clock::now();
                TRACE("cont");
                continue;
            }
            TRACE("<1== " << cur - buffer);
            return ok(static_cast<size_t>(cur - buffer));
        }
        if (!FD_ISSET(fd, &fds)) {
            TRACE("fd not ready");
            continue;
        }
        size_t requested = end - cur;
        callv(bytesRead, f, ::read, fd, cur, requested);
        TRACE("requested " << requested << ", read " << bytesRead);
        cur += bytesRead;
    }
    TRACE("<2== " << cur - buffer);
    if (cur == buffer) {
        return fail<size_t>(Error::Timeout);
    }
    return ok(static_cast<size_t>(cur - buffer));
}

Result<boost::blank> SerialDeviceImpl::Impl::write(const uint8_t* buffer, size_t size) {
    auto f = []{ return fail<b::blank>(Error::DevWriteFailed); };
    const uint8_t* cur = buffer;
    const uint8_t* end = buffer + size;
    while (cur < end) {
        callv(
            bytesWritten,
            f,
            ::write,
            fd,
            cur,
            end - cur);
        cur += bytesWritten;
    }
    return ok(b::blank());
}

Result<boost::blank> SerialDeviceImpl::Impl::flush() {
    auto f = []{ return fail<b::blank>(Error::DevWriteFailed); };
    call(f, ::tcflush, fd, TCIOFLUSH);
    return ok(boost::blank{});
}
