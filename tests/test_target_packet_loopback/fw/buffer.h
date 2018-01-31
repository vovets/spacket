#pragma once

#include <spacket/buffer.h>
#include <spacket/buffer_pool_allocator.h>
#include <spacket/util/guarded_memory_pool.h>

constexpr size_t MAX_BUFFER_SIZE = 256;

using Pool = GuardedMemoryPoolT<MAX_BUFFER_SIZE, 8>;
extern Pool pool;
using BufferAllocator = PoolAllocatorT<Pool, pool>;
using Buffer = BufferT<BufferAllocator, MAX_BUFFER_SIZE>;
