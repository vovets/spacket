#pragma once

#include "ch.h"

#include <functional>

#if CH_CFG_USE_WAITEXIT != TRUE
#error CH_CFG_USE_WAITEXIT needs to be defined as TRUE
#endif

struct ThreadStorageBase {
    thread_t* thread;
    std::function<void()> function;
};

template <std::size_t STACK_SIZE>
struct ThreadStorageT: ThreadStorageBase {
    THD_WORKING_AREA(workingArea, STACK_SIZE);
};
    
class ThreadImpl {
public:
    using NativeHandle = thread_t*;

public:
    template <typename ThreadStorage>
    static ThreadImpl create(ThreadStorage& storage, tprio_t prio, std::function<void()> function) {
        storage.function = function;
        storage.thread = chThdCreateStatic(
            storage.workingArea,
            sizeof(storage.workingArea),
            prio,
            &threadFunction,
            &storage);
        return ThreadImpl(&storage);
    }

    ThreadImpl(): storage(nullptr) {}

    static void threadFunction(void* arg) {
        static_cast<ThreadStorageBase *>(arg)->function();
    }

    NativeHandle nativeHandle() { return storage != nullptr ? storage->thread : nullptr; }

    bool joinable() const { return storage != nullptr; }

    void requestStop() {
        if (storage != nullptr) {
            chThdTerminate(storage->thread);
        }
    }

    void wait() {
        if (storage != nullptr) {
            chThdWait(storage->thread);
            storage = nullptr;
        }
    }

private:
    ThreadImpl(ThreadStorageBase* storage)
        : storage(storage)
    {}

private:
    ThreadStorageBase* storage;
};
