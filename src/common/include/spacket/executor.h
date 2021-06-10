#pragma once

#include <spacket/deferred_proc.h>
#include <spacket/ring.h>
#include <spacket/cpm_debug.h>


struct ExecutorI {
    virtual Result<Void> defer(DeferredProc&& dp) = 0;
    // returns true if there are more to execute
    virtual Result<bool> executeOne() = 0;
    virtual ~ExecutorI() {}
};

template <std::size_t CAPACITY>
class ExecutorT: public ExecutorI {
    using Ring = RingT<DeferredProc, CAPACITY>;
    
    Ring ring;

    // returns true if there are more to execute
    Result<bool> executeOne() override {
        if (!ring.empty()) {
            cpm::dpl("ExecutorT::executeOne|exec");
            DeferredProc dp = std::move(ring.tail());
            ring.eraseTail();
            return
            dp() > [&]() { return ok(!ring.empty()); };
        }
        return ok(false);
    }   

    Result<Void> defer(DeferredProc&& dp) override {
        if (!ring.put(dp)) {
            return {toError(ErrorCode::ExecutorRingFull)};
        }
        return ok();
    }
};
