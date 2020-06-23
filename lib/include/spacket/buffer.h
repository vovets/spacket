#pragma once

#include <spacket/result.h>
#include <spacket/bind.h>
#include <spacket/result_utils.h>
#include <spacket/debug_print.h>
#include <spacket/allocator.h>

#include <boost/intrusive_ptr.hpp>
#include <cstring>
#include <vector>
#include <limits>

namespace buffer_impl {

struct TypeId {};

struct Header {
    uint16_t size;
    uint8_t refCnt;

    Header(uint16_t size): size(size), refCnt(0) {}

} __attribute__((packed));

inline
constexpr size_t headerSize() { return sizeof(Header) + sizeof(alloc::Object); }

inline
constexpr size_t allocSize(size_t dataSize) {
    return dataSize + headerSize();
}

struct BufferDebug {

#define PREFIX()
#ifdef BUFFER_ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPX_FUNCTIONS
#else
IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPX_FUNCTIONS_NOP
#endif
#undef PREFIX

};

} // buffer_impl

class Buffer: private buffer_impl::BufferDebug {
private:
    struct Storage: alloc::Object {
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
            alloc::registry().allocator(s).deallocateObject(s);
        }
    }

public:
    using TypeId = buffer_impl::TypeId;

private:
    using StoragePtr = boost::intrusive_ptr<Storage>;

private:
    StoragePtr storage;

public:
    std::size_t maxSize() {
        auto& allocator = alloc::registry().allocator(storage.get());
        return
        std::min(
            allocator.maxSize(),
            static_cast<std::size_t>(std::numeric_limits<decltype(buffer_impl::Header::size)>::max()))
        - buffer_impl::headerSize();
    }

    static Result<Buffer> create();
    static Result<Buffer> create(std::size_t size);
    static Result<Buffer> create(std::initializer_list<std::uint8_t> l);
    static Result<Buffer> create(std::vector<std::uint8_t> v);
    
    Buffer(Buffer&& toMove) noexcept;
    Buffer& operator=(Buffer&& toMove) noexcept;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    ~Buffer();

    
    uint8_t* begin();
    uint8_t* end();

    void resize(size_t newSize);
    void release() { storage.reset(); }

    const uint8_t* begin() const;
    const uint8_t* end() const;
    size_t size() const;
    void* id() const { return storage.get(); }
    
    Result<Buffer> prefix(size_t size) const;
    Result<Buffer> suffix(uint8_t* begin) const;
    Result<Buffer> copy() const { return prefix(size()); }

private:
    Buffer(StoragePtr&& data);

    static auto toHeaderSize(size_t size) {
        return static_cast<decltype(buffer_impl::Header::size)>(size);
    }
};

inline
Buffer::Buffer(StoragePtr&& storage)
    : storage(std::move(storage)) {
    dpl("buffer: ctor: %p", this->storage.get());
}

inline
Buffer::~Buffer() {
    dpl("buffer: dtor: %p", this->storage.get());
}

inline
Result<Buffer> Buffer::create() {
    auto& allocator = alloc::defaultAllocator();
    returnOnFailT(p, Buffer, allocator.allocateObject());
    Storage* s = static_cast<Storage*>(p);
    s->header = { toHeaderSize(allocator.maxSize() - buffer_impl::headerSize()) };
    return ok(Buffer(StoragePtr(s)));
}

inline
Result<Buffer> Buffer::create(size_t size) {
    auto& allocator = alloc::defaultAllocator();
    if (size > (allocator.maxSize() - buffer_impl::headerSize())) {
        return fail<Buffer>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, Buffer, allocator.allocateObject());
    Storage* s = static_cast<Storage*>(p);
    s->header = { toHeaderSize(size) };
    return ok(Buffer(StoragePtr(s)));
}

inline
Result<Buffer> Buffer::create(std::initializer_list<std::uint8_t> l) {
    auto& allocator = alloc::defaultAllocator();
    if (l.size() > (allocator.maxSize() - buffer_impl::headerSize())) {
        return fail<Buffer>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, Buffer, allocator.allocateObject());
    Storage* s = static_cast<Storage*>(p);
    s->header = { toHeaderSize(l.size()) };
    Buffer r{StoragePtr(s)};
    std::memcpy(r.begin(), l.begin(), l.size());
    return ok(std::move(r));
}

inline
Result<Buffer> Buffer::create(std::vector<std::uint8_t> v) {
    auto& allocator = alloc::defaultAllocator();
    if (v.size() > (allocator.maxSize() - buffer_impl::headerSize())) {
        return fail<Buffer>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, Buffer, allocator.allocateObject());
    Storage* s = static_cast<Storage*>(p);
    s->header = { toHeaderSize(v.size()) };
    Buffer r{StoragePtr(s)};
    std::memcpy(r.begin(), &v.front(), v.size());
    return ok(std::move(r));
}

inline
Buffer::Buffer(Buffer&& toMove) noexcept
    : storage(std::move(toMove.storage)) {
    dpl("buffer: move ctor: %p", storage.get());
}

inline
Buffer& Buffer::operator=(Buffer&& toMove) noexcept {
    storage = std::move(toMove.storage);
    dpl("buffer: move asgn: %p", storage.get());
    return *this;
}

inline
uint8_t* Buffer::begin() {
    return storage->buffer;
}

inline
uint8_t* Buffer::end() {
    return storage->buffer + size();
}

inline
void Buffer::resize(size_t n) {
    storage->header.size = toHeaderSize(std::min(n, maxSize()));
}

inline
const uint8_t* Buffer::begin() const {
    return storage->buffer;
}

inline
const uint8_t* Buffer::end() const {
    return storage->buffer + size();
}

inline
size_t Buffer::size() const {
    return storage->header.size;
}

inline
Result<Buffer> Buffer::prefix(size_t n) const {
    returnOnFail(prefix, Buffer::create(std::min(n, size())));
    std::memcpy(prefix.begin(), begin(), prefix.size());
    return ok(std::move(prefix));
}

inline
Result<Buffer> Buffer::suffix(uint8_t* begin) const {
    returnOnFail(prefix, Buffer::create(end() - begin));
    std::memcpy(prefix.begin(), begin, prefix.size());
    return ok(std::move(prefix));
}
