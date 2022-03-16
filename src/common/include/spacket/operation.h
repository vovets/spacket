#pragma once

#include <spacket/buffer_box.h>


struct Operation {
    BufferBox buffer;
    DeferredProc completion;
};
