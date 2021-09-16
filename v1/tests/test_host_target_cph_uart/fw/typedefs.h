#pragma once

#include "constants.h"

#include <spacket/buffer.h>
#include <spacket/static_pool_allocator.h>
#include <spacket/uart2.h>

using Allocator = StaticPoolAllocatorT<buffer_impl::allocSize(BUFFER_MAX_SIZE), POOL_NUM_BUFFERS, alignof(buffer_impl::Data)>;
using ProcAllocator = StaticPoolAllocatorT<PROC_MAX_SIZE, NUM_PROC>;
using Uart2 = Uart2T<UART_RX_CAPACITY, UART_TX_CAPACITY>;
