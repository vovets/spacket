#pragma once

#include <spacket/impl/thread_impl_p.h>


struct ThreadParams;

class Thread: public ThreadImpl {
    using Impl = ThreadImpl;
    
public:
    template <typename ...U>
    static ThreadParams params(U&&... u) { return Impl::params(std::forward<U>(u)...); }
    
    static Thread create(ThreadParams p, std::function<void()> f) {
        return { ThreadImpl::create(p, f) };
    }

    Thread() {}

    Thread(Thread&&) = default;
    Thread& operator=(Thread&&) = default;

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Impl::NativeHandle nativeHandle() { return Impl::nativeHandle(); }

    bool joinable() const { return Impl::joinable(); }

    void requestStop() { Impl::requestStop(); }

    static bool shouldStop() { return Impl::shouldStop(); }

    void wait() { Impl::wait(); }

private:
    Thread(ThreadImpl&& impl): Impl(std::move(impl)) {}
};
