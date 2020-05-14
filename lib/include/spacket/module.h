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
Buffer extract(boost::optional<Buffer>& opt) {
    if (!opt) { FATAL_ERROR("extract"); }
    Buffer tmp = std::move(*opt);
    opt = {};
    return tmp;
}

template <typename Buffer>
struct ModuleT;

template <typename Buffer>
struct DeferredProcT {
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Func = Result<DeferredProc> (Module::*)(Buffer&&);

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

    Result<DeferredProc> operator()() {
        return (module->*func)(std::move(buffer));
    }
};

template <typename Buffer>
using ModuleListT = boost::intrusive::list<ModuleT<Buffer>>;

template <typename Buffer>
struct ModuleT: bi::list_base_hook<bi::link_mode<bi::normal_link>> {
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using ModuleList = ModuleListT<Buffer>;

    ModuleList* moduleList = nullptr;
    
    Result<Module*> lower(Module& m) {
        auto it = typename ModuleList::reverse_iterator(moduleList->iterator_to(m));
        if (it == moduleList->rend()) {
            return fail<Module*>(toError(ErrorCode::ModuleNoLower));
        }
        return ok(&(*it));
    }
    
    Result<Module*> upper(Module& m) {
        auto it = ++moduleList->iterator_to(m);
        if (it == moduleList->end()) {
            return fail<Module*>(toError(ErrorCode::ModuleNoUpper));
        }
        return ok(&(*it));
    }    

    virtual Result<DeferredProc> up(Buffer&& b) = 0;
    virtual Result<DeferredProc> down(Buffer&& b) = 0;
};
