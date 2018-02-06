#pragma once

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/buffer.h>
#include <spacket/bind.h>

#include <memory>

template <class Impl>
class SerialDeviceBase {
public:
    template<typename Buffer>
    Result<Buffer> read(Buffer b, Timeout t) {
        return
        impl().read(b.begin(), b.size(), t) >=
        [&](size_t size) {
            return b.prefix(size);
        };
    }

    template<typename Buffer>
    Result<boost::blank> write(const Buffer& b) {
        return impl().write(b.begin(), b.size());
    }

    // not every platform support write with timeout
    // so there may be error here about missing impl().write()
    template<typename Buffer>
    Result<boost::blank> write(const Buffer& b, Timeout t) {
        return impl().write(b.begin(), b.size(), t);
    }
    
protected:
    ~SerialDeviceBase() {}

private:
    Impl& impl() { return static_cast<Impl&>(*this); }
};
