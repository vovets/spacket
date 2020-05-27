#pragma once

#include <spacket/queue.h>
#include <spacket/module.h>


template <typename Buffer>
Buffer extract(boost::optional<Buffer>& opt) {
    if (!opt) { FATAL_ERROR("extract"); }
    Buffer tmp = std::move(*opt);
    opt = {};
    return tmp;
}

template <typename Buffer>
struct BufferBoxT: public QueueT<Buffer> {
    boost::optional<Buffer> buffer;

    void operator delete(void*, std::size_t) {}

    bool put(Buffer& b) override {
        if (buffer) { return false; }
        buffer = std::move(b);
        return true;
    }

    bool canPut() const override {
        return !buffer;
    }
};

template <typename Buffer>
struct EndpointT: ModuleT<Buffer> {
    using Queue = QueueT<Buffer>;
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Module::ops;
    
    BufferBoxT<Buffer> readBox;

    Result<DeferredProc> up(Buffer&& b_) override {
        Buffer b = std::move(b_);
        if (!readBox.put(b)) {
            return { toError(ErrorCode::ModulePacketDropped) };
        }
        return { toError(ErrorCode::ModulePacketConsumed) };
    }
    
    Result<DeferredProc> down(Buffer&& b) override {
        return
        ops->lower(*this) >=
        [&] (Module* m) {
            return ok(makeProc(std::move(b), *m, &Module::down));
        };
    }

    Result<Buffer> read() {
        if (readBox.buffer) { return ok(extract(readBox.buffer)); }
        return { toError(ErrorCode::Timeout) };
    }

    Result<Void> write(Buffer b) {
        return
        ops->lower(*this) >=
        [&] (Module* m) {
            auto dp = makeProc(std::move(b), *m, &Module::down);
            return ops->defer(dp);
        };
    }
};
