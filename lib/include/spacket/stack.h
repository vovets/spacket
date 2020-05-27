#pragma once

#include <spacket/driver.h>
#include <spacket/util/thread_error_report.h>


template <typename Buffer>
struct DriverServiceT: ModuleT<Buffer> {
    using Driver = DriverT<Buffer>;
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Module::ops;

    Driver& driver;

    DriverServiceT(Driver& driver): driver(driver) {}

    Result<DeferredProc> up(Buffer&& buffer) override {
        cpm::dpb("DriverServiceT::up|", &buffer);
        auto r =
        ops->upper(*this) >=
        [&] (Module* m) {
            return ok(makeProc(std::move(buffer), *m, &Module::up));
        };
        return r;
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

template <typename Buffer, std::size_t RingCapacity>
class StackT: ModuleOpsT<Buffer> {
    using Driver = DriverT<Buffer>;
    using Stack = StackT<Buffer, RingCapacity>;
    using DriverService = DriverServiceT<Buffer>;
    using ModuleList = ModuleListT<Buffer>;
    using Module = ModuleT<Buffer>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Queue = QueueT<Buffer>;
    using Ring = RingT<DeferredProc, RingCapacity>;

    struct RxCompleteQueue: public Queue {
        Stack& stack;

        RxCompleteQueue(Stack& stack): stack(stack) {}
        
        void operator delete(void*, std::size_t) {}

        bool canPut() const override {
            return stack.ring.canPut();
        }

        bool put(Buffer& b) { return stack.rxCompletePut(b); }
    };
            
    struct TxCompleteQueue: public Queue {
        Stack& stack;

        TxCompleteQueue(Stack& stack): stack(stack) {}

        void operator delete(void*, std::size_t) {}

        bool canPut() const override {
            return stack.ring.canPut();
        }

        bool put(Buffer& b) { return stack.txCompletePut(b); }
    };
    
private:
    ModuleList moduleList;
    Ring ring;
    RxCompleteQueue rxCompleteQueue;
    TxCompleteQueue txCompleteQueue;
    Driver& driver;
    DriverService driverService;

public:
    StackT(Driver& driver)
        : rxCompleteQueue(*this)
        , txCompleteQueue(*this)
        , driver(driver)
        , driverService(driver)
    {
        push(driverService);
    }

    ~StackT() {
        driver.stop();
    }

    void push(Module& m) {
        m.ops = this;
        bool wasEmpty = moduleList.empty();
        moduleList.push_back(m);
        if (wasEmpty) {
            driver.start(rxCompleteQueue, txCompleteQueue);
        }
    }

    void tick() {
        allocateRxBuffers();
        auto guard = driverGuard();
        if (ring.empty()) { return; }
        DeferredProc dp = std::move(ring.tail());
        ring.eraseTail();
        guard.unlock();
        exec(dp);
    }

private:
    void exec(DeferredProc& p) {
        cpm::dpl("StackT::exec|");
        Result<DeferredProc> r = ok(std::move(p));
        while (isOk(r)) {
            r = getOkUnsafe(std::move(r))();
        }

        std::move(r) >
        []() { return ok(); } <= // it is here just to convert the success type, never executed
        [&] (Error e) {
            if (e == toError(ErrorCode::ModulePacketConsumed)) {
                return ok();
            }
            return fail(e);
        } <=
        threadErrorReport;
    }

    auto driverGuard() {
        return makeGuard([&]() { driver.lock(); }, [&]() { driver.unlock(); });
    }

    Result<Module*> lower(Module& m) override {
        auto it = typename ModuleList::reverse_iterator(moduleList.iterator_to(m));
        if (it == moduleList.rend()) {
            return fail<Module*>(toError(ErrorCode::ModuleNoLower));
        }
        return ok(&(*it));
    }
    
    Result<Module*> upper(Module& m) override {
        auto it = ++moduleList.iterator_to(m);
        if (it == moduleList.end()) {
            return fail<Module*>(toError(ErrorCode::ModuleNoUpper));
        }
        return ok(&(*it));
    }    

    Result<Void> defer(DeferredProc& dp) override {
        auto guard = driverGuard();

        if (ring.full()) return { toError(ErrorCode::StackRingFull) };

        if (!ring.put(dp)) {
            FATAL_ERROR("StackT::defer|");
        }
        
        return ok();
    }

    // called from interrupt context
    bool rxCompletePut(Buffer& b) {
        cpm::dpl("StackT::rxCompletePut|%X", b.id());

        if (ring.full()) { return false; }
        
        DeferredProc dp(
            [&,buffer=std::move(b),module=&moduleList.front()] () mutable {
                this->allocateRxBuffers();
                return module->up(std::move(buffer));
            });
        
        if (!ring.put(dp)) {
            FATAL_ERROR("rxCompletePut");
        }
        
        return true;
    }

    // called from interrupt context
    bool txCompletePut(Buffer& b) {
        cpm::dpl("StackT::txCompletePut|%X", b.id());

        if (ring.full()) { return false; }
        
        DeferredProc dp(
            [&,buffer=std::move(b)] () mutable {
                cpm::dpl("StackT::txCompletePut|lambda|%X", buffer.id());
                Buffer tmp = std::move(buffer);
                return fail<DeferredProc>(toError(ErrorCode::ModulePacketConsumed));
            });
        
        if (!ring.put(dp)) {
            FATAL_ERROR("txCompletePut");
        }
        
        return true;
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
};
