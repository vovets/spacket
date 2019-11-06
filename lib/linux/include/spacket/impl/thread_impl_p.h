#pragma once

#include <thread>
#include <future>

template <typename ValueType_>
struct ThreadLocal {
    using ValueType = ValueType_;
    static thread_local ValueType value;
};

template <typename ValueType>
typename ThreadLocal<ValueType>::ValueType thread_local ThreadLocal<ValueType>::value;

namespace thread_impl {

inline
const char* defaultThreadName() { static const char* name = "noname"; return name; }

using ShouldStop = ThreadLocal<bool>;
using Name = ThreadLocal<const char*>;

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

    static void checkStack() {}
    static void setName(const char* name) { thread_impl::Name::value = name; }
    static const char* getName() { return thread_impl::Name::value; }

    ThreadImpl(ThreadImpl&& src)
    : thread(std::move(src.thread))
    , shouldStopPtr(src.shouldStopPtr) {
        src.shouldStopPtr = nullptr;
    }

    ThreadImpl& operator=(ThreadImpl&& from) {
        thread = std::move(from.thread);
        shouldStopPtr = from.shouldStopPtr;
        from.shouldStopPtr = nullptr;
        return *this;
    }

    ThreadImpl(const ThreadImpl&) = delete;

    ThreadImpl(): shouldStopPtr(nullptr) {}

    NativeHandle nativeHandle() { return thread.native_handle(); }

    bool joinable() const { return thread.joinable(); }

    void requestStop() { *shouldStopPtr = true; }

    static bool shouldStop() { return thread_impl::ShouldStop::value; }

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
                thread_impl::ShouldStop::value = false;
                thread_impl::Name::value = thread_impl::defaultThreadName();
                this->shouldStopPtr = &thread_impl::ShouldStop::value;
                promise.set_value();
                f();
            });
        future.wait();
    }

private:
    std::thread thread;
    bool* shouldStopPtr;
};
