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

struct NativeDevice::Impl {
    Impl(int fd, struct timeval byteTimeout);
    ~Impl();
    Result<size_t> read(uint8_t* buffer, size_t maxRead, Timeout t);
    Result<boost::blank> write(const uint8_t* buffer, size_t size);
    Result<boost::blank> flush();

    int fd;
    struct timeval byteTimeout;
};


NativeDevice::NativeDevice(ImplPtr p) noexcept: impl(std::move(p)) {}

Result<NativeDevice> NativeDevice::doOpen(PortConfig portConfig) {
    using TvMsec = decltype(timeval().tv_usec);
    
    auto f = [] { return fail<NativeDevice>(toError(ErrorCode::DevInitFailed)); };
    
    if (portConfig.baud == Baud::NonStandard) {
        return fail<NativeDevice>(toError(ErrorCode::ConfigBadBaud));
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
    return ok(NativeDevice(std::move(impl)));
}

Result<size_t> NativeDevice::read(uint8_t* buffer, size_t maxRead, Timeout t) {
    return impl->read(buffer, maxRead, t);
}

Result<boost::blank> NativeDevice::write(const uint8_t* buffer, size_t size) {
    return impl->write(buffer, size);
}

Result<boost::blank> NativeDevice::flush() {
    return impl->flush();
}

NativeDevice::Impl::Impl(int fd, struct timeval byteTimeout)
    : fd(fd)
    , byteTimeout(byteTimeout) {
}

NativeDevice::Impl::~Impl() {
    ::close(fd);
}

Result<size_t> NativeDevice::Impl::read(uint8_t* buffer, size_t maxRead, Timeout t) {
    TRACE("===>");
    auto f = [] { return fail<size_t>(toError(ErrorCode::DevReadError)); };
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
        return fail<size_t>(toError(ErrorCode::DevReadTimeout));
    }
    return ok(static_cast<size_t>(cur - buffer));
}

Result<boost::blank> NativeDevice::Impl::write(const uint8_t* buffer, size_t size) {
    auto f = []{ return fail<b::blank>(toError(ErrorCode::DevWriteError)); };
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

Result<boost::blank> NativeDevice::Impl::flush() {
    auto f = []{ return fail<b::blank>(toError(ErrorCode::DevWriteError)); };
    call(f, ::tcflush, fd, TCIOFLUSH);
    return ok(boost::blank{});
}
