#pragma once

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/buffer.h>

#include <memory>

template <class Impl>
class SerialDeviceBase {
public:
    template<typename Buffer>
    Result<Buffer> read(Timeout t, Buffer b) {
        auto fail = idFail<Buffer>;
        rcallv(size, fail, impl().read(t, b.begin(), b.size()));
        return b.prefix(size);
    }

    template<typename Buffer>
    Result<boost::blank> write(const Buffer& b) {
        auto fail = idFail<boost::blank>;
        rcall(fail, impl().write(b.begin(), b.size()));
        return ok(boost::blank());
    }
    
protected:
    ~SerialDeviceBase() {}

private:
    Impl& impl() { return static_cast<Impl&>(*this); }
};
