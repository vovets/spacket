#pragma once

#include <spacket/buffer.h>
#include <spacket/result.h>

enum class PacketizerNeedSync { Yes, No };

template <typename Buffer>
class PacketizerT {
public:
    enum Result { Continue, Finished, Overflow };

private:
    enum State { Sync, SkipDelim, Within };
    
public:
    static ::Result<PacketizerT<Buffer>> create(PacketizerNeedSync needSync) {
        return
        Buffer::create(Buffer::maxSize()) >=
        [&](Buffer&& b) {
            return ok(PacketizerT<Buffer>(std::move(b), needSync));
        };
    }
    
    PacketizerT(PacketizerT&& src)
        : state(src.state)
        , out(std::move(src.out))
        , next(out.begin())
    {
        src.next = nullptr;
    }

    PacketizerT& operator=(PacketizerT&& src) noexcept {
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
    PacketizerT(Buffer&& b, PacketizerNeedSync needSync)
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
