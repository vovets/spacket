#pragma once

#include "constants.h"

#include <spacket/buffer.h>
#include <spacket/buffer_pool_allocator.h>
#include <spacket/util/guarded_memory_pool.h>

using Pool = GuardedMemoryPoolT<buffer_impl::allocSize(BUFFER_MAX_SIZE), POOL_NUM_BUFFERS>;
using Allocator = PoolAllocatorT<Pool>;
