#pragma once


class ScopeGuardBase {
protected:
    ScopeGuardBase(): disarmed(false) {}
    
    ScopeGuardBase(ScopeGuardBase&& from): disarmed(from.disarmed) {
        from.disarmed = true;
    }
    
    ScopeGuardBase& operator=(ScopeGuardBase&& from) {
        disarmed = from.disarmed;
        from.disarmed = true;
        return *this;
    }
    
    bool disarmed = false;
};

template <typename Function>
class ScopeGuardT: public ScopeGuardBase {
public:
    template <typename F>
    explicit ScopeGuardT(F&& f): function(std::forward<F>(f)) {}
    ~ScopeGuardT() { if (!disarmed) { function(); } }

    ScopeGuardT(const ScopeGuardT&) = delete;
    ScopeGuardT& operator=(const ScopeGuardT&) = delete;

    ScopeGuardT(ScopeGuardT&&) = default;
    ScopeGuardT& operator=(ScopeGuardT&& from) {
        this->function.~Function();
        new (&this->function) Function(std::move(from.function));
        return *this;
    }

    void disarm() { disarmed = true; }
    
protected:
    Function function;
};

template <typename Function>
class LockGuardT: public ScopeGuardT<Function> {
    using Base = ScopeGuardT<Function>;
public:
    template <typename F>
    explicit LockGuardT(F&& f): Base(std::forward<F>(f)) {}

    LockGuardT(const LockGuardT&) = delete;
    LockGuardT& operator=(const LockGuardT&) = delete;

    LockGuardT(LockGuardT&&) = default;
    LockGuardT& operator=(LockGuardT&&) = default;

    void unlock() { Base::function(); Base::disarm(); }
};

template <template <typename> class Guard, typename Function>
auto makeGuard(Function&& f) {
    return Guard<Function>(std::forward<Function>(f));
}

template <typename FunctionAcquire, typename FunctionRelease>
auto makeGuard(FunctionAcquire acquire, FunctionRelease&& release) {
    acquire();
    return makeGuard<LockGuardT>(std::forward<FunctionRelease>(release));
}

void osalSysLock();
void osalSysUnlock();
void osalSysLockFromISR();
void osalSysUnlockFromISR();

inline
auto makeOsalSysLockGuard() {
    return makeGuard([]() { osalSysLock(); }, []() { osalSysUnlock(); });
}

inline
auto makeOsalSysLockFromISRGuard() {
    return makeGuard([]() { osalSysLockFromISR(); }, []() { osalSysUnlockFromISR(); });
}
