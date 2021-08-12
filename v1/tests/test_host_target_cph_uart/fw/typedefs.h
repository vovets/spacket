#pragma once

#include "constants.h"

#include <spacket/buffer.h>
#include <spacket/static_pool_allocator.h>
#include <spacket/uart2.h>

using Allocator = StaticPoolAllocatorT<buffer_impl::allocSize(BUFFER_MAX_SIZE), alignof(buffer_impl::Data), POOL_NUM_BUFFERS>;
using ProcAllocator = StaticPoolAllocatorT<PROC_MAX_SIZE, alignof(std::max_align_t), NUM_PROC>;
using Uart2 = Uart2T<UART_RX_CAPACITY, UART_TX_CAPACITY>;
