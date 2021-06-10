#pragma once

#include <spacket/module.h>
#include <spacket/cobs.h>


struct CobsModule: Module {
    Result<Void> up(Buffer&& b) override {
        cpm::dpb("CobsModule::up|", &b);
        return
        cobs::unstuff(std::move(b)) >=
        [&](Buffer&& u) {
            return deferUp(std::move(u));
        };
    }

    Result<Void> down(Buffer&& b) override {
        cpm::dpb("CobsModule::down|", &b);
        return
        cobs::stuff(std::move(b)) >=
        [&](Buffer&& s) {
            return deferDown(std::move(s));
        };
    }
};
