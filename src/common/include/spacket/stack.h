#pragma once

#include <spacket/allocator.h>
#include <spacket/driver.h>
#include <spacket/module.h>
#include <spacket/log_error.h>
#include <spacket/cpm_debug.h>
#include <spacket/guard_utils.h>


struct DriverService: Module {
    using Module::ops;

    Driver& driver;

    DriverService(Driver& driver): driver(driver) {}

    Result<Void> up(Buffer&& buffer) override {
        cpm::dpb("DriverService::up|", &buffer);
        return
        ops->upper(*this) >=
        [&] (Module* m) {
            return ops->deferProc(makeProc(std::move(buffer), *m, &Module::up));
        };
    }

    Result<Void> down(Buffer&& buffer) override {
        cpm::dpb("DriverService::down|", &buffer);
        {
            auto guard = makeGuard([&]() { driver.lock(); }, [&]() { driver.unlock(); });
            if (driver.txRequestQueue().put(buffer)) {
                return ok();
            }
        }
        return { toError(ErrorCode::ModuleTxQueueFull) };
    }
};

template <std::size_t IORingCapacity, std::size_t ProcessRingCapacity>
class StackT: ModuleOps {
    using Queue = QueueT<Buffer>;
    using Stack = StackT<IORingCapacity, ProcessRingCapacity>;
    using IORing = RingT<DeferredProc, IORingCapacity>;
    using ProcessRing = RingT<DeferredProc, ProcessRingCapacity>;

    struct RxCompleteQueue: public Queue {
        Stack& stack;

        RxCompleteQueue(Stack& stack): stack(stack) {}
        
        void operator delete(void*, std::size_t) {}

        bool canPut() const override {
            return stack.ioRing.canPut();
        }

        bool put(Buffer& b) { return stack.rxCompletePut(b); }
    };
            
    struct TxCompleteQueue: public Queue {
        Stack& stack;

        TxCompleteQueue(Stack& stack): stack(stack) {}

        void operator delete(void*, std::size_t) {}

        bool canPut() const override {
            return stack.ioRing.canPut();
        }

        bool put(Buffer& b) { return stack.txCompletePut(b); }
    };
    
private:
    alloc::Allocator& allocator;
    ModuleList moduleList;
    IORing ioRing;
    ProcessRing processRing;
    RxCompleteQueue rxCompleteQueue;
    TxCompleteQueue txCompleteQueue;
    Driver& driver;
    DriverService driverService;

public:
    StackT(alloc::Allocator& allocator, Driver& driver)
        : allocator(allocator)
        , rxCompleteQueue(*this)
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
        if (ioRing.empty()) { return; }
        DeferredProc dp = std::move(ioRing.tail());
        ioRing.eraseTail();
        guard.unlock();
        if (!processRing.put(dp)) { FATAL_ERROR("tick"); }
        process();
    }

private:
    void process() {
        cpm::dpl("StackT::process|");
        while (!processRing.empty()) {
            DeferredProc dp = std::move(processRing.tail());
            processRing.eraseTail();
            dp() <= logError;
        }
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

    Result<Void> deferIO(DeferredProc&& dp) override {
        auto guard = driverGuard();

        if (ioRing.full()) return {toError(ErrorCode::StackIORingFull) };

        if (!ioRing.put(dp)) {
            FATAL_ERROR("StackT::defer|");
        }
        
        return ok();
    }

    Result<Void> deferProc(DeferredProc&& dp) override {
        if (!processRing.put(dp)) return { toError(ErrorCode::StackProcRingFull)};
        return ok();
    }

    // called from interrupt context
    bool rxCompletePut(Buffer& b) {
        cpm::dpl("StackT::rxCompletePut|%X", b.id());

        if (ioRing.full()) { return false; }
        
        DeferredProc dp(
            [&,buffer=std::move(b),module=&moduleList.front()] () mutable {
                this->allocateRxBuffers();
                return module->up(std::move(buffer));
            });
        
        if (!ioRing.put(dp)) {
            FATAL_ERROR("rxCompletePut");
        }
        
        return true;
    }

    // called from interrupt context
    bool txCompletePut(Buffer& b) {
        cpm::dpl("StackT::txCompletePut|%X", b.id());

        if (ioRing.full()) { return false; }
        
        DeferredProc dp(
            [&,buffer=std::move(b)] () mutable -> Result<Void> {
                cpm::dpl("StackT::txCompletePut|lambda|%X", buffer.id());
                Buffer tmp = std::move(buffer);
                return ok();
            });
        
        if (!ioRing.put(dp)) {
            FATAL_ERROR("txCompletePut");
        }
        
        return true;
    }

    void allocateRxBuffers() {
        Result<Void> r = ok();
        while (isOk(r)) {
            r =
            Buffer::create(allocator) >=
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
                return logError(e);
            };
        }
    }
};
