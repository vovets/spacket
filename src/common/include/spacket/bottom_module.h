#pragma once

#include <spacket/module.h>
#include <spacket/driver.h>


struct BottomModule: public Module {
    Driver2& driver;

    BottomModule(Driver2& driver)
        : driver(driver)
    {
    }

    Result<Void> service() {
        return
        driver.serviceRx() >
        [&]() {
            if (driver.rxReady()) {
                return
                driver.rx() >=
                [&](Buffer&& b) {
                    return up(std::move(b));
                };
            }
            return ok();
        } >
        [&]() {
            for (;;) {
                auto r = executor().executeOne();
                if (isFail(r)) { return fail(getFailUnsafe(r)); }
                bool thereAreMore = getOkUnsafe(r);
                if (!thereAreMore) { return ok(); }
            }
            return ok();
        };
            
    }
    
    Result<Void> up(Buffer&& b) override {
        cpm::dpb("Bottom::up|", &b);
        return deferUp(std::move(b));
    }
    
    Result<Void> down(Buffer&& b) override {
        cpm::dpb("Bottom::down|", &b);
        if (!driver.txReady()) {
            return
            executor().defer(
                DeferredProc(
                    [&,buffer=std::move(b)] () mutable {
                        return this->down(std::move(buffer));
                    }
                ));
        }
        return
        driver.serviceTx()
        > [&]() { return driver.tx(std::move(b)); };
    }
};
