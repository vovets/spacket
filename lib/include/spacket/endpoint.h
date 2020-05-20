#pragma once

#include <spacket/queue.h>
#include <spacket/module.h>

struct SimpleAddress {};

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
struct EndpointBaseT {
    using Queue = QueueT<Buffer>;
    using Module = ModuleT<Buffer>;
    
    BufferBoxT<Buffer> readBox;
    BufferBoxT<Buffer> writeBox;

    Module* module;

    EndpointBaseT(Module* module): module(module) {}

    Queue& readQueue() { return readBox; }
    Queue& writeQueue() { return writeBox; }
};

template <typename Buffer, typename Proto>
struct EndpointHandleT {
    using Endpoint = EndpointBaseT<Buffer>;

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

template <typename Buffer>
struct EndpointSimpleT: EndpointBaseT<Buffer> {
    using Module = ModuleT<Buffer>;
    using Base = EndpointBaseT<Buffer>;
    EndpointSimpleT(Module* module): Base(module) {}
};

template <typename Buffer>
struct EndpointServiceT: ModuleT<Buffer> {
    using This = EndpointServiceT<Buffer>;
    using Address = SimpleAddress;
    static constexpr std::size_t maxEndpoint = 1;
    using Endpoint = EndpointSimpleT<Buffer>;
    using EndpointBase = EndpointBaseT<Buffer>;
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
        
    void destroy(EndpointBase* endpoint_) {
        if (endpoint_ != endpoint || endpoint == nullptr) {
            FATAL_ERROR("EndpointServiceT::destroy|");
            return;
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

template <typename Buffer, typename Endpoint>
Result<Buffer> write(Endpoint& endpoint) {
}
