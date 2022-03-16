#pragma once

#include "constants.h"

#include <spacket/buffer.h>
#include <spacket/static_pool_allocator.h>
#include <spacket/module.h>
#include <spacket/uart2.h>
#include <spacket/stack.h>


using BufferAllocator = StaticPoolAllocatorT<buffer_impl::allocSize(BUFFER_MAX_SIZE), POOL_NUM_BUFFERS, alignof(buffer_impl::Data)>;
using FuncAllocator = StaticPoolAllocatorT<PROC_MAX_SIZE, NUM_PROC>;
using TxtBufferAllocator = StaticPoolAllocatorT<buffer_impl::allocSize(97), 4, alignof(buffer_impl::Data)>;
using Executor = ExecutorT<EXECUTOR_CAPACITY>;
using Uart2 = Uart2T<2, 1>;
