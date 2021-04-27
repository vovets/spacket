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

struct Endpoint: Module {
    using Queue = QueueT<Buffer>;
    using Module::ops;
    
    BufferBoxT<Buffer> readBox;

    Result<Void> up(Buffer&& b_) override {
        cpm::dpl("Endpoint::up|");
        Buffer b = std::move(b_);
        if (!readBox.put(b)) {
            return { toError(ErrorCode::ModulePacketDropped) };
        }
        return ok();
    }
    
    Result<Void> down(Buffer&& b) override {
        cpm::dpl("Endpoint::down|");
        return
        ops->lower(*this) >=
        [&] (Module* m) {
            return ops->deferProc(makeProc(std::move(b), *m, &Module::down));
        };
    }

    Result<Buffer> read() {
        if (readBox.buffer) {
            cpm::dpb("Endpoint::read|", &*readBox.buffer);
            return ok(extract(readBox.buffer));
        }
        return { toError(ErrorCode::Timeout) };
    }

    Result<Void> write(Buffer b) {
        cpm::dpb("Endpoint::write|", &b);
        return
        ops->lower(*this) >=
        [&] (Module* m) {
            return ops->deferIO(makeProc(std::move(b), *m, &Module::down));
        };
    }
};