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
    ExecutorI& executor;

public:
    Stack(ExecutorI& executor): executor(executor)
    {
    }

    ~Stack() {
    }

    void push(Module& m) {
        m.attach(this, &executor);
        moduleList.push_back(m);
    }

private:
    Result<Void> deferDown(Module& from, Buffer&& b) override {
        auto it = typename ModuleList::reverse_iterator(moduleList.iterator_to(from));
        if (it == moduleList.rend()) {
            return fail(toError(ErrorCode::StackNoLowerModule));
        }
        auto dp = DeferredProc(
            [&,buffer=std::move(b),module=&(*it)] () mutable {
                return module->down(std::move(buffer));
            }
        );
        return executor.defer(std::move(dp));
    }
    
    Result<Void> deferUp(Module& m, Buffer&& b) override {
        auto it = ++moduleList.iterator_to(m);
        if (it == moduleList.end()) {
            return fail(toError(ErrorCode::StackNoUpperModule));
        }
        auto dp = DeferredProc(
            [&,buffer=std::move(b),module=&(*it)] () mutable {
                return module->up(std::move(buffer));
            }
        );
        return executor.defer(std::move(dp));
    }    
};
