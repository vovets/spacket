#pragma once

#define CHECK(expr)                             \
    if (expr) {                                 \
        chprintf(chp, "SUCCESS\n\n");           \
    } else {                                    \
        chprintf(chp, "FAILURE\n\n");           \
    }
