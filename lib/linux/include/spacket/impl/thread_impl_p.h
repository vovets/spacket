#pragma once

#include <thread>
#include <future>

namespace thread_impl {

thread_local bool shouldStop = false;

} // thread_impl

struct ThreadParams {};

class ThreadImpl {
public:
    using NativeHandle = std::thread::native_handle_type;
    
public:
    static ThreadParams params() { return {}; }

    static ThreadImpl create(ThreadParams p, std::function<void()> f) {
        return { p, f };
    }

    ThreadImpl(ThreadImpl&& src)
    : thread(std::move(src.thread))
    , shouldStopPtr(src.shouldStopPtr) {
        src.shouldStopPtr = nullptr;
    }

    ThreadImpl(const ThreadImpl&) = delete;

    ThreadImpl(): shouldStopPtr(nullptr) {}

    NativeHandle nativeHandle() { return thread.native_handle(); }

    bool joinable() const { return thread.joinable(); }

    void requestStop() { *shouldStopPtr = true; }

    static bool shouldStop() { return thread_impl::shouldStop; }

    void wait() { thread.join(); }

private:
    ThreadImpl(ThreadParams, std::function<void()> f)
        : shouldStopPtr(nullptr)
    {
        // TODO: may be use simpler synchronisation?
        std::promise<void> promise;
        auto future = promise.get_future();
        thread = std::thread(
            [this, f, &promise] {
                this->shouldStopPtr = &thread_impl::shouldStop;
                promise.set_value();
                f();
            });
        future.wait();
    }

private:
    std::thread thread;
    bool* shouldStopPtr;
};
