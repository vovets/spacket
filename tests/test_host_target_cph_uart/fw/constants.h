#pragma once

#include <cstdint>

constexpr std::size_t BUFFER_MAX_SIZE = 17;
//constexpr std::size_t BUFFER_MAX_SIZE = 300;
constexpr std::size_t POOL_NUM_BUFFERS = 4;
constexpr std::size_t DRIVER_RX_RING_CAPACITY = 3;
constexpr std::size_t DRIVER_TX_RING_CAPACITY = 3;
constexpr std::size_t STACK_IO_RING_CAPACITY = POOL_NUM_BUFFERS + 1;
constexpr std::size_t STACK_PROC_RING_CAPACITY = 2;

