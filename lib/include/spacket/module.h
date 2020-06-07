#pragma once

// Composable Protocol Modules

#include <spacket/time_utils.h>
#include <spacket/memory_utils.h>
#include <spacket/guard_utils.h>
#include <spacket/buffer_debug.h>
#include <spacket/deferred_proc.h>

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
struct ModuleT;

template <typename Buffer>
auto makeProc(
    Buffer&& b,
    ModuleT<Buffer>& m,
    Result<Void> (ModuleT<Buffer>::*f)(Buffer&&))
{
    return DeferredProcT<Buffer>(
        [buffer=std::move(b), module=&m, func=f]() mutable {
            return (module->*func)(std::move(buffer));
        });
}

template <typename Buffer>
struct ModuleOpsT {
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;

    virtual Result<Module*> lower(Module& m) = 0;
    virtual Result<Module*> upper(Module& m) = 0;
    virtual Result<Void> deferIO(DeferredProc&& dp) = 0;
    virtual Result<Void> deferProc(DeferredProc&& dp) = 0;
};

template <typename Buffer>
using ModuleListT = boost::intrusive::list<ModuleT<Buffer>>;

template <typename Buffer>
struct ModuleT: bi::list_base_hook<bi::link_mode<bi::normal_link>> {
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using ModuleList = ModuleListT<Buffer>;
    using ModuleOps = ModuleOpsT<Buffer>;

    ModuleOps* ops = nullptr;
    
    virtual Result<Void> up(Buffer&& b) = 0;
    virtual Result<Void> down(Buffer&& b) = 0;
};
