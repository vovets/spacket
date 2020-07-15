#pragma once

template <typename Element>
struct QueueT {
    // false if full
    virtual bool put(Element&) = 0;
    virtual bool canPut() const = 0;
    virtual ~QueueT() {}
};
