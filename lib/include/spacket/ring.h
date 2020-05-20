#pragma once

#include <spacket/queue.h>

#include <boost/optional.hpp>

#include <array>

template <typename T, std::size_t Capacity>
class RingT: public QueueT<T> {
private:
    using Element = boost::optional<T>;
    using Array = std::array<Element, Capacity>;

private:
    Array array;
    std::size_t headIndex = 0;
    std::size_t tailIndex = 0;

public:
    ~RingT() override {}

    // why is it needed?
    // google deleting destructors and https://eli.thegreenplace.net/2015/c-deleting-destructors-and-virtual-operator-delete/ 
    void operator delete(void*, std::size_t) {}
    
    bool canPut() const override { return !full(); }
    bool put(T& b) override { return put_(b); }
    
    bool full() const {
        return next(headIndex) == tailIndex;
    }

    // false if full
    bool put_(T& b) {
        std::size_t nextHead = next(headIndex);
        if (nextHead == tailIndex) { return false; }
        array[headIndex] = std::move(b);
        headIndex = nextHead;
        return true;
    }

    // false if empty
    bool eraseTail() {
        if (empty()) { return false; }
        array[tailIndex] = boost::none;
        tailIndex = next(tailIndex);
        return true;
    }

    bool empty() const {
        return headIndex == tailIndex;
    }

    T& tail() {
        if (empty()) { FATAL_ERROR("RingT::tail"); }
        return *array[tailIndex];
    }

private:
    std::size_t next(std::size_t current) const {
        return (current + 1) % Capacity;
    }
};
