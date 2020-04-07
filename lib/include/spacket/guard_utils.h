#pragma once


class ScopeGuardBase {
protected:
    ScopeGuardBase(): disarmed(false) {}
    ScopeGuardBase(ScopeGuardBase&& from): disarmed(from.disarmed) {
        from.disarmed = true;
    }
    
    bool disarmed = false;
};

template <typename Function>
class ScopeGuardT: public ScopeGuardBase {
public:
    explicit ScopeGuardT(Function f): function(f) {}
    ~ScopeGuardT() { if (!disarmed) { function(); } }

    ScopeGuardT(const ScopeGuardT&) = delete;
    ScopeGuardT& operator=(const ScopeGuardT&) = delete;

    ScopeGuardT(ScopeGuardT&&) = default;
    ScopeGuardT& operator=(ScopeGuardT&&) = default;
    

    void disarm() { disarmed = true; }
    
protected:
    Function function;
};

template <typename Function>
class LockGuardT: public ScopeGuardT<Function> {
    using Base = ScopeGuardT<Function>;
public:
    explicit LockGuardT(Function f): Base(f) {}

    void unlock() { Base::function(); Base::disarm(); }
};

template <template <typename> class Guard, typename Function>
auto makeGuard(Function&& f) {
    return Guard<Function>(std::forward<Function>(f));
}


void osalSysLock();
void osalSysUnlock();
void osalSysLockFromISR();
void osalSysUnlockFromISR();

auto makeOsalSysLockGuard() {
    osalSysLock();
    return makeGuard<LockGuardT>([]() { osalSysUnlock(); });
}

auto makeOsalSysLockFromISRGuard() {
    osalSysLockFromISR();
    return makeGuard<LockGuardT>([]() { osalSysUnlockFromISR(); });
}
