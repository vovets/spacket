#pragma once

#include "constants.h"

#include <spacket/buffer.h>
#include <spacket/buffer_pool_allocator.h>
#include <spacket/util/guarded_memory_pool.h>
#include <spacket/uart2.h>

using Pool = GuardedMemoryPoolT<buffer_impl::allocSize(BUFFER_MAX_SIZE), POOL_NUM_BUFFERS>;
using Allocator = PoolAllocatorT<Pool>;
using Uart2 = Uart2T<UART_RX_CAPACITY, UART_TX_CAPACITY>;
