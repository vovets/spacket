#pragma once

#include <spacket/impl/thread_impl_p.h>

class Thread: public ThreadImpl {
    using Impl = ThreadImpl;
public:
    template <typename ...U>
    static Thread create(U&&... u) {
        return ThreadImpl::template create(std::forward<U>(u)...);
    }

    Thread() {}

    Thread(Thread&&) = default;
    Thread& operator=(Thread&&) = default;

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    bool joinable() const {
        return Impl::joinable();
    }

    void requestStop() {
        Impl::requestStop();
    }

    void wait() {
        Impl::wait();
    }

private:
    Thread(ThreadImpl&& impl): Impl(impl) {}
};
