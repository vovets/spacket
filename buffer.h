#pragma once

#include <memory>

template<typename Allocator>
class BufferT {
    using This = BufferT<Allocator>;
public:
    BufferT(size_t size);
    uint8_t* begin();
    uint8_t* end();
private:
    struct Deleter{
        void operator()(uint8_t*) const;
    };
    using DataPtr = std::unique_ptr<uint8_t, Deleter>;
    DataPtr data;
    size_t size;
};

template<typename Allocator>
inline
void BufferT<Allocator>::Deleter::operator()(uint8_t* p) const {
    Allocator().deallocate(p);
}

template<typename Allocator>
inline
BufferT<Allocator>::BufferT(size_t size)
    : data(Allocator().allocate(size))
    , size(size) {
}

template<typename Allocator>
inline
uint8_t* BufferT<Allocator>::begin() {
    return data.get();
}

template<typename Allocator>
inline
uint8_t* BufferT<Allocator>::end() {
    return data.get() + size;
}

class StdAllocator {
public:
    uint8_t* allocate(size_t count) {
        return static_cast<uint8_t*>(::operator new[](count));
    }
    void deallocate(uint8_t* ptr) {
        ::operator delete[](ptr);
    }
};

using Buffer = BufferT<StdAllocator>;
