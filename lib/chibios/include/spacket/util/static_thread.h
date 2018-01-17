#pragma once

#include "ch.h"

template <size_t STACK_SIZE>
class StaticThread {
public:
    thread_t* create(tprio_t prio, tfunc_t f, void* arg) {
        return chThdCreateStatic(
            workingArea,
            sizeof(workingArea),
            prio,
            f,
            arg);
    }

private:
    THD_WORKING_AREA(workingArea, STACK_SIZE);
};
