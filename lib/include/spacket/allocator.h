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

using AllocatorId = std::uint8_t;

struct Object {
    AllocatorId id;
};

class Allocator {
    AllocatorId id;

protected:
    Allocator();

public:
    Result<Object*> allocateObject() {
        return
        allocate() >=
        [&](void* mem) {
            Object* object = reinterpret_cast<Object*>(mem);
            object->id = id;
            return ok(std::move(object));
        };
    }

    void deallocateObject(Object* object) {
        deallocate(object);
    }

    virtual std::size_t maxSize() const = 0;

private:
    virtual Result<void*> allocate() = 0;
    virtual void deallocate(void*) = 0;
};

class Registry {
    static_assert(constants::MAX_ALLOCATORS <= std::numeric_limits<AllocatorId>::max() + 1, "MAX_ALLOCATORS is too big");
    using Array = std::array<Allocator*, constants::MAX_ALLOCATORS>;

    Array array;
    std::size_t firstFree = 0;

public:
    Registry() {
        dpl("Registry::Registry|");
    }

    AllocatorId registerAllocator(Allocator& pool) {
        if (firstFree == array.size()) { FATAL_ERROR("registerPool"); }
        auto id = firstFree++;
        array[id] = &pool;
        dpl("Registry::registerAllocator|registered id=%d", id);
        return static_cast<AllocatorId>(id);
    }

    Allocator& allocator(Object* object) {
        if (object->id >= firstFree) { FATAL_ERROR("PoolMapT::deallocate|bad id"); }
        return *array[object->id];
    }

};

template<typename Dummy = void>
struct RegistryStatic {
    static Storage<Registry> registryStorage;
    static Registry *registryInstance;
};

template<typename Dummy>
Storage<Registry> RegistryStatic<Dummy>::registryStorage;

template<typename Dummy>
Registry* RegistryStatic<Dummy>::registryInstance = nullptr;

inline
Registry& registry() {
    if (RegistryStatic<>::registryInstance == nullptr) {
        RegistryStatic<>::registryInstance = new (&RegistryStatic<>::registryStorage) Registry();
    }
    return *RegistryStatic<>::registryInstance;
}

inline
Allocator::Allocator(): id(registry().registerAllocator(*this)) {
    dpl("Allocator::Allocator|");
}

} // alloc