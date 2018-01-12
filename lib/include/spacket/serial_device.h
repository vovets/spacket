#pragma once

#include <spacket/result.h>
#include <spacket/port_config.h>
#include <spacket/time_utils.h>
#include <spacket/buffer.h>

#include <memory>

class SerialDevice {
public:
    static Result<SerialDevice> open(PortConfig portConfig);
    SerialDevice(SerialDevice&&);
    ~SerialDevice();

    template<typename Buffer>
    Result<Buffer> read(Timeout t, Buffer b) {
        auto fail = idFail<Buffer>;
        rcallv(size, fail, read(t, b.begin(), b.size()));
        return b.prefix(size);
    }

    template<typename Buffer>
    Result<boost::blank> write(const Buffer& b) {
        auto fail = idFail<boost::blank>;
        rcall(fail, write(b.begin(), b.size()));
        return ok(boost::blank());
    }
    
    Result<boost::blank> flush();

private:
    Result<size_t> read(Timeout t, uint8_t* buffer, size_t maxRead);
    Result<boost::blank> write(uint8_t* buffer, size_t size);

private:
    struct Impl;
    using ImplPtr = std::unique_ptr<Impl>;
    SerialDevice(ImplPtr p);
    ImplPtr impl;
};
