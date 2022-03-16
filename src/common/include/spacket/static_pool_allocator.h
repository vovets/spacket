#pragma once

#include <spacket/allocator.h>

#include <cstddef>


template <std::size_t OBJECT_SIZE, std::size_t NUM_OBJECTS, std::size_t ALIGN = alignof(std::max_align_t)>
class StaticPoolAllocatorT: public alloc::Allocator {
    struct FreeListNode {
        FreeListNode* next;
    };

    constexpr static std::size_t minAlign = alignof(FreeListNode);
    constexpr static std::size_t align = std::max(ALIGN, minAlign);

    using ObjectStorage = std::aligned_storage_t<OBJECT_SIZE, align>;
    using Array = std::array<ObjectStorage, NUM_OBJECTS>;

    Array data;
    FreeListNode* freeListHead;

    void init() {
        for (auto& o: data) {
            deallocate(&o);
        }
    }

public:
    StaticPoolAllocatorT(): freeListHead(nullptr) {
        init();
    }

    Result<void*> allocate(std::size_t size, std::size_t align) override {
        if (size > OBJECT_SIZE) {
#ifdef SPACKET_STATIC_POOL_ALLOCATOR_ALL_ERRORS_FATAL
            FATAL_ERROR("StaticPoolAllocatorTooBig");
#endif
            return fail<void*>(
                toError(ErrorCode::StaticPoolAllocatorTooBig));
        }
        if (align > ALIGN) {
#ifdef SPACKET_STATIC_POOL_ALLOCATOR_ALL_ERRORS_FATAL
            FATAL_ERROR("StaticPoolAllocatorBadAlign");
#endif
            return fail<void*>(
                toError(ErrorCode::StaticPoolAllocatorBadAlign));
        }
        if (freeListHead == nullptr) {
#ifdef SPACKET_STATIC_POOL_ALLOCATOR_ALL_ERRORS_FATAL
            FATAL_ERROR("StaticPoolAllocatorOutOfMem");
#endif
            return fail<void*>(
                toError(ErrorCode::StaticPoolAllocatorOutOfMem));
        }
        FreeListNode* retval = freeListHead;
        freeListHead = freeListHead->next;
        retval->next = nullptr;
        return ok(static_cast<void*>(std::move(retval)));
    }
    
    void deallocate(void* p) override {
        FreeListNode* n = new (p) FreeListNode{freeListHead};
        freeListHead = n;
    }

    std::size_t maxSize() const override {
        return OBJECT_SIZE;
    }
};
