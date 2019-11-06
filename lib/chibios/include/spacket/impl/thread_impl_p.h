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

struct ThreadParams {
    ThreadStorageBase* storage;
    void* workingArea;
    std::size_t workingAreaSize;
    tprio_t prio;
};
    
class ThreadImpl {
public:
    using NativeHandle = thread_t*;

public:
    template <typename ThreadStorage>
    static ThreadParams params(ThreadStorage& storage, tprio_t prio) {
        return { &storage, &storage.workingArea, sizeof(storage.workingArea), prio };
    }

    static ThreadImpl create(ThreadParams p, std::function<void()> function) {
        p.storage->function = function;
        p.storage->thread = chThdCreateStatic(
            p.workingArea,
            p.workingAreaSize,
            p.prio,
            &threadFunction,
            p.storage);
        return ThreadImpl(p.storage);
    }

    static void checkStack() {
        if (*((uint32_t*)chThdGetWorkingAreaX(chThdGetSelfX())) != 0x55555555U) {
            chSysHalt("stack base overrun");
        }
    }
    

    ThreadImpl(): storage(nullptr) {}

    static void threadFunction(void* arg) {
        static_cast<ThreadStorageBase *>(arg)->function();
    }

    NativeHandle nativeHandle() { return storage == nullptr ? nullptr : storage->thread; }

    bool joinable() const { return storage != nullptr; }

    void requestStop() {
        if (storage != nullptr) {
            chThdTerminate(storage->thread);
        }
    }

    static bool shouldStop() { return chThdShouldTerminateX(); }
    static void setName(const char* name) { chRegSetThreadName(name); }
    static const char* getName() { return chRegGetThreadNameX(chThdGetSelfX()); }

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
