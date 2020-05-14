#pragma once

#include <spacket/queue.h>

#include <boost/optional.hpp>

#include <array>

template <typename Buffer, std::size_t Capacity>
class RingT: public QueueT<Buffer> {
public:
    using Element = boost::optional<Buffer>;

private:
    using Array = std::array<Element, Capacity>;

private:
    Element emptyElement;
    Array array;
    std::size_t headIndex = 0;
    std::size_t tailIndex = 0;

public:
    virtual ~RingT() {}

    // why is it needed?
    // google deleting destructors and https://eli.thegreenplace.net/2015/c-deleting-destructors-and-virtual-operator-delete/ 
    void operator delete(void*, std::size_t) {}
    
    bool canPut() const override { return !full(); }
    bool put(Buffer& b) override { return put_(b); }
    
    bool full() const {
        return next(headIndex) == tailIndex;
    }

    // false if full
    bool put_(Buffer& b) {
        std::size_t nextHead = next(headIndex);
        if (nextHead == tailIndex) { return false; }
        array[headIndex] = std::move(b);
        headIndex = nextHead;
        return true;
    }

    // false if empty
    bool eraseTail() {
        if (empty()) { return false; }
        array[tailIndex] = {};
        tailIndex = next(tailIndex);
        return true;
    }

    bool empty() const {
        return headIndex == tailIndex;
    }

    Element& tail() {
        if (empty()) { return emptyElement; }
        return array[tailIndex];
    }

private:
    std::size_t next(std::size_t current) const {
        return (current + 1) % Capacity;
    }
};
