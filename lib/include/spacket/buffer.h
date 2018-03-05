#pragma once

#include <spacket/result.h>
#include <spacket/bind.h>
#include <spacket/result_utils.h>

#include <boost/intrusive_ptr.hpp>
#include <cstring>

namespace buffer_impl {

struct TypeId {};

struct Header {
    uint16_t size;
    uint8_t refCnt;
} __attribute__((packed));

static constexpr size_t headerSize() { return sizeof(Header); }

static constexpr size_t allocSize(size_t dataSize) {
    return dataSize + headerSize();
}

}

template<typename Allocator>
class BufferT {
private:
    struct Storage {
        buffer_impl::Header header;
        uint8_t buffer[];
    };

    friend void intrusive_ptr_add_ref(Storage* s) {
        if (s->header.refCnt < std::numeric_limits<decltype(s->header.refCnt)>::max()) {
            ++s->header.refCnt;
        }
    }
    
    friend void intrusive_ptr_release(Storage* s) {
        --s->header.refCnt;
        if (s->header.refCnt == 0) {
            Allocator().deallocate(s);
        }
    }

public:
    using TypeId = buffer_impl::TypeId;

public:
    static constexpr size_t maxSize() {
        return
        std::min(
            Allocator::maxSize(),
            static_cast<size_t>(std::numeric_limits<decltype(buffer_impl::Header::size)>::max()))
        - buffer_impl::headerSize();
    }

    static Result<BufferT> create(size_t size);
    static Result<BufferT> create(std::initializer_list<uint8_t> l);
    static Result<BufferT> create(std::vector<uint8_t> v);
    
    BufferT(BufferT&& toMove) noexcept;
    BufferT& operator=(BufferT&& toMove) noexcept;
    
    BufferT(const BufferT& src) noexcept;
    BufferT& operator=(const BufferT& src) noexcept;

    uint8_t* begin();
    uint8_t* end();
    size_t size();

    const uint8_t* begin() const;
    const uint8_t* end() const;
    size_t size() const;
    
    Result<BufferT> prefix(size_t size) const;
    Result<BufferT> suffix(uint8_t* begin) const;
    Result<BufferT> copy() const { return prefix(size()); }

private:
    BufferT(Storage* data);

    static Storage* storage(void* p) { return static_cast<Storage*>(p); }
    
private:
    using DataPtr = boost::intrusive_ptr<Storage>;
    DataPtr data;
};

template<typename Allocator>
inline
BufferT<Allocator>::BufferT(Storage* data)
    : data(data) {
}

template<typename Allocator>
inline
Result<BufferT<Allocator>> BufferT<Allocator>::create(size_t size) {
    returnOnFailT(p, BufferT, Allocator().allocate(size + buffer_impl::headerSize()));
    Storage* s = storage(p);
    s->header.size = size;
    return ok(BufferT(s));
}

template<typename Allocator>
inline
Result<BufferT<Allocator>> BufferT<Allocator>::create(std::initializer_list<uint8_t> l) {
    returnOnFailT(p, BufferT, Allocator().allocate(l.size() + buffer_impl::headerSize()));
    Storage* s = storage(p);
    s->header.size = l.size();
    BufferT r(s);
    std::memcpy(r.begin(), l.begin(), l.size());
    return ok(std::move(r));
}

template<typename Allocator>
inline
Result<BufferT<Allocator>> BufferT<Allocator>::create(std::vector<uint8_t> v) {
    returnOnFailT(p, BufferT, Allocator().allocate(v.size() + buffer_impl::headerSize()));
    Storage* s = storage(p);
    s->header.size = v.size();
    BufferT r(s);
    std::memcpy(r.begin(), &v.front(), v.size());
    return ok(std::move(r));
}

template<typename Allocator>
inline
BufferT<Allocator>::BufferT(BufferT&& toMove) noexcept
    : data(std::move(toMove.data)) {
}

template<typename Allocator>
inline
BufferT<Allocator>& BufferT<Allocator>::operator=(BufferT&& toMove) noexcept {
    data = std::move(toMove.data);
    return *this;
}

template<typename Allocator>
inline
BufferT<Allocator>::BufferT(const BufferT& src) noexcept
    : data(src.data) {
}

template<typename Allocator>
inline
BufferT<Allocator>& BufferT<Allocator>::operator=(const BufferT& src) noexcept {
    data = src.data;
    return *this;
}

template<typename Allocator>
inline
uint8_t* BufferT<Allocator>::begin() {
    return data.get()->buffer;
}

template<typename Allocator>
inline
uint8_t* BufferT<Allocator>::end() {
    return data.get()->buffer + size();
}

template<typename Allocator>
inline
size_t BufferT<Allocator>::size() {
    return data.get()->header.size;
}

template<typename Allocator>
inline
const uint8_t* BufferT<Allocator>::begin() const {
    return data.get()->buffer;
}

template<typename Allocator>
inline
const uint8_t* BufferT<Allocator>::end() const {
    return data.get()->buffer + size();
}

template<typename Allocator>
inline
size_t BufferT<Allocator>::size() const {
    return data.get()->header.size;
}

template<typename Allocator>
inline
Result<BufferT<Allocator>> BufferT<Allocator>::prefix(size_t n) const {
    returnOnFail(prefix, BufferT::create(std::min(n, size())));
    std::memcpy(prefix.begin(), begin(), prefix.size());
    return ok(std::move(prefix));
}

template<typename Allocator>
inline
Result<BufferT<Allocator>> BufferT<Allocator>::suffix(uint8_t* begin) const {
    returnOnFail(prefix, BufferT::create(end() - begin));
    std::memcpy(prefix.begin(), begin, prefix.size());
    return ok(std::move(prefix));
}
