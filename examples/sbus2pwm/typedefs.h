#pragma once

#include "constants.h"

#include <spacket/buffer.h>
#include <spacket/static_pool_allocator.h>
#include <spacket/module.h>
#include <spacket/uart2.h>
#include <spacket/stack.h>
#include <spacket/endpoint.h>
#include <spacket/multistack.h>


using BufferAllocator = StaticPoolAllocatorT<buffer_impl::allocSize(BUFFER_MAX_SIZE), alignof(buffer_impl::Data), POOL_NUM_BUFFERS>;
using ProcAllocator = StaticPoolAllocatorT<PROC_MAX_SIZE, alignof(std::max_align_t), NUM_PROC>;
using Executor = ExecutorT<EXECUTOR_CAPACITY>;
using Multistack = MultistackT<Address, 2>;
using Uart2 = Uart2T<2, 1>;
