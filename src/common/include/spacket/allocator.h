#pragma once

#include <spacket/constants.h>
#include <spacket/memory_utils.h>
#include <spacket/debug_print.h>
#include <spacket/bind.h>

#include <limits>


namespace alloc {

#ifdef ALLOC_ENABLE_DEBUG_PRINT

IMPLEMENT_DPL_FUNCTION

#else

IMPLEMENT_DPL_FUNCTION_NOP

#endif


class Allocator {
public:
    virtual ~Allocator() {}

    virtual Result<void*> allocate(std::size_t size, std::size_t align) = 0;

    virtual void deallocate(void* p) = 0;

    virtual std::size_t maxSize() const = 0;
};

template<typename T>
Result<T*> allocate(Allocator& allocator, std::size_t size) {
    return
    allocator.allocate(size, alignof(T)) >=
    [&](void* p) {
        return ok(static_cast<T*>(p));
    };
}

template<typename T>
Result<T*> allocate(Allocator& allocator) {
    return allocate<T>(allocator, sizeof(T));
}

} // alloc
