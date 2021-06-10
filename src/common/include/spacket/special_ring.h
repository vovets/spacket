#pragma once

#include <boost/optional.hpp>

#include <array>


template <typename T, std::size_t Capacity>
class SpecialRingT {
private:
    using Element = boost::optional<T>;
    using Array = std::array<Element, Capacity + 1>;

private:
    Array array;
    std::size_t emptyIdx = 0;
    std::size_t fullIdx = 0;
    std::size_t pointIdx = 0;

public:
    bool full() const {
        return next(emptyIdx) == fullIdx;
    }
    
    bool canPut() const {
        return !full();
    }

    bool put(T&& b) {
        if (full()) { return false; }
        array[emptyIdx] = std::move(b);
        emptyIdx = next(emptyIdx);
        return true;
    }

    bool canGet() const {
        return fullIdx != pointIdx;
    }

    T get() {
        if (!canGet()) { FATAL_ERROR("SpecialRingT::get"); }
        T tmp = std::move(*array[fullIdx]);
        fullIdx = next(fullIdx);
        return tmp;
    }

    T* point() {
        if (pointIdx == emptyIdx) { return nullptr; }
        return &*array[pointIdx];
    }

    bool advancePoint() {
        if (pointIdx == emptyIdx) { return false; }
        pointIdx = next(pointIdx);
        return true;
    }

private:
    std::size_t next(std::size_t current) const {
        return (current + 1) % array.size();
    }
};
