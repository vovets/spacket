#pragma once

#include "result.h"
#include "config.h"
#include "time_utils.h"
#include "buffer.h"

#include <memory>

class SerialDevice {
public:
    static Result<SerialDevice> open(PortConfig portConfig);
    SerialDevice(SerialDevice&&);
    ~SerialDevice();
    
    Result<Buffer> read(Timeout t, size_t maxSize);
    Result<boost::blank> write(const Buffer& b);
    Result<boost::blank> flush();
private:
    struct Impl;
    using ImplPtr = std::unique_ptr<Impl>;
    SerialDevice(ImplPtr p);
    ImplPtr impl;
};
