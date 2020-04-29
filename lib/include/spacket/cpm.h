#pragma once

// Composable Protocol Modules

#include <spacket/time_utils.h>
#include <spacket/memory_utils.h>
#include <spacket/guard_utils.h>
#include <spacket/buffer_debug.h>

#include <boost/optional.hpp>
#include <boost/intrusive/list.hpp>


namespace bi = boost::intrusive;

namespace cpm {

#ifdef CPH_ENABLE_DEBUG_PRINT

    IMPLEMENT_DPL_FUNCTION
    IMPLEMENT_DPB_FUNCTION

#else

    IMPLEMENT_DPL_FUNCTION_NOP
    IMPLEMENT_DPB_FUNCTION_NOP

#endif

}

template <typename Buffer>
using StreamT = boost::optional<Buffer>;

template <typename Stream>
typename Stream::value_type extract(Stream& stream) {
    using Buffer = typename Stream::value_type;
    if (!stream) { FATAL_ERROR("extract"); }
    Buffer tmp = std::move(*stream);
    stream = {};
    return tmp;
}

template <typename Buffer>
struct ModuleT;

template <typename Buffer>
struct ContextOpsT {
    using Module = ModuleT<Buffer>;
    
    virtual Module* lower(Module& m) = 0;
    virtual Module* upper(Module& m) = 0;
};

template <typename Buffer>
struct DeferredProcT {
    using Module = ModuleT<Buffer>;
    using ContextOps = ContextOpsT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Func = Result<DeferredProc> (Module::*)(Buffer&&, ContextOps&);

    Buffer buffer;
    Module* module;
    Func func;

    DeferredProcT(Buffer&& buffer, Module& module, Func func)
        : buffer(std::move(buffer))
        , module(&module)
        , func(func)
    {}
    
    DeferredProcT(const DeferredProcT&) = delete;
    DeferredProcT(DeferredProcT&&) = default;

    Result<DeferredProc> operator()(ContextOps& ops) {
        return (module->*func)(std::move(buffer), ops);
    }
};

template <typename Buffer>
struct ModuleT: bi::list_base_hook<bi::link_mode<bi::normal_link>> {
    using ContextOps = ContextOpsT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    
    virtual Result<DeferredProc> up(Buffer&& b, ContextOps& ops) = 0;
    virtual Result<DeferredProc> down(Buffer&& b, ContextOps& ops) = 0;
};

template <typename Buffer>
using ModuleListT = boost::intrusive::list<ModuleT<Buffer>>;

struct SimpleAddress {};

template <typename Buffer>
struct PcbBaseT {
    using Stream = StreamT<Buffer>;

    Stream stream;
};

template <typename Buffer, typename Proto>
struct PcbHandleT {
    using Pcb = PcbBaseT<Buffer>;

    Pcb* pcb;
    Proto* proto;

    PcbHandleT(Pcb* pcb, Proto* proto): pcb(pcb), proto(proto) {}

    ~PcbHandleT() {
        if (pcb != nullptr && proto != nullptr) {
            proto->destroy(pcb);
        }
    }

    void swap(PcbHandleT& other) {
        std::swap(pcb, other.pcb);
        std::swap(proto, other.proto);
    }

    PcbHandleT(const PcbHandleT&) = delete;
    PcbHandleT& operator=(const PcbHandleT&) = delete;

    PcbHandleT(PcbHandleT&& from): pcb(from.pcb), proto(from.proto) {
        from.pcb = nullptr;
        from.proto = nullptr;
    }

    PcbHandleT& operator=(PcbHandleT&& other) {
        PcbHandleT tmp(std::move(other));
        this->swap(tmp);
        return *this;
    }
};

template <typename Buffer>
struct PcbSimpleT: PcbBaseT<Buffer> {
    using Base = PcbBaseT<Buffer>;
};

template <typename Buffer>
struct ProtoSimpleT: ModuleT<Buffer> {
    using This = ProtoSimpleT<Buffer>;
    using Address = SimpleAddress;
    static constexpr std::size_t maxPcb = 1;
    using Pcb = PcbSimpleT<Buffer>;
    using PcbBase = PcbBaseT<Buffer>;
    using PcbStorage = Storage<Pcb>;
    using PcbHandle = PcbHandleT<Buffer, This>;

    PcbStorage pcbStorage;
    Pcb* pcb;
    
    Pcb* create(const Address&) {
        if (pcb != nullptr) { return nullptr; }
        pcb = new (&pcbStorage) Pcb();
        return pcb;
    }
        
    void destroy(Pcb* pcb_) {
        if (pcb_ != pcb || pcb == nullptr) { return; }
        pcb->~Pcb();
        pcb = nullptr;
    }

    Result<Buffer> up(Buffer&& b) {
        if (pcb == nullptr) {
            return ok(std::move(b));
        }
        pcb->stream = std::move(b);
        return fail<Buffer>(toError(ErrorCode::ModulePacketConsumed));
    }

    Result<Buffer> down(Buffer&& b) {
        return ok(std::move(b));
    }
};

template <typename Buffer, typename Proto, typename Address>
PcbHandleT<Buffer, Proto> createHandle(Proto& proto, const Address& address) {
    return PcbHandleT<Buffer, Proto>(proto.create(address), &proto);
}
