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
            if (!ops->defer(dp)) {
                return fail(toError(ErrorCode::Timeout));
            }
            return ok();
        };
    }
};

template <typename Buffer, typename Proto>
struct EndpointHandleT {
    using Endpoint = EndpointT<Buffer>;

    Endpoint* endpoint;
    Proto* proto;

    EndpointHandleT(Endpoint* endpoint, Proto* proto): endpoint(endpoint), proto(proto) {}

    ~EndpointHandleT() {
        if (endpoint != nullptr && proto != nullptr) {
            proto->destroy(endpoint);
        }
    }

    void swap(EndpointHandleT& other) {
        std::swap(endpoint, other.endpoint);
        std::swap(proto, other.proto);
    }

    EndpointHandleT(const EndpointHandleT&) = delete;
    EndpointHandleT& operator=(const EndpointHandleT&) = delete;

    EndpointHandleT(EndpointHandleT&& from): endpoint(from.endpoint), proto(from.proto) {
        from.endpoint = nullptr;
        from.proto = nullptr;
    }

    EndpointHandleT& operator=(EndpointHandleT&& other) {
        EndpointHandleT tmp(std::move(other));
        this->swap(tmp);
        return *this;
    }
};

struct SimpleAddress {};

template <typename Buffer>
struct EndpointServiceT: ModuleT<Buffer> {
    using This = EndpointServiceT<Buffer>;
    using Address = SimpleAddress;
    static constexpr std::size_t maxEndpoint = 1;
    using Endpoint = EndpointT<Buffer>;
    using EndpointStorage = Storage<Endpoint>;
    using EndpointHandle = EndpointHandleT<Buffer, This>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Module = ModuleT<Buffer>;
    using Module::upper;
    using Module::lower;

    EndpointStorage endpointStorage;
    Endpoint* endpoint;
    
    Endpoint* create(const Address&) {
        if (endpoint != nullptr) { return nullptr; }
        endpoint = new (&endpointStorage) Endpoint(this);
        return endpoint;
    }
        
    void destroy(Endpoint* endpoint_) {
        if (endpoint_ != endpoint || endpoint == nullptr) {
            FATAL_ERROR("EndpointServiceT::destroy|");
        }
        endpoint->~Endpoint();
        endpoint = nullptr;
    }

    Result<DeferredProc> up(Buffer&& b) override {
        cpm::dpb("EndpointServiceT::up|", &b);
        if (endpoint != nullptr) {
            if (endpoint->readQueue().canPut()) {
                endpoint->readQueue().put(b);
                cpm::dpl("EndpointServiceT::up|consumed");
            } else {
                cpm::dpl("EndpointServiceT::up|endpoint full, packet dropped");
                return fail<DeferredProc>(toError(ErrorCode::ModulePacketDropped));
            }
        } else {
            cpm::dpl("EndpointServiceT::up|no endpoint, packet dropped");
            return fail<DeferredProc>(toError(ErrorCode::ModulePacketDropped));
        }
        return fail<DeferredProc>(toError(ErrorCode::ModulePacketConsumed));
    }

    Result<DeferredProc> down(Buffer&& b) override {
        cpm::dpb("EndpointServiceT::down|", &b);
        return
        lower(*this) >=
        [&] (Module* m) {
            return defer(std::move(b), *m, &Module::down);
        };
    }
};

template <typename Buffer, typename Proto, typename Address>
EndpointHandleT<Buffer, Proto> createHandle(Proto& proto, const Address& address) {
    return EndpointHandleT<Buffer, Proto>(proto.create(address), &proto);
}
