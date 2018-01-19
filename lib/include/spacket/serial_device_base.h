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
    Result<Buffer> read(Timeout t, Buffer b) {
        return
        impl().read(t, b.begin(), b.size()) >>=
        [&](size_t size) {
            return b.prefix(size);
        };
    }

    template<typename Buffer>
    Result<boost::blank> write(const Buffer& b) {
        return
        impl().write(b.begin(), b.size()) >>=
        [&](boost::blank&&) {
            return ok(boost::blank{});
        };
    }
    
protected:
    ~SerialDeviceBase() {}

private:
    Impl& impl() { return static_cast<Impl&>(*this); }
};
