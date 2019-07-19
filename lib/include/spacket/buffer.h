#pragma once

#include <spacket/result.h>
#include <spacket/bind.h>
#include <spacket/result_utils.h>
#include <spacket/debug_print.h>

#include <boost/intrusive_ptr.hpp>
#include <cstring>


namespace buffer_impl {

struct TypeId {};

struct Header {
    uint16_t size;
    uint8_t refCnt;
    // TODO: proper alignment

    Header(uint16_t size): size(size), refCnt(0) {}

} __attribute__((packed));

inline
constexpr size_t headerSize() { return sizeof(Header); }

inline
constexpr size_t allocSize(size_t dataSize) {
    return dataSize + headerSize();
}

#ifdef BUFFER_ENABLE_DEBUG_PRINT

IMPLEMENT_DPX_FUNCTIONS

#else

IMPLEMENT_DPX_FUNCTIONS_NOP

#endif

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

    void resize(size_t newSize);
    void release() { storage.reset(); }

    const uint8_t* begin() const;
    const uint8_t* end() const;
    size_t size() const;
    void* id() const { return storage.get(); }
    
    Result<BufferT> prefix(size_t size) const;
    Result<BufferT> suffix(uint8_t* begin) const;
    Result<BufferT> copy() const { return prefix(size()); }

private:
    using StoragePtr = boost::intrusive_ptr<Storage>;

private:
    BufferT(StoragePtr&& data);

    static auto toHeaderSize(size_t size) {
        return static_cast<decltype(buffer_impl::Header::size)>(size);
    }
    
private:
    StoragePtr storage;
};

template<typename Allocator>
inline
BufferT<Allocator>::BufferT(StoragePtr&& storage)
    : storage(std::move(storage)) {
    buffer_impl::dpl("buffer created: %x", this->storage.get());
}

template<typename Allocator>
inline
Result<BufferT<Allocator>> BufferT<Allocator>::create(size_t size) {
    if (size > maxSize()) {
        return fail<BufferT>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, BufferT, Allocator().allocate(buffer_impl::allocSize(size)));
    Storage* s = static_cast<Storage*>(p);
    s->header = { toHeaderSize(size) };
    return ok(BufferT(StoragePtr(s)));
}

template<typename Allocator>
inline
Result<BufferT<Allocator>> BufferT<Allocator>::create(std::initializer_list<uint8_t> l) {
    if (l.size() > maxSize()) {
        return fail<BufferT>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, BufferT, Allocator().allocate(buffer_impl::allocSize(l.size())));
    Storage* s = static_cast<Storage*>(p);
    s->header = { toHeaderSize(l.size()) };
    BufferT<Allocator> r{StoragePtr(s)};
    std::memcpy(r.begin(), l.begin(), l.size());
    return ok(std::move(r));
}

template<typename Allocator>
inline
Result<BufferT<Allocator>> BufferT<Allocator>::create(std::vector<uint8_t> v) {
    if (v.size() > maxSize()) {
        return fail<BufferT>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, BufferT, Allocator().allocate(buffer_impl::allocSize(v.size())));
    Storage* s = static_cast<Storage*>(p);
    s->header = { toHeaderSize(v.size()) };
    BufferT<Allocator> r{StoragePtr(s)};
    std::memcpy(r.begin(), &v.front(), v.size());
    return ok(std::move(r));
}

template<typename Allocator>
inline
BufferT<Allocator>::BufferT(BufferT&& toMove) noexcept
    : storage(std::move(toMove.storage)) {
    buffer_impl::dpl("buffer: moved %x", storage.get());
}

template<typename Allocator>
inline
BufferT<Allocator>& BufferT<Allocator>::operator=(BufferT&& toMove) noexcept {
    storage = std::move(toMove.storage);
    buffer_impl::dpl("buffer: moved %x", storage.get());
    return *this;
}

template<typename Allocator>
inline
BufferT<Allocator>::BufferT(const BufferT& src) noexcept
    : storage(src.storage) {
}

template<typename Allocator>
inline
BufferT<Allocator>& BufferT<Allocator>::operator=(const BufferT& src) noexcept {
    storage = src.storage;
    return *this;
}

template<typename Allocator>
inline
uint8_t* BufferT<Allocator>::begin() {
    return storage->buffer;
}

template<typename Allocator>
inline
uint8_t* BufferT<Allocator>::end() {
    return storage->buffer + size();
}

template<typename Allocator>
inline
void BufferT<Allocator>::resize(size_t n) {
    storage->header.size = toHeaderSize(std::min(n, maxSize()));
}

template<typename Allocator>
inline
const uint8_t* BufferT<Allocator>::begin() const {
    return storage->buffer;
}

template<typename Allocator>
inline
const uint8_t* BufferT<Allocator>::end() const {
    return storage->buffer + size();
}

template<typename Allocator>
inline
size_t BufferT<Allocator>::size() const {
    return storage->header.size;
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
