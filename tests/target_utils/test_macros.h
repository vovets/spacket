#pragma once

#define CHECK(expr)                             \
    if (expr) {                                 \
        chprintf(chp, "SUCCESS\r\n");           \
    } else {                                    \
        chprintf(chp, "FAILURE\r\n");           \
        return;                                 \
    }
