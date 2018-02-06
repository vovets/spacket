#pragma once

#define CHECK(expr)                             \
    if (expr) {                                 \
        chprintf(stream, "SUCCESS\r\n");        \
    } else {                                    \
        chprintf(stream, "FAILURE\r\n");        \
        return;                                 \
    }
