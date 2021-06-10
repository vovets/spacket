#pragma once

#include <spacket/module.h>
#include <spacket/crc.h>

struct CrcModule: Module {
    Result<Void> up(Buffer&& b) override {
        cpm::dpb("CrcModule::up|", &b);
        return
        crc::check(std::move(b)) >=
        [&](Buffer&& b) {
            return deferUp(std::move(b));
        };
    }

    Result<Void> down(Buffer&& b) override {
        cpm::dpb("CrcModule::down|", &b);
        return
        crc::append(std::move(b)) >=
        [&](Buffer&& b) {
            return deferDown(std::move(b));
        };
    }
};
