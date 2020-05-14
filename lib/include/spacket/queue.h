#pragma once

template <typename Buffer>
struct QueueT {
    // false if full
    virtual bool put(Buffer&) = 0;
    virtual bool canPut() const = 0;
    virtual ~QueueT() {}
};
