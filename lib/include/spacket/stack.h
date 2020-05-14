#pragma once

#include <spacket/driver.h>
#include <spacket/util/thread_error_report.h>


template <typename Buffer>
struct DriverServiceT: ModuleT<Buffer> {
    using Driver = DriverT<Buffer>;
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Module::upper;
    using Module::lower;

    Driver& driver;

    DriverServiceT(Driver& driver): driver(driver) {}

    Result<DeferredProc> up(Buffer&& buffer) override {
        cpm::dpl("DriverServiceT::up|");
        return
        upper(*this) >=
        [&] (Module* m) {
            return ok(DeferredProc(std::move(buffer), *m, &Module::up));
        };
    }

    Result<DeferredProc> down(Buffer&& buffer) override {
        cpm::dpb("DriverServiceT::down|", &buffer);
        {
            auto guard = makeGuard([&]() { driver.lock(); }, [&]() { driver.unlock(); });
            if (driver.txRequestQueue().put(buffer)) {
                return fail<DeferredProc>(toError(ErrorCode::ModulePacketConsumed));
            }
        }
        return fail<DeferredProc>(toError(ErrorCode::ModuleTxQueueFull));
    }
};

template <typename Buffer>
class StackT {
    using Driver = DriverT<Buffer>;
    using Stack = StackT<Buffer>;
    using DriverService = DriverServiceT<Buffer>;
    using ModuleList = ModuleListT<Buffer>;
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Queue = QueueT<Buffer>;
    using Ring = RingT<Buffer, 4>;

private:
    ModuleList moduleList;
    Ring rxCompleteRing;
    Ring txCompleteRing;
    Driver& driver;
    DriverService driverService;
    

public:
    StackT(Driver& driver)
        : driver(driver)
        , driverService(driver)
    {
        push(driverService);
        driver.start(rxCompleteRing, txCompleteRing);
    }

    ~StackT() {
        driver.stop();
    }

    void push(Module& m) {
        m.moduleList = &moduleList;
        moduleList.push_back(m);
    }

    void tick() {
        releaseTxBuffers();
        allocateRxBuffers();
        pollRx() >=
        [&](Buffer&& b) {
            cpm::dpb("StackT::tick|", &b);
            allocateRxBuffers();
            return runModules(std::move(b));
        } <=
        [&](Error e) {
            if (e != toError(ErrorCode::ModuleRxEmpty)) {
                return threadErrorReport(e);
            }
            return ok();
        // } <=
        // [&](Error) {
        //     chThdSleep(toSystime(std::chrono::milliseconds(1000)));
        //     return ok();
        };
    }

private:
    Result<Void> runModules(Buffer&& b) {
        if (moduleList.empty()) { return fail(toError(ErrorCode::ModuleNoModules)); }
        Result<DeferredProc> r = ok(
            DeferredProc(
                std::move(b),
                *moduleList.begin(),
                &Module::up
            ));
        while (isOk(r)) {
            r = getOkUnsafe(std::move(r))();
        }

        return
        std::move(r) >
        []() { return ok(); } <= // it is here just to convert the success type, never executed
        [&] (Error e) {
            if (e == toError(ErrorCode::ModulePacketConsumed)) {
                return ok();
            }
            return fail(e);
        };
    }

    auto driverGuard() {
        return makeGuard([&]() { driver.lock(); }, [&]() { driver.unlock(); });
    }

    void allocateRxBuffers() {
        Result<Void> r = ok();
        while (isOk(r)) {
            r =
            Buffer::create(Buffer::maxSize()) >=
            [&](Buffer&& b) {
                auto guard = driverGuard();
                if (driver.rxRequestQueue().put(b)) {
                    return ok();
                }
                return fail(toError(ErrorCode::StackRxRequestFull));
            } <=
            [&] (Error e) {
                if (e == toError(ErrorCode::StackRxRequestFull)
                || e == toError(ErrorCode::GuardedPoolOutOfMem)) {
                    return fail(e);
                }
                return threadErrorReport(e);
            };
        }
    }

    void releaseTxBuffers() {
        while (true) {
            auto guard = driverGuard();
            if (txCompleteRing.empty()) { break; }
            Buffer tmp = extract(txCompleteRing.tail());
            txCompleteRing.eraseTail();
            guard.unlock();
            cpm::dpb("StackT::releaseTxBuffer|", &tmp);
        }
    }
            
    Result<Buffer> pollRx() {
        auto guard = driverGuard();
        if (rxCompleteRing.empty()) {
            return fail<Buffer>(toError(ErrorCode::ModuleRxEmpty));
        }
        auto r = ok(extract(rxCompleteRing.tail()));
        rxCompleteRing.eraseTail();
        return std::move(r);
    }
};
