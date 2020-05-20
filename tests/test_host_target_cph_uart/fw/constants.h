#pragma once

#include <cstdint>

constexpr std::size_t BUFFER_MAX_SIZE = 23;
constexpr std::size_t POOL_NUM_BUFFERS = 3;
constexpr std::size_t DRIVER_RX_RING_CAPACITY = 3;
constexpr std::size_t DRIVER_TX_RING_CAPACITY = 3;
constexpr std::size_t STACK_RING_CAPACITY = POOL_NUM_BUFFERS + 1;
constexpr std::uint8_t MULTIPLEXER_NUM_CHANNELS = 2;
