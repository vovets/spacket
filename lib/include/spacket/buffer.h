#pragma once

#include <spacket/result.h>
#include <spacket/result_utils.h>

#include <memory>
#include <cstring>

template<typename Allocator, size_t MAX_SIZE>
class BufferT {
    using This = BufferT<Allocator, MAX_SIZE>;
    
public:
    static constexpr size_t maxSize() { return MAX_SIZE; }
    
public:
    static Result<BufferT> create(size_t size);
    static Result<BufferT> create(std::initializer_list<uint8_t> l);
    static Result<BufferT> create(std::vector<uint8_t> v);
    
    BufferT(BufferT&& toMove) noexcept;
    BufferT& operator=(BufferT&& toMove) noexcept;
    
    uint8_t* begin() const;
    uint8_t* end() const;
    size_t size() const;
    Result<BufferT> prefix(size_t size) const;
    Result<BufferT> suffix(uint8_t* begin) const;
    Result<BufferT> copy() const { return prefix(size()); }

private:
    BufferT(uint8_t* data, size_t size);
    
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
BufferT<Allocator, MAX_SIZE>::BufferT(uint8_t* data, size_t size)
    : data(data)
    , size_(size) {
}

template<typename Allocator, size_t MAX_SIZE>
inline
Result<BufferT<Allocator, MAX_SIZE>> BufferT<Allocator, MAX_SIZE>::create(size_t size) {
    returnOnFailT(p, BufferT, Allocator().allocate(size));
    return ok(BufferT(p, size));
}

template<typename Allocator, size_t MAX_SIZE>
inline
Result<BufferT<Allocator, MAX_SIZE>> BufferT<Allocator, MAX_SIZE>::create(std::initializer_list<uint8_t> l) {
    returnOnFailT(p, BufferT, Allocator().allocate(l.size()));
    BufferT r(p, l.size());
    std::memcpy(r.begin(), l.begin(), l.size());
    return ok(std::move(r));
}

template<typename Allocator, size_t MAX_SIZE>
inline
Result<BufferT<Allocator, MAX_SIZE>> BufferT<Allocator, MAX_SIZE>::create(std::vector<uint8_t> v) {
    returnOnFailT(p, BufferT, Allocator().allocate(v.size()));
    BufferT r(p, v.size());
    std::memcpy(r.begin(), &v.front(), v.size());
    return ok(std::move(r));
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE>::BufferT(BufferT&& toMove) noexcept
    : data(std::move(toMove.data))
    , size_(toMove.size_) {
    toMove.size_ = 0;
}

template<typename Allocator, size_t MAX_SIZE>
inline
BufferT<Allocator, MAX_SIZE>& BufferT<Allocator, MAX_SIZE>::operator=(BufferT&& toMove) noexcept {
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
Result<BufferT<Allocator, MAX_SIZE>> BufferT<Allocator, MAX_SIZE>::prefix(size_t n) const {
    returnOnFail(prefix, BufferT::create(std::min(n, size_)));
    std::memcpy(prefix.begin(), begin(), prefix.size());
    return std::move(prefix);
}

template<typename Allocator, size_t MAX_SIZE>
inline
Result<BufferT<Allocator, MAX_SIZE>> BufferT<Allocator, MAX_SIZE>::suffix(uint8_t* begin) const {
    returnOnFail(prefix, BufferT::create(end() - begin));
    std::memcpy(prefix.begin(), begin, prefix.size());
    return std::move(prefix);
}
