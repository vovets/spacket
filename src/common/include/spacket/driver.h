#pragma once

#include <spacket/buffer.h>


struct Driver2 {
    virtual ~Driver2() {}
    virtual void start() = 0;
    virtual Result<Void> serviceRx() = 0;
    virtual bool rxReady() = 0;
    virtual Result<Buffer> rx() = 0;
    virtual Result<Void> serviceTx() = 0;
    virtual bool txReady() = 0;
    virtual Result<Void> tx(Buffer&& b) = 0;
};
