#pragma once

#include <spacket/buffer.h>
#include <spacket/result.h>

enum class PacketizerNeedSync { Yes, No };

class Packetizer {
public:
    enum Result { Continue, Finished, Overflow };

private:
    enum State { Sync, SkipDelim, Within };
    
public:
    static ::Result<Packetizer> create(alloc::Allocator& allocator, PacketizerNeedSync needSync) {
        return
        Buffer::create(allocator) >=
        [&](Buffer&& b) {
            return ok(Packetizer(std::move(b), needSync));
        };
    }

    Packetizer(const Packetizer&) = delete;

    Packetizer(Packetizer&& src)
        : state(src.state)
        , out(std::move(src.out))
        , next(out.begin())
    {
        src.next = nullptr;
    }

    Packetizer& operator=(Packetizer&& src) noexcept {
        state = src.state;
        out = std::move(src.out);
        next = out.begin();
        src.next = nullptr;
        return *this;
    }

    Result consume(uint8_t byte) {
        switch(state) {
            case Sync: return consumeSync(byte);
            case SkipDelim: return consumeSkipDelim(byte);
            case Within: return consumeWithin(byte);
        }
        // should never reach here
        return Overflow;
    }

    Buffer release() {
        out.resize(next - out.begin());
        return std::move(out);
    }

private:
    Packetizer(Buffer&& b, PacketizerNeedSync needSync)
        : state(needSync == PacketizerNeedSync::Yes ? Sync : SkipDelim)
        , out(std::move(b))
        , next(out.begin())
    {}

    Result consumeSync(uint8_t byte) {
        if (byte == 0) {
            state = SkipDelim;
        }
        return Continue;
    }
    
    Result consumeSkipDelim(uint8_t byte) {
        if (byte == 0) {
            return Continue;
        } else {
            if (next == out.end()) {
                return Overflow;
            } else {
                *next++ = byte;
                state = Within;
                return Continue;
            }
        }
    }

    Result consumeWithin(uint8_t byte) {
        if (byte == 0) {
            return Finished;
        } else {
            if (next == out.end()) {
                return Overflow;
            } else {
                *next++ = byte;
                return Continue;
            }
        }
    }

private:
    State state;
    Buffer out;
    uint8_t* next;
};
