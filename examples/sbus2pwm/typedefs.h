#pragma once

#include "constants.h"

#include <spacket/buffer.h>
#include <spacket/buffer_pool_allocator.h>
#include <spacket/module.h>
#include <spacket/uart2.h>
#include <spacket/stack.h>
#include <spacket/endpoint.h>
#include <spacket/multistack.h>
#include <spacket/util/guarded_memory_pool.h>

using Pool = GuardedMemoryPoolT<buffer_impl::allocSize(BUFFER_MAX_SIZE), POOL_NUM_BUFFERS>;
using Allocator = PoolAllocatorT<Pool>;
using Executor = ExecutorT<EXECUTOR_CAPACITY>;
using Multistack = MultistackT<Address, 2>;
using Uart2 = Uart2T<2, 1>;
