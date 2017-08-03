#pragma once

#include <memory>
#include <cstring>

template<typename Allocator>
class BufferT {
    using This = BufferT<Allocator>;
public:
    BufferT(size_t size);
    BufferT(std::initializer_list<uint8_t> l);
    uint8_t* begin() const;
    uint8_t* end() const;
    size_t size() const;
    BufferT prefix(size_t size) const;
private:
    struct Deleter{
        void operator()(uint8_t*) const;
    };
    using DataPtr = std::unique_ptr<uint8_t, Deleter>;
    DataPtr data;
    size_t size_;
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
    , size_(size) {
}

template<typename Allocator>
inline
BufferT<Allocator>::BufferT(std::initializer_list<uint8_t> l)
    : data(Allocator().allocate(l.size()))
    , size_(l.size()) {
    std::memcpy(begin(), l.begin(), size_);
}

template<typename Allocator>
inline
uint8_t* BufferT<Allocator>::begin() const {
    return data.get();
}

template<typename Allocator>
inline
uint8_t* BufferT<Allocator>::end() const {
    return data.get() + size_;
}

template<typename Allocator>
inline
size_t BufferT<Allocator>::size() const {
    return size_;
}

template<typename Allocator>
inline
BufferT<Allocator> BufferT<Allocator>::prefix(size_t n) const {
    auto prefix = This(std::min(n, size_));
    std::memcpy(prefix.begin(), begin(), prefix.size());
    return std::move(prefix);
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
