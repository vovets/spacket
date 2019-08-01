#pragma once

#include "ch.h"


#if CH_CFG_USE_WAITEXIT != TRUE
#error CH_CFG_USE_WAITEXIT needs to be defined as TRUE
#endif

template <size_t STACK_SIZE>
class StaticThreadT {
public:
    StaticThreadT(): thread(nullptr) {}

    thread_t* create(tprio_t prio, tfunc_t f, void* arg) {
        thread = chThdCreateStatic(
            workingArea,
            sizeof(workingArea),
            prio,
            f,
            arg);
        return thread;
    }

    void terminate() {
        chThdTerminate(thread);
    }

    msg_t wait() {
        if (thread != nullptr) {
            auto result = chThdWait(thread);
            thread = nullptr;
            return result;
        }
        return MSG_OK;
    }
private:
    THD_WORKING_AREA(workingArea, STACK_SIZE);
    thread_t* thread;
};
