#pragma once

#include <spacket/allocator.h>
#include <spacket/driver.h>
#include <spacket/module.h>
#include <spacket/executor.h>
#include <spacket/log_error.h>
#include <spacket/cpm_debug.h>
#include <spacket/guard_utils.h>


class Stack: StackOps {
    ModuleList moduleList;

public:
    void push(Module& m) {
        m.attach(this);
        moduleList.push_back(m);
    }

private:
    Module* upper_(Module& from) {
        auto it = ++moduleList.iterator_to(from);
        if (it == moduleList.end()) {
            return nullptr;
        }
        return &(*it);
    }
        
    Module* lower_(Module& from) {
        auto it = typename ModuleList::reverse_iterator(moduleList.iterator_to(from));
        if (it == moduleList.rend()) {
            return nullptr;
        }
        return &(*it);
    }

    Result<Module*> upper(Module& from) override {
        Module* m = upper_(from);
        if (m == nullptr) {
            return fail<Module*>(toError(ErrorCode::StackNoUpperModule));
        }
        return ok(std::move(m));
    }

    Result<Module*> lower(Module& from) override {
        Module* m = lower_(from);
        if (m == nullptr) {
            return fail<Module*>(toError(ErrorCode::StackNoLowerModule));
        }
        return ok(std::move(m));
    }
};
