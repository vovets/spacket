#pragma once

#include <memory>
#include <cstring>

template<typename Allocator, size_t MAX_SIZE>
class BufferT {
    using This = BufferT<Allocator, MAX_SIZE>;
    
public:
    static constexpr size_t maxSize() { return MAX_SIZE; }
    
public:
    BufferT(size_t size);
    BufferT(std::initializer_list<uint8_t> l);
    BufferT(std::vector<uint8_t> v);
    BufferT(BufferT&& toMove);
    BufferT& operator=(BufferT&& toMove);
    
    uint8_t* begin() const;
    uint8_t* end() const;
    size_t size() const;
    BufferT prefix(size_t size) const;
    BufferT suffix(uint8_t* begin) const;
    BufferT copy() const { return prefix(size()); }
    
private:
    struct Deleter{
        void operator()(uint8_t*) const;
    };
    using DataPtr = std::unique_ptr<uint8_t, Deleter>;
    DataPtr data;
    size_t size_;
};

template<typename Allocator, size_t MAX_SIZE>
inline
void BufferT<Allocator, MAX_SIZE>::Deleter::operator()(uint8_t* p) const {
    Allocator().deallocate(p);
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE>::BufferT(size_t size)
    : data(Allocator().allocate(size))
    , size_(size) {
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE>::BufferT(std::initializer_list<uint8_t> l)
    : data(Allocator().allocate(l.size()))
    , size_(l.size()) {
    std::memcpy(begin(), l.begin(), size_);
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE>::BufferT(std::vector<uint8_t> v)
    : data(Allocator().allocate(v.size()))
    , size_(v.size()) {
    std::memcpy(begin(), &v.front(), size_);
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE>::BufferT(BufferT&& toMove)
    : data(std::move(toMove.data))
    , size_(toMove.size_) {
    toMove.size_ = 0;
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE>& BufferT<Allocator, MAX_SIZE>::operator=(BufferT&& toMove) {
    data = std::move(toMove.data);
    size_ = toMove.size_;
    toMove.size_ = 0;
}

template<typename Allocator, size_t MAX_SIZE>
inline
uint8_t* BufferT<Allocator, MAX_SIZE>::begin() const {
    return data.get();
}

template<typename Allocator, size_t MAX_SIZE>
inline
uint8_t* BufferT<Allocator, MAX_SIZE>::end() const {
    return data.get() + size_;
}

template<typename Allocator, size_t MAX_SIZE>
inline
size_t BufferT<Allocator, MAX_SIZE>::size() const {
    return size_;
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE> BufferT<Allocator, MAX_SIZE>::prefix(size_t n) const {
    auto prefix = This(std::min(n, size_));
    std::memcpy(prefix.begin(), begin(), prefix.size());
    return std::move(prefix);
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE> BufferT<Allocator, MAX_SIZE>::suffix(uint8_t* begin) const {
    auto prefix = This(end() - begin);
    std::memcpy(prefix.begin(), begin, prefix.size());
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
