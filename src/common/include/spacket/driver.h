#pragma once

#include <spacket/queue.h>
#include <spacket/buffer.h>

struct Driver {
    using Queue = QueueT<Buffer>;

    virtual void start(Queue& rxCompleteQueue, Queue& txCompleteQueue) = 0;

    virtual Queue& rxRequestQueue() = 0;

    virtual Queue& txRequestQueue() = 0;

    virtual void stop() = 0;

    virtual void lock() = 0;

    virtual void unlock() = 0;
};
