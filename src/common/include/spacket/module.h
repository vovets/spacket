#pragma once

// Composable Protocol Modules

#include <spacket/executor.h>

#include <boost/intrusive/list.hpp>


namespace bi = boost::intrusive;

struct Module;

struct StackOps {
    virtual Result<Void> deferUp(Module& from, Buffer&& b) = 0;
    virtual Result<Void> deferDown(Module& from, Buffer&& b) = 0;
};

using ModuleList = boost::intrusive::list<Module>;

class Module: public bi::list_base_hook<bi::link_mode<bi::normal_link>> {
    StackOps* ops_ = nullptr;
    ExecutorI* executor_ = nullptr;

    StackOps* ops() {
        if (ops_ == nullptr) {
            FATAL_ERROR("Module::ops");
        }
        return ops_;
    }

protected:
    ExecutorI& executor() {
        if (executor_ == nullptr) {
            FATAL_ERROR("Module::executor");
        }
        return *executor_;
    }

    Result<Void> deferUp(Buffer&& b) {
        return ops()->deferUp(*this, std::move(b));
    }
        
    Result<Void> deferDown(Buffer&& b) {
        return ops()->deferDown(*this, std::move(b));
    }

public:
    void attach(StackOps* ops, ExecutorI* executor) {
        ops_ = ops;
        executor_ = executor;
    }
    
    virtual Result<Void> up(Buffer&& b) = 0;
    virtual Result<Void> down(Buffer&& b) = 0;
};
