#pragma once

#include <spacket/buffer.h>


struct Driver2 {
    enum class Event: std::uint8_t {
        RX_READY         = 0b00000001,
        TX_READY         = 0b00000010,
        RX_NEEDS_SERVICE = 0b00000100,
        TX_NEEDS_SERVICE = 0b00001000,
    };

    struct Events {
        std::uint8_t events = 0;

        bool has(Event e) { return static_cast<std::uint8_t>(e) & events; }
    };

    virtual ~Driver2() {}
    virtual void start() = 0;
    virtual Events poll() = 0;
    virtual Result<Void> serviceRx() = 0;
    virtual Result<Buffer> rx() = 0;
    virtual Result<Void> serviceTx() = 0;
    virtual Result<Void> tx(Buffer&& b) = 0;
};
