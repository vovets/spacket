#pragma once

#include <spacket/buffer.h>
#include <spacket/result.h>

template <typename Buffer>
class PacketizerT {
public:
    enum Result { Continue, Finished, Overflow };
    
public:
    PacketizerT(Buffer& b)
        : out(&b)
        , next(out->begin())
    {}

    PacketizerT(PacketizerT&& src)
        : out(src.out)
        , next(out->begin())
    {
        src.out = nullptr;
        src.next = nullptr;
    }

    PacketizerT& operator=(PacketizerT&& rhs) noexcept {
        out = rhs.out;
        next = out->begin();
        rhs.out = nullptr;
        rhs.next = nullptr;
        return *this;
    }

    Result consume(uint8_t byte) {
        if (next == out->begin()) {
            return consumeBegin(byte);
        } else {
            if (next == out->end()) {
                return consumeEnd(byte);
            } else {
                return consumeWithin(byte);
            }
        }
    }

    size_t size() const {
        return next - out->begin();
    }

private:
    Result consumeBegin(uint8_t byte) {
        if (byte == 0) {
            return Continue;
        } else {
            if (next == out->end()) {
                return Overflow;
            } else {
                *next++ = byte;
                return Continue;
            }
        }
    }

    Result consumeEnd(uint8_t byte) {
        if (byte == 0) {
            return Finished;
        } else {
            return Overflow;
        }
    }

    Result consumeWithin(uint8_t byte) {
        if (byte == 0) {
            return Finished;
        } else {
            *next++ = byte;
            return Continue;
        }
    }

private:
    Buffer* out;
    uint8_t* next;
};
