#pragma once

// Composable Protocol Modules

#include <spacket/deferred_proc.h>

#include <boost/intrusive/list.hpp>


namespace bi = boost::intrusive;

struct Module;

auto makeProc(
    Buffer&& b,
    Module& m,
    Result<Void> (Module::*f)(Buffer&&))
{
    return DeferredProc(
        [buffer=std::move(b), module=&m, func=f]() mutable {
            return (module->*func)(std::move(buffer));
        });
}

struct ModuleOps {
    virtual Result<Module*> lower(Module& m) = 0;
    virtual Result<Module*> upper(Module& m) = 0;
    virtual Result<Void> deferIO(DeferredProc&& dp) = 0;
    virtual Result<Void> deferProc(DeferredProc&& dp) = 0;
};

using ModuleList = boost::intrusive::list<Module>;

struct Module: bi::list_base_hook<bi::link_mode<bi::normal_link>> {
    ModuleOps* ops = nullptr;
    
    virtual Result<Void> up(Buffer&& b) = 0;
    virtual Result<Void> down(Buffer&& b) = 0;
};
