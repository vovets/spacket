#pragma once

#include <spacket/result.h>
#include <spacket/bind.h>
#include <spacket/result_utils.h>
#include <spacket/debug_print.h>
#include <spacket/allocator.h>

#include <cstring>
#include <vector>
#include <limits>

namespace buffer_impl {

struct TypeId {};

struct BufferDebug {

#define PREFIX()
#ifdef BUFFER_ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
#endif
#undef PREFIX

};

struct Data {
    using SizeType = uint16_t;

    alloc::Allocator& allocator;
    SizeType size;
};

constexpr std::size_t dataSize = sizeof(Data);

constexpr std::size_t allocSize(std::size_t bufferSize) {
    return bufferSize + dataSize;
}

} // buffer_impl


class Buffer: private buffer_impl::BufferDebug {
public:
    using TypeId = buffer_impl::TypeId;

private:
    buffer_impl::Data* data;

    std::uint8_t* buffer() { return reinterpret_cast<std::uint8_t*>(data) + buffer_impl::dataSize; }
    const std::uint8_t* buffer() const {
        return reinterpret_cast<const std::uint8_t*>(data) + buffer_impl::dataSize;
    }

public:
    static std::size_t maxSize(alloc::Allocator& allocator) {
        return
        std::min(
            allocator.maxSize(),
            static_cast<std::size_t>(std::numeric_limits<buffer_impl::Data::SizeType>::max()))
        - buffer_impl::dataSize;
    }

    std::size_t maxSize() {
        return maxSize(data->allocator);
    }

    alloc::Allocator& allocator() {
        return data->allocator;
    }

    static Result<Buffer> create(alloc::Allocator& allocator);
    static Result<Buffer> create(alloc::Allocator& allocator, std::size_t size);
    static Result<Buffer> create(alloc::Allocator& allocator, std::initializer_list<std::uint8_t> l);
    static Result<Buffer> create(alloc::Allocator& allocator, std::vector<std::uint8_t> v);
    
    Buffer(Buffer&& toMove) noexcept;
    Buffer& operator=(Buffer&& toMove) noexcept;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    ~Buffer();

    
    uint8_t* begin();
    uint8_t* end();

    void resize(size_t newSize);
    void release() { deallocate(); }

    const uint8_t* begin() const;
    const uint8_t* end() const;
    size_t size() const;
    void* id() const { return data; }
    
    Result<Buffer> prefix(size_t size) const;
    Result<Buffer> suffix(uint8_t* begin) const;
    Result<Buffer> copy() const { return prefix(size()); }

private:
    Buffer(buffer_impl::Data*& data);

    void deallocate() {
        data->allocator.deallocate(data);
        data = nullptr;
    }
};

inline
Buffer::Buffer(buffer_impl::Data*& src)
    : data(src) {
    src = nullptr;
    dpl("buffer: ctor: %p", this->data);
}

inline
Buffer::~Buffer() {
    dpl("buffer: dtor: %p", this->data);
    if (data != nullptr) {
        deallocate();
    }
}

inline
Result<Buffer> Buffer::create(alloc::Allocator& allocator) {
    return create(allocator, Buffer::maxSize(allocator));
}

inline
Result<Buffer> Buffer::create(alloc::Allocator& allocator, std::size_t size) {
    if (size > maxSize(allocator)) {
        return fail<Buffer>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, Buffer, alloc::allocate<buffer_impl::Data>(allocator, size));
    buffer_impl::Data* s = new (p) buffer_impl::Data{allocator, static_cast<buffer_impl::Data::SizeType>(size)};
    return ok(Buffer(s));
}

inline
Result<Buffer> Buffer::create(alloc::Allocator& allocator, std::initializer_list<std::uint8_t> l) {
    if (l.size() > (maxSize(allocator))) {
        return fail<Buffer>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, Buffer, alloc::allocate<buffer_impl::Data>(allocator, l.size()));
    buffer_impl::Data* s = new (p) buffer_impl::Data{allocator, static_cast<buffer_impl::Data::SizeType>(l.size())};
    Buffer r(s);
    std::memcpy(r.begin(), l.begin(), l.size());
    return ok(std::move(r));
}

inline
Result<Buffer> Buffer::create(alloc::Allocator& allocator, std::vector<std::uint8_t> v) {
    if (v.size() > (maxSize(allocator))) {
        return fail<Buffer>(toError(ErrorCode::BufferCreateTooBig));
    }
    returnOnFailT(p, Buffer, alloc::allocate<buffer_impl::Data>(allocator, v.size()));
    buffer_impl::Data* s = new (p) buffer_impl::Data{allocator, static_cast<buffer_impl::Data::SizeType>(v.size())};
    Buffer r(s);
    std::memcpy(r.begin(), &v.front(), v.size());
    return ok(std::move(r));
}

inline
Buffer::Buffer(Buffer&& toMove) noexcept
    : data(toMove.data) {
    dpl("buffer: move ctor: %p", data);
    toMove.data = nullptr;
}

inline
Buffer& Buffer::operator=(Buffer&& toMove) noexcept {
    if (data) {
        deallocate();
    }
    data = toMove.data;
    toMove.data = nullptr;
    dpl("buffer: move asgn: %p", data);
    return *this;
}

inline
std::uint8_t* Buffer::begin() {
    return buffer();
}

inline
std::uint8_t* Buffer::end() {
    return buffer() + size();
}

inline
void Buffer::resize(std::size_t n) {
    data->size = static_cast<buffer_impl::Data::SizeType>(std::min(n, maxSize()));
}

inline
const std::uint8_t* Buffer::begin() const {
    return buffer();
}

inline
const std::uint8_t* Buffer::end() const {
    return buffer() + size();
}

inline
std::size_t Buffer::size() const {
    return data->size;
}

inline
Result<Buffer> Buffer::prefix(size_t n) const {
    returnOnFail(prefix, Buffer::create(data->allocator, std::min(n, size())));
    std::memcpy(prefix.begin(), begin(), prefix.size());
    return ok(std::move(prefix));
}

inline
Result<Buffer> Buffer::suffix(uint8_t* begin) const {
    returnOnFail(prefix, Buffer::create(data->allocator, end() - begin));
    std::memcpy(prefix.begin(), begin, prefix.size());
    return ok(std::move(prefix));
}
