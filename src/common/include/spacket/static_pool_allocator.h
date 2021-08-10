#pragma once

#include <spacket/allocator.h>


template <std::size_t SIZE, std::size_t ALIGN, std::size_t NUM_OBJECTS>
class StaticPoolAllocatorT: public alloc::Allocator {
    struct FreeListNode {
        FreeListNode* next;
    };
    
    static_assert(SIZE >= sizeof(FreeListNode) && ALIGN >= alignof(FreeListNode));

    using ObjectStorage = std::aligned_storage_t<SIZE, ALIGN>;
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
        if (size > SIZE) {
            return fail<void*>(
                toError(ErrorCode::StaticPoolAllocatorTooBig));
        }
        if (align > ALIGN) {
            return fail<void*>(
                toError(ErrorCode::StaticPoolAllocatorBadAlign));
        }
        if (freeListHead == nullptr) {
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
        return SIZE;
    }
};
