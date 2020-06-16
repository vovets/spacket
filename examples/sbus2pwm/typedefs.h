#pragma once

#include "constants.h"

#include <spacket/buffer.h>
#include <spacket/buffer_pool_allocator.h>
#include <spacket/module.h>
#include <spacket/uart.h>
#include <spacket/stack.h>
#include <spacket/endpoint.h>
#include <spacket/multistack.h>
#include <spacket/util/guarded_memory_pool.h>

using Pool = GuardedMemoryPoolT<buffer_impl::allocSize(BUFFER_MAX_SIZE), POOL_NUM_BUFFERS>;
using BufferAllocator = PoolAllocatorT<Pool>;
using Buffer = BufferT<BufferAllocator>;
using Module = ModuleT<Buffer>;
using Uart = UartT<Buffer, DRIVER_RX_RING_CAPACITY, DRIVER_TX_RING_CAPACITY>;
using Stack = StackT<Buffer, STACK_IO_RING_CAPACITY, STACK_PROC_RING_CAPACITY>;
using Endpoint = EndpointT<Buffer>;
using Address = AddressT<Buffer>;
using Multistack = MultistackT<Buffer, Address, 2>;
